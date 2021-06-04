#include "Encoder.h"
#include "Msg.h"
#include "Params.h"

    //-- class H264Encoder --//

H264Encoder::~H264Encoder()
{
    flush();
}

void H264Encoder::encode(const Frame & frame)
{
    if(!mRecoveryTimeout)
        return;

    FrameSize encodeSize = frame.size.scaled(Params()->scale)
            .aligned(4).bounded({320, 200}, {1920, 1080});

    if(encodeSize != mEncodeSize) {
        Msg(FILELINE, 2) << "New frame size, encoder resources will be recreated";
        flush();
        mhEncoder.release();
        mYuvImage.release();
        mhConvertCtx.release();
        mFrameCount = 0;
        mEncodeSize = encodeSize;
    }

    if(!mhConvertCtx) {
        Msg(FILELINE, 2) << "Obtaining image converter context";
        mhConvertCtx = sws_getContext(
                    frame.size.width, frame.size.height, AV_PIX_FMT_BGRA,
                    mEncodeSize.width, mEncodeSize.height, AV_PIX_FMT_YUV420P,
                    SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if(!mhConvertCtx) {
            Msg(FILELINE) << "Could not obtain image converter context";
            mRecoveryTimeout.start();
            return;
        }
    }

    if(!mYuvImage) {
        Msg(FILELINE, 2) << "Allocating YUV image";
        size_t dataSize = mYuvImage.alloc(
                    mEncodeSize.width, mEncodeSize.height, AV_PIX_FMT_YUV420P);
        if(dataSize <= 0) {
            Msg(FILELINE) << "Could not allocate YUV image";
            mRecoveryTimeout.start();
            return;
        }
    }

    if(!mhEncoder) {
        /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
        x264_param_t params = {};
        /**/
        x264_param_t params;
        memset(&params, 0, sizeof(params));
        /**/

        Msg(FILELINE, 2) << "Obtaining x264 preset";
        if(x264_param_default_preset(
                    &params, Params()->preset,
                    "animation+zerolatency") < 0) {
            Msg(FILELINE) << "Could not obtain x264 preset";
            mRecoveryTimeout.start();
            return;
        }

        Msg(FILELINE, 2) << "Filling x264 param structure";
        // Input format:
        params.i_csp = X264_CSP_I420; // colorspace
        params.i_width = mEncodeSize.width;
        params.i_height = mEncodeSize.height;
        params.b_vfr_input = 0; // constant frame rate
        params.i_fps_num = Params()->fps.num;
        params.i_fps_den = Params()->fps.den;
        // Output control:
        params.rc.i_rc_method = X264_RC_CRF;
        params.rc.f_rf_constant =     Params()->crf;
        params.rc.f_rf_constant_max = Params()->crfMax;
        if(Params()->bitrate > 0) {
            params.rc.i_vbv_max_bitrate = Params()->bitrate;
            params.rc.i_vbv_buffer_size = Params()->bitrate * 2;
            params.rc.f_vbv_buffer_init = 1.0;
        }
        // Frame control:
        if(Params()->keyint > 0)
            params.i_keyint_max = Params()->keyint;
        params.b_intra_refresh =
                (Params()->intraRefresh ? 1 : 0);
        // For streaming:
        params.b_repeat_headers = 1;
        params.b_annexb = 1;
        // Processing control:
        params.i_log_level = X264_LOG_ERROR;
        params.i_threads = 1;

        Msg(FILELINE, 2) << "Applying x264 profile";
        if(x264_param_apply_profile(&params, "main") < 0) {
            Msg(FILELINE) << "Could not apply x264 profile";
            mRecoveryTimeout.start();
            return;
        }

        Msg(FILELINE, 2) << "Opening x264 encoder";
        mhEncoder = x264_encoder_open(&params);
        if(!mhEncoder) {
            Msg(FILELINE) << "Could not open x264 encoder";
            mRecoveryTimeout.start();
            return;
        }
    }

    Msg(FILELINE, 2) << "Converting and scaling image";
    const uint8_t * pRgbPlanes[4] = {
        (const uint8_t *)frame.pPixels, nullptr, nullptr, nullptr};
    int rgbStrides[4] = {
        int(frame.pitch), 0, 0, 0};
    sws_scale(mhConvertCtx, pRgbPlanes, rgbStrides, 0, frame.size.height,
              mYuvImage.pPlanes(), mYuvImage.strides());

    Msg(FILELINE, 2) << "Filling x264 picture structure";
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    x264_picture_t picture = {};
    /**/
    x264_picture_t picture;
    memset(&picture, 0, sizeof(picture));
    /**/
    picture.i_type = X264_QP_AUTO;
    picture.i_pts = mFrameCount++;
    picture.img.i_csp = X264_CSP_I420;
    picture.img.i_plane = mYuvImage.planeCount();
    for(int i = 0; i < 4; ++i)
        picture.img.plane[i] = const_cast<uint8_t *>(mYuvImage.pPlanes(i));
    for(int i = 0; i < 4; ++i)
        picture.img.i_stride[i] = mYuvImage.strides(i);

    encode(&picture);
}

void H264Encoder::flush()
{
    if(!mhEncoder)
        return;

    Msg(FILELINE, 2) << "Flushing pending encoded data";
    while(x264_encoder_delayed_frames(mhEncoder))
        encode(nullptr);
}

void H264Encoder::encode(x264_picture_t * picture)
{
    Msg(FILELINE, 2) << "Encoding image data";
    x264_picture_t pictureOut;
    x264_nal_t * nals;
    int nalCount;
    int bulkSize = x264_encoder_encode(
                mhEncoder, &nals, &nalCount, picture, &pictureOut);
    if(bulkSize < 0) {
        Msg(FILELINE) << "Could not encode image data, error " << bulkSize;
        mRecoveryTimeout.start();
        return;
    }

    if(mNalMode == NalMode::wholeBulk) {
        deliver(nals[0].p_payload, bulkSize);
        return;
    }

    for(int i = 0; i < nalCount; ++i) {
        uint8_t * pData = nals[i].p_payload;
        size_t size = nals[i].i_payload;
        if(mNalMode == NalMode::withoutStartcodes) {
            Msg(FILELINE, 2) << "Removing encoded data startcodes";
            if(size >= 3 && pData[0] == 0x00 && pData[1] == 0x00) {
                if(pData[2] == 0x01) {
                    pData += 3;
                    size -= 3;
                } else
                if(size >= 4 && pData[2] == 0x00 && pData[3] == 0x01) {
                    pData += 4;
                    size -= 4;
                }
            }
        }
        Msg(FILELINE, 2) << "Delivering encoded data";
        deliver(pData, size);
    }
}
