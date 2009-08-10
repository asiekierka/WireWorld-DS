@ === void memcpy32(void *dst, const void *src, u32 wdcount) CODE_IN_IWRAM; =============
@ r0, r1: dst, src
@ r2: wdcount, then wdcount>>3
@ r3-r10: data buffer
@ r12: wdn&7
    .align  2
    .code   32
    .global memcpy32
memcpy32:
    and     r12, r2, #7     @ r12= residual word count
    movs    r2, r2, lsr #3  @ r2=block count
    beq     .Lres_cpy32
    push    {r4-r10}
    @ Copy 32byte chunks with 8fold xxmia
    @ r2 in [1,inf>
.Lmain_cpy32:
        ldmia   r1!, {r3-r10}   
        stmia   r0!, {r3-r10}
        subs    r2, #1
        bne     .Lmain_cpy32
    pop     {r4-r10}
    @ And the residual 0-7 words. r12 in [0,7]
.Lres_cpy32:
        subs    r12, #1
        ldrcs   r3, [r1], #4
        strcs   r3, [r0], #4
        bcs     .Lres_cpy32
    bx  lr

@ === void memset32(void *dst, u32 src, u32 wdn); =====================
	.align	2
	.code	32
	.global	memset32
memset32:
	and		r12, r2, #7
	movs	r2, r2, lsr #3
	beq		.Lres_set32
	push	{r4-r10}
	@ set 32byte chunks with 8fold xxmia
	mov		r3, r1
	mov		r4, r1
	mov		r5, r1
	mov		r6, r1
	mov		r7, r1
	mov		r8, r1
	mov		r9, r1
	mov		r10, r1
.Lmain_set32:
		stmia	r0!, {r3-r10}
		subs	r2, r2, #1
		bhi		.Lmain_set32
	pop		{r4-r10}
	@ residual 0-7 words
.Lres_set32:
		subs	r12, r12, #1
		stmcsia	r0!, {r1}
		bhi		.Lres_set32
	bx	lr
