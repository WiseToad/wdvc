#ifndef TEXT_H
#define TEXT_H

#include "TypeTraits.h"
#include "Stateful.h"
#include "Buffer.h"
#include <string>

    //-- template class BufferCvt --//

template <typename TargetEncodingT, typename SourceEncodingT = TargetEncodingT>
class BufferCvt: public SimpleStateful
{
public:
    template <typename TargetCharT, typename SourceCharT>
    size_t cvt(BufferRef<TargetCharT> target, const SourceCharT * pSource);
};

    //-- template class StringCvt --//

template <typename TargetEncodingT, typename SourceEncodingT = TargetEncodingT>
class StringCvt: public BufferCvt<TargetEncodingT, SourceEncodingT>
{
public:
    template <typename TargetCharT, typename SourceCharT>
    std::basic_string<TargetCharT> cvt(const std::basic_string<SourceCharT> & source);
};

    //-- template class Text --//

template <typename CharT, typename EncodingT = void>
class Text final: public SimpleStateful
{
public:
    using CharType = CharT;

    Text() = default;

    Text(CharT chr, size_t num = 1):
        mStr(num, chr) {}

    template <typename OtherCharT, typename std::enable_if<
                  is_char<OtherCharT>::value, int>::type = 0>
    Text(OtherCharT chr, size_t num = 1):
        Text(TextCvt<OtherCharT, EncodingT>(std::basic_string<OtherCharT>(num, chr))) {}

    Text(const CharT * pStr):
        mStr(pStr) {}

    template <typename OtherCharT, typename std::enable_if<
                  is_char<OtherCharT>::value, int>::type = 0>
    Text(const OtherCharT * pStr):
        Text(TextCvt<OtherCharT, EncodingT>(pStr)) {}

    Text(const std::basic_string<CharT> & str):
        mStr(str) {}

    Text(std::basic_string<CharT> && str):
        mStr(std::move(str)) {}

    template <typename OtherCharT>
    Text(const std::basic_string<OtherCharT> & str):
        Text(TextCvt<OtherCharT, EncodingT>(str)) {}

    Text(const Text & other):
        SimpleStateful(other.state()), mStr(other.mStr) {}

    Text(Text && other):
        SimpleStateful(other.state()), mStr(std::move(other.mStr)) {}

    template <typename OtherCharT, typename OtherEncodingT>
    Text(const Text<OtherCharT, OtherEncodingT> & other):
        Text(TextCvt<OtherCharT, OtherEncodingT>(other)) {}

    // deleted
    template <typename T, typename std::enable_if<
                  is_numeric<T>::value || is_bool<T>::value, int>::type = 0>
    Text(T) = delete;

    Text & operator = (CharT chr) {
        setState(State::valid);
        mStr = chr;
        return *this;
    }
    template <typename OtherCharT>
    typename std::enable_if<is_char<OtherCharT>::value, Text &>::type
    operator = (OtherCharT chr) {
        Text text(chr);
        setState(text.state());
        mStr = std::move(text.mStr);
        return *this;
    }
    Text & operator = (const CharT * pStr) {
        setState(State::valid);
        mStr = pStr;
        return *this;
    }
    template <typename OtherCharT>
    typename std::enable_if<is_char<OtherCharT>::value, Text &>::type
    operator = (const OtherCharT * pStr) {
        Text text(pStr);
        setState(text.state());
        mStr = std::move(text.mStr);
        return *this;
    }
    Text & operator = (std::basic_string<CharT> str) {
        setState(State::valid);
        mStr = std::move(str);
        return *this;
    }
    template <typename OtherCharT>
    Text & operator = (const std::basic_string<OtherCharT> & str) {
        Text text(str);
        setState(text.state());
        mStr = std::move(text.mStr);
        return *this;
    }
    Text & operator = (Text other) {
        setState(other.state());
        mStr = std::move(other.mStr);
        return *this;
    }
    template <typename OtherCharT, typename OtherEncodingT>
    Text & operator = (const Text<OtherCharT, OtherEncodingT> & other) {
        Text text(other);
        setState(text.state());
        mStr = std::move(text.mStr);
        return *this;
    }

