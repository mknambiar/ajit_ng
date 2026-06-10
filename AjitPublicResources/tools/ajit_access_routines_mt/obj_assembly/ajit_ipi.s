	.file	"ajit_ipi.c"
	.section	".text"
	.align 4
	.global __ajit_get_ipi_message_pointer__
	.type	__ajit_get_ipi_message_pointer__, #function
	.proc	0110
__ajit_get_ipi_message_pointer__:
	sll	%o1, 1, %o1
	add	%o0, 8, %o0
	add	%o1, %o2, %o1
	sll	%o1, 3, %o1
	jmp	%o7+8
	 add	%o1, %o0, %o0
	.size	__ajit_get_ipi_message_pointer__, .-__ajit_get_ipi_message_pointer__
	.align 4
	.global __ajit_set_ipi_interrupt__
	.type	__ajit_set_ipi_interrupt__, #function
	.proc	04
__ajit_set_ipi_interrupt__:
	save	%sp, -96, %sp
	ld	[%i0+4], %o7
	ld	[%i0], %g4
	add	%i3, %i3, %g2
	mov	%i0, %g3
	add	%g2, %i4, %g2
	add	%i0, 8, %i0
	sll	%g2, 3, %g1
	sll	%i2, 16, %i2
	sll	%i1, 24, %i1
	sll	%i3, 8, %i3
	or	%i1, %i2, %i1
	or	%i1, %i3, %i1
	or	%i1, %i4, %i1
	st	%i1, [%g1+%i0]
	add	%g1, %i0, %g1
	st	%i5, [%g1+4]
	and	%o7, %g4, %g4
	mov	1, %g1
	sll	%g1, %g2, %g1
	andcc	%g4, %g1, %g0
	bne	.L6
	 mov	1, %i0
	or	%g1, %o7, %g1
	mov	0, %i0
	st	%g1, [%g3+4]
.L6:
	jmp	%i7+8
	 restore
	.size	__ajit_set_ipi_interrupt__, .-__ajit_set_ipi_interrupt__
	.align 4
	.global __ajit_enable_ipi_interrupt__
	.type	__ajit_enable_ipi_interrupt__, #function
	.proc	04
__ajit_enable_ipi_interrupt__:
	sll	%o1, 1, %o1
	ld	[%o0], %g2
	add	%o1, %o2, %o2
	mov	1, %g1
	sll	%g1, %o2, %o2
	andcc	%o2, %g2, %g0
	bne	.L7
	 or	%o2, %g2, %o2
	mov	0, %g1
	st	%o2, [%o0]
.L7:
	jmp	%o7+8
	 mov	%g1, %o0
	.size	__ajit_enable_ipi_interrupt__, .-__ajit_enable_ipi_interrupt__
	.align 4
	.global __ajit_disable_ipi_interrupt__
	.type	__ajit_disable_ipi_interrupt__, #function
	.proc	04
__ajit_disable_ipi_interrupt__:
	sll	%o1, 1, %o1
	ld	[%o0], %g2
	add	%o1, %o2, %o2
	mov	1, %g1
	sll	%g1, %o2, %o2
	andcc	%o2, %g2, %g0
	be	.L10
	 andn	%g2, %o2, %o2
	mov	0, %g1
	st	%o2, [%o0]
.L10:
	jmp	%o7+8
	 mov	%g1, %o0
	.size	__ajit_disable_ipi_interrupt__, .-__ajit_disable_ipi_interrupt__
	.align 4
	.global __ajit_clear_ipi_interrupt__
	.type	__ajit_clear_ipi_interrupt__, #function
	.proc	04
