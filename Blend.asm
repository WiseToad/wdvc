format MS COFF

section '.data?' writeable

section '.data'

section '.text' executable

macro initBlend
{
	pxor	mm7, mm7
	pxor	mm6, mm6
	pcmpeqd mm6, mm6
	punpcklbw mm6, mm7	; mm6 = 00 ff 00 ff 00 ff 00 ff
	movq	mm7, mm6
	psllq	mm7, 48		; mm7 = 00 ff 00 00 00 00 00 00
	movq	mm5, mm6
	psrlw	mm5, 7		; mm5 = 00 01 00 01 00 01 00 01
}

macro closeBlend
{
	emms
}

macro initColor
{
	punpcklbw mm0, mm7	; mm0 = 00 sA 00 sB 00 sG 00 sR
	movq	mm1, mm0
	punpckhwd mm1, mm1
	punpckhwd mm1, mm1	; mm1 = 00 sA 00 sA 00 sA 00 sA
	movq	mm2, mm1
	pxor	mm2, mm6	; mm2 = 00~sA 00~sA 00~sA 00~sA
	psrlq	mm1, 16
	por	mm1, mm7	; mm1 = 00 ff 00 sA 00 sA 00 sA
}

macro blendColor
{
	movd	mm3, [edi]
	punpcklbw mm3, mm7	; mm3 = 00 dA 00 dB 00 dG 00 dR

	movq	mm4, mm0
	pmullw	mm4, mm1
	pmullw	mm3, mm2
	paddw	mm4, mm3	; mm4 = dABGR * 255

	paddw	mm4, mm5
	movq	mm3, mm4
	psrlw	mm3, 8
	paddw	mm4, mm3
	psrlw	mm4, 8		; mm4 = dABGR

	packuswb mm4, mm4
	movd	[edi], mm4
}

; void blendImage(Pixel *aPixels, int aWidth, int aHeight, int aPitch, Pixel *aImage, int aImagePitch);
public blendImage as '_blendImage'
blendImage:
	label	.pTargetPixels dword at ebp + 8
	label	.targetPitch dword at ebp + 12
	label	.width dword at ebp + 16
	label	.height dword at ebp + 20
	label	.pSourcePixels dword at ebp + 24
	label	.sourcePitch dword at ebp + 28

	label	.targetDelta dword at ebp - 4
	label	.sourceDelta dword at ebp - 8

	push	ebp
	mov	ebp, esp
	sub	esp, 8
	push	esi
	push	edi

	initBlend

	mov	edi, [.pTargetPixels]
	mov	edx, [.targetPitch]
	mov	eax, [.width]
	sal	eax, 2
	sub	edx, eax
	mov	[.targetDelta], edx
	mov	esi, [.pSourcePixels]
	mov	edx, [.sourcePitch]
	mov	eax, [.width]
	sal	eax, 2
	sub	edx, eax
	mov	[.sourceDelta], edx
	mov	edx, [.height]
.@loopRow:
	mov	ecx, [.width]
.@pack8:
	sub	ecx, 8
	jc	.@pack4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	jmp	.@pack8
.@pack4:
	test	ecx, 4
	jz	.@pack2
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
.@pack2:
	test	ecx, 2
	jz	.@pack1
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
.@pack1:
	test	ecx, 1
	jz	.@loopRow@end
	movd	mm0, [esi]
	initColor
	blendColor
	add	esi, 4
	add	edi, 4
.@loopRow@end:
	add	edi, [.targetDelta]
	add	esi, [.sourceDelta]
	dec	edx
	jnz	.@loopRow

	closeBlend

	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp
	ret