    // deleted
    template <typename T>
    typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, Text &>::type
    operator = (T) = delete;

    Text & operator += (CharT rhs) {
        mStr += rhs;
        return *this;
    }
    template <typename OtherCharT>
    typename std::enable_if<is_char<OtherCharT>::value, Text &>::type
    operator += (OtherCharT rhs) {
        Text text(rhs);
        if(state(State::valid))
            setState(text.state());
        mStr += text.mStr;
        return *this;
    }
    Text & operator += (const CharT * rhs) {
        mStr += rhs;
        return *this;
    }
    template <typename OtherCharT>
    typename std::enable_if<is_char<OtherCharT>::value, Text &>::type
    operator += (const OtherCharT * rhs) {
        Text text(rhs);
        if(state(State::valid))
            setState(text.state());
        mStr += text.mStr;
        return *this;
    }
    Text & operator += (const std::basic_string<CharT> & rhs) {
        mStr += rhs;
        return *this;
    }
    template <typename OtherCharT>
    Text & operator += (const std::basic_string<OtherCharT> & rhs) {
        Text text(rhs);
        if(state(State::valid))
            setState(text.state());
        mStr += text.mStr;
        return *this;
    }
    Text & operator += (const Text & rhs) {
        mStr += rhs;
        return *this;
    }
    template <typename OtherCharT, typename OtherEncodingT>
    Text & operator += (const Text<OtherCharT, OtherEncodingT> & rhs) {
        Text text(rhs);
        if(state(State::valid))
            setState(text.state());
        mStr += text.mStr;
        return *this;
    }

    // deleted
    template <typename T>
    typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, Text &>::type
    operator += (T) = delete;

    friend std::basic_string<CharT> & operator += (
            std::basic_string<CharT> & lhs, const Text & rhs) {
        lhs += rhs.mStr;
        return lhs;
    }
    template <typename OtherCharT>
    friend std::basic_string<OtherCharT> & operator += (
            std::basic_string<OtherCharT> & lhs, const Text & rhs) {
        lhs += Text<OtherCharT, EncodingT>(rhs);
        return lhs;
    }