__ajit_clear_ipi_interrupt__:
	ld	[%o0+4], %g3
	mov	%o0, %g2
	sll	%o1, 1, %o1
	mov	0, %o0
	add	%o1, %o2, %o2
	mov	1, %g1
	sll	%g1, %o2, %g1
	andn	%g3, %g1, %g1
	jmp	%o7+8
	 st	%g1, [%g2+4]
	.size	__ajit_clear_ipi_interrupt__, .-__ajit_clear_ipi_interrupt__
	.align 4
	.global __ajit_set_ipi_mask_register__
	.type	__ajit_set_ipi_mask_register__, #function
	.proc	020
__ajit_set_ipi_mask_register__:
	jmp	%o7+8
	 st	%o1, [%o0]
	.size	__ajit_set_ipi_mask_register__, .-__ajit_set_ipi_mask_register__
	.align 4
	.global __ajit_get_ipi_mask_register__
	.type	__ajit_get_ipi_mask_register__, #function
	.proc	016
__ajit_get_ipi_mask_register__:
	jmp	%o7+8
	 ld	[%o0], %o0
	.size	__ajit_get_ipi_mask_register__, .-__ajit_get_ipi_mask_register__
	.align 4
	.global __ajit_set_ipi_value_register__
	.type	__ajit_set_ipi_value_register__, #function
	.proc	020
__ajit_set_ipi_value_register__:
	jmp	%o7+8
	 st	%o1, [%o0+4]
	.size	__ajit_set_ipi_value_register__, .-__ajit_set_ipi_value_register__
	.align 4
	.global __ajit_get_ipi_value_register__
	.type	__ajit_get_ipi_value_register__, #function
	.proc	016
__ajit_get_ipi_value_register__:
	jmp	%o7+8
	 ld	[%o0+4], %o0
	.size	__ajit_get_ipi_value_register__, .-__ajit_get_ipi_value_register__
	.align 4
	.global __ajit_read_ipi_info__
	.type	__ajit_read_ipi_info__, #function
	.proc	020
__ajit_read_ipi_info__:
	sll	%o1, 1, %o1
	cmp	%o5, 0
	be	.L20
	 add	%o1, %o2, %o2
	sll	%o2, 3, %g1
	add	%o0, 8, %g2
	add	%g1, %g2, %g1
	st	%g1, [%o5]
.L20:
	cmp	%o3, 0
	be	.L32
	 cmp	%o4, 0
	ld	[%o0], %g1
	srl	%g1, %o2, %g1
	and	%g1, 1, %g1
	st	%g1, [%o3]
.L32:
	be	.L33
	 nop
	ld	[%o0+4], %g1
	srl	%g1, %o2, %g1
	and	%g1, 1, %g1
	st	%g1, [%o4]
.L33:
	jmp	%o7+8
	 nop
	.size	__ajit_read_ipi_info__, .-__ajit_read_ipi_info__
	.align 4
	.global __ajit_acquire_ipi_lock__
	.type	__ajit_acquire_ipi_lock__, #function
	.proc	020
__ajit_acquire_ipi_lock__:
	add	%o0, 72, %o0
	or	%o7, %g0, %g1
	call	acquire_mutex_using_swap, 0
	 or	%g1, %g0, %o7
	.size	__ajit_acquire_ipi_lock__, .-__ajit_acquire_ipi_lock__
	.align 4
	.global __ajit_release_ipi_lock__
	.type	__ajit_release_ipi_lock__, #function
	.proc	020
__ajit_release_ipi_lock__:
	jmp	%o7+8
	 st	%g0, [%o0+72]
	.size	__ajit_release_ipi_lock__, .-__ajit_release_ipi_lock__
	.align 4
	.global __ajit_ipi_init__
	.type	__ajit_ipi_init__, #function
	.proc	020
__ajit_ipi_init__:
	st	%g0, [%o0]
	st	%g0, [%o0+4]
	jmp	%o7+8
	 st	%g0, [%o0+72]
	.size	__ajit_ipi_init__, .-__ajit_ipi_init__
	.ident	"GCC: (GNU) 12.1.1 20220507 (Red Hat Cross 12.1.1-2)"
	.section	.note.GNU-stack,"",@progbits