    friend Text operator + (Text lhs, CharT rhs) {
        lhs += rhs;
        return lhs;
    }
    friend Text operator + (CharT lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, Text>::type
    operator + (Text lhs, OtherCharT rhs) {
        lhs += rhs;
        return lhs;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, Text>::type
    operator + (OtherCharT lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    friend Text operator + (Text lhs, const CharT * rhs) {
        lhs += rhs;
        return lhs;
    }
    friend Text operator + (const CharT * lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, Text>::type
    operator + (Text lhs, const OtherCharT * rhs) {
        lhs += rhs;
        return lhs;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, Text>::type
    operator + (const OtherCharT * lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    friend Text operator + (Text lhs, const std::basic_string<CharT> & rhs) {
        lhs += rhs;
        return lhs;
    }
    friend Text operator + (const std::basic_string<CharT> & lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    template <typename OtherCharT>
    friend Text operator + (Text lhs, const std::basic_string<OtherCharT> & rhs) {
        lhs += rhs;
        return lhs;
    }
    template <typename OtherCharT>
    friend Text operator + (const std::basic_string<OtherCharT> & lhs, const Text & rhs) {
        Text text(lhs);
        text += rhs;
        return text;
    }
    friend Text operator + (Text lhs, const Text & rhs) {
        lhs += rhs;
        return lhs;
    }
    template <typename OtherCharT, typename OtherEncodingT>
    friend Text operator + (Text lhs, const Text<OtherCharT, OtherEncodingT> & rhs) {
        lhs += rhs;
        return lhs;
    }

    // deleted
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, Text>::type
    operator + (Text, T) = delete;
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, Text>::type
    operator + (T, const Text &) = delete;
    /**/

    friend bool operator == (const Text & lhs, CharT rhs) {
        return lhs.mStr == rhs;
    }
    friend bool operator == (CharT lhs, const Text & rhs) {
        return lhs == rhs.mStr;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator == (const Text & lhs, OtherCharT rhs) {
        return lhs.mStr == Text(rhs);
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator == (OtherCharT lhs, const Text & rhs) {
        return Text(lhs) == rhs.mStr;
    }
    friend bool operator == (const Text & lhs, const CharT * rhs) {
        return lhs.mStr == rhs;
    }
    friend bool operator == (const CharT * lhs, const Text & rhs) {
        return lhs == rhs.mStr;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator == (const Text & lhs, const OtherCharT * rhs) {
        return lhs.mStr == Text(rhs);
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator == (const OtherCharT * lhs, const Text & rhs) {
        return Text(lhs) == rhs.mStr;
    }
    friend bool operator == (const Text & lhs, const std::basic_string<CharT> & rhs) {
        return lhs.mStr == rhs;
    }
    friend bool operator == (const std::basic_string<CharT> & lhs, const Text & rhs) {
        return lhs == rhs.mStr;
    }
    template <typename OtherCharT>
    friend bool operator == (const Text & lhs, const std::basic_string<OtherCharT> & rhs) {
        return lhs.mStr == Text(rhs);
    }
    template <typename OtherCharT>
    friend bool operator == (const std::basic_string<OtherCharT> & lhs, const Text & rhs) {
        return Text(lhs) == rhs.mStr;
    }
    friend bool operator == (const Text & lhs, const Text & rhs) {
        return lhs.mStr == rhs.mStr;
    }
    template <typename OtherCharT, typename OtherEncodingT>
    friend bool operator == (const Text & lhs, const Text<OtherCharT, OtherEncodingT> & rhs) {
        return lhs.mStr == Text(rhs);
    }

    // deleted
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, bool>::type
    operator == (const Text &, T) = delete;
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, bool>::type
    operator == (T, const Text &) = delete;
    /**/

    friend bool operator != (const Text & lhs, CharT rhs) {
        return lhs.mStr != rhs;
    }
    friend bool operator != (CharT lhs, const Text & rhs) {
        return lhs != rhs.mStr;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator != (const Text & lhs, OtherCharT rhs) {
        return lhs.mStr != Text(rhs);
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator != (OtherCharT lhs, const Text & rhs) {
        return Text(lhs) != rhs.mStr;
    }
    friend bool operator != (const Text & lhs, const CharT * rhs) {
        return lhs.mStr != rhs;
    }
    friend bool operator != (const CharT * lhs, const Text & rhs) {
        return lhs != rhs.mStr;
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator != (const Text & lhs, const OtherCharT * rhs) {
        return lhs.mStr != Text(rhs);
    }
    template <typename OtherCharT>
    friend typename std::enable_if<is_char<OtherCharT>::value, bool>::type
    operator != (const OtherCharT * lhs, const Text & rhs) {
        return Text(lhs) != rhs.mStr;
    }
    friend bool operator != (const Text & lhs, const std::basic_string<CharT> & rhs) {
        return lhs.mStr != rhs;
    }
    friend bool operator != (const std::basic_string<CharT> & lhs, const Text & rhs) {
        return lhs != rhs.mStr;
    }
    template <typename OtherCharT>
    friend bool operator != (const Text & lhs, const std::basic_string<OtherCharT> & rhs) {
        return lhs.mStr != Text(rhs);
    }
    template <typename OtherCharT>
    friend bool operator != (const std::basic_string<OtherCharT> & lhs, const Text & rhs) {
        return Text(lhs) != rhs.mStr;
    }
    friend bool operator != (const Text & lhs, const Text & rhs) {
        return lhs.mStr != rhs.mStr;
    }
    template <typename OtherCharT, typename OtherEncodingT>
    friend bool operator != (const Text & lhs, const Text<OtherCharT, OtherEncodingT> & rhs) {
        return lhs.mStr != Text(rhs);
    }

    // deleted
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, bool>::type
    operator != (const Text &, T) = delete;
    template <typename T>
    friend typename std::enable_if<is_numeric<T>::value || is_bool<T>::value, bool>::type
    operator != (T, const Text &) = delete;
    /**/

    friend std::basic_ostream<CharT> & operator << (
            std::basic_ostream<CharT> & stream, const Text & text) {
        return stream << text.mStr;
    }
    template <typename OtherCharT>
    friend std::basic_ostream<OtherCharT> & operator << (
            std::basic_ostream<OtherCharT> & stream, const Text & text) {
        return stream << Text<OtherCharT, EncodingT>(text);
    }

    operator const CharT * () const {
        return mStr.data();
    }
    const CharT * pStr () const {
        return mStr.data();
    }
    operator const std::basic_string<CharT> & () const {
        return mStr;
    }
    operator std::basic_string<CharT> () {
        return mStr;
    }
    const std::basic_string<CharT> & str() const {
        return mStr;
    }
    std::basic_string<CharT> str() {
        return mStr;
    }
    size_t length() {
        return mStr.length();
    }
    size_t size() {
        return (mStr.length() + 1) * sizeof(CharT);
    }

private:
    template <typename OtherCharT, typename OtherEncodingT>
    class TextCvt: public StringCvt<EncodingT, OtherEncodingT>
    {
    public:
        TextCvt(const std::basic_string<OtherCharT> & str):
            mStr(StringCvt<EncodingT, OtherEncodingT>
                 ::template cvt<CharT, OtherCharT>(str)) {}
    private:
        std::basic_string<CharT> mStr;
        friend class Text;
    };

    template <typename OtherCharT, typename OtherEncodingT>
    Text(TextCvt<OtherCharT, OtherEncodingT> && textCvt):
        SimpleStateful(textCvt.state()), mStr(std::move(textCvt.mStr)) {}

    std::basic_string<CharT> mStr;

    template <typename, typename>
    friend class Text;
};

using CharText = Text<char>;
using WideText = Text<wchar_t>;
using Text16 = Text<char16_t>;
using Text32 = Text<char32_t>;

    //-- template class StringCvt --//

template <typename TargetEncodingT, typename SourceEncodingT>
template <typename TargetCharT, typename SourceCharT>
std::basic_string<TargetCharT>
    StringCvt<TargetEncodingT, SourceEncodingT>
    ::cvt(const std::basic_string<SourceCharT> & source)
{
    std::basic_string<TargetCharT> target;
    Buffer<TargetCharT> targetBuf(1024);

    size_t count;
    const SourceCharT * pSource = source.data();
    do {
        using Base = BufferCvt<TargetEncodingT, SourceEncodingT>;
        count = Base::cvt(targetBuf, pSource);
        if(count < 0) {
            Base::setState(Base::State::invalid);
            return {};
        }
        if(!Base::state(Base::State::valid))
            return {};
        target += std::basic_string<TargetCharT>(targetBuf.pData(), count);
        pSource += count;
    } while(count >= targetBuf.count());

    return target;
}

    //-- class BufferCvt<void> --//

template<>
class BufferCvt<void>: public SimpleStateful
{
public:
    size_t cvt(BufferRef<char> target, const wchar_t * pSource) {
        return wcsrtombs(target.pData, &pSource, target.count, &mCvtState);
    }
    size_t cvt(BufferRef<wchar_t> target, const char * pSource) {
        return mbsrtowcs(target.pData, &pSource, target.count, &mCvtState);
    }

private:
    std::mbstate_t mCvtState;
};

#endif // TEXT_H
