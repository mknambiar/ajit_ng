	.file	"ajit_spi_flash.c"
	.section	".text"
	.align 4
	.global ajit_spi_transfer_is_in_progress
	.type	ajit_spi_transfer_is_in_progress, #function
	.proc	016
ajit_spi_transfer_is_in_progress:
	ld	[%o0+8], %o0
	jmp	%o7+8
	 and	%o0, 1, %o0
	.size	ajit_spi_transfer_is_in_progress, .-ajit_spi_transfer_is_in_progress
	.align 4
	.global ajit_spi_flash_send_byte
	.type	ajit_spi_flash_send_byte, #function
	.proc	020
ajit_spi_flash_send_byte:
	save	%sp, -96, %sp
	sll	%i3, 1, %i3
	sll	%i1, 4, %i1
	and	%i3, 2, %i3
	and	%i1, 112, %i1
	st	%i2, [%i0]
	or	%i3, %i1, %i3
	add	%i0, 8, %i5
	or	%i3, 1, %i3
	st	%i3, [%i0+8]
.L4:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L4
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_send_byte, .-ajit_spi_flash_send_byte
	.align 4
	.global ajit_spi_flash_receive_byte
	.type	ajit_spi_flash_receive_byte, #function
	.proc	014
ajit_spi_flash_receive_byte:
	save	%sp, -96, %sp
	sll	%i2, 1, %i2
	sll	%i1, 4, %i1
	and	%i2, 2, %i2
	and	%i1, 112, %i1
	st	%g0, [%i0]
	or	%i2, %i1, %i2
	add	%i0, 8, %i5
	or	%i2, 1, %i2
	st	%i2, [%i0+8]
.L7:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L7
	 nop
	ldub	[%i0+3], %i0
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_receive_byte, .-ajit_spi_flash_receive_byte
	.align 4
	.global ajit_spi_flash_reset
	.type	ajit_spi_flash_reset, #function
	.proc	020
ajit_spi_flash_reset:
	save	%sp, -96, %sp
	mov	102, %g1
	sll	%i1, 4, %i1
	st	%g1, [%i0]
	and	%i1, 112, %i1
	add	%i0, 8, %i5
	or	%i1, 3, %i1
	st	%i1, [%i0+8]
.L10:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L10
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_reset, .-ajit_spi_flash_reset
	.align 4
	.global ajit_spi_flash_memory_reset
	.type	ajit_spi_flash_memory_reset, #function
	.proc	020
ajit_spi_flash_memory_reset:
	save	%sp, -96, %sp
	mov	153, %g1
	sll	%i1, 4, %i1
	st	%g1, [%i0]
	and	%i1, 112, %i1
	add	%i0, 8, %i5
	or	%i1, 3, %i1
	st	%i1, [%i0+8]
.L13:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L13
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_memory_reset, .-ajit_spi_flash_memory_reset
	.align 4
	.global ajit_spi_configure
	.type	ajit_spi_configure, #function
	.proc	020
ajit_spi_configure:
	sll	%o1, 6, %o1
	sll	%o2, 4, %o2
	and	%o1, 255, %o1
	or	%o2, %o3, %o3
	or	%o1, %o3, %o1
	jmp	%o7+8
	 st	%o1, [%o0+12]
	.size	ajit_spi_configure, .-ajit_spi_configure
	.align 4
	.global ajit_spi_flash_is_busy
	.type	ajit_spi_flash_is_busy, #function
	.proc	016
ajit_spi_flash_is_busy:
	save	%sp, -96, %sp
	mov	5, %g1
	sll	%i1, 4, %i1
	st	%g1, [%i0]
	and	%i1, 112, %i1
	add	%i0, 8, %i5
	or	%i1, 1, %g1
	st	%g1, [%i0+8]
.L17:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L17
	 nop
	or	%i1, 3, %i1
	st	%g0, [%i0]
	st	%i1, [%i0+8]
.L18:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L18
	 nop
	ld	[%i0], %i0
	and	%i0, 1, %i0
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_is_busy, .-ajit_spi_flash_is_busy
	.align 4
	.global ajit_spi_flash_generic_erase
	.type	ajit_spi_flash_generic_erase, #function
	.proc	020
ajit_spi_flash_generic_erase:
	save	%sp, -96, %sp
	mov	6, %g1
	sll	%i1, 4, %i4
	st	%g1, [%i0]
	and	%i4, 112, %i4
	add	%i0, 8, %i5
	or	%i4, 3, %i1
	st	%i1, [%i0+8]
.L22:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L22
	 nop
	or	%i4, 1, %i4
	st	%i2, [%i0]
	st	%i4, [%i0+8]
.L23:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L23
	 srl	%i3, 16, %g1
	st	%i4, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L24:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L24
	 srl	%i3, 8, %g1
	st	%i4, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L25:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L25
	 nop
	and	%i3, 255, %i3
	st	%i1, [%i0+8]
	st	%i3, [%i0]
.L26:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L26
	 mov	5, %i2
	mov	1, %i3
	mov	3, %i4
	st	%i2, [%i0]
.L41:
	st	%i3, [%i5]
.L28:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L28
	 nop
	st	%g0, [%i0]
	st	%i4, [%i5]
.L29:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L29
	 nop
	ld	[%i0], %g1
	andcc	%g1, 1, %g0
	be	.L40
	 mov	4, %g1
	call	__ajit_sleep__, 0
	 mov	256, %o0
	b	.L41
	 st	%i2, [%i0]
.L40:
	st	%i1, [%i0+8]
	st	%g1, [%i0]
.L31:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L31
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_generic_erase, .-ajit_spi_flash_generic_erase
	.align 4
	.global ajit_spi_flash_sector_erase
	.type	ajit_spi_flash_sector_erase, #function
	.proc	020
ajit_spi_flash_sector_erase:
	mov	%o2, %o3
	mov	216, %o2
	or	%o7, %g0, %g1
	call	ajit_spi_flash_generic_erase, 0
	 or	%g1, %g0, %o7
	.size	ajit_spi_flash_sector_erase, .-ajit_spi_flash_sector_erase
	.align 4
	.global ajit_spi_flash_read
	.type	ajit_spi_flash_read, #function
	.proc	020
ajit_spi_flash_read:
	save	%sp, -96, %sp
	mov	3, %g1
	sll	%i1, 4, %i1
	st	%g1, [%i0]
	and	%i1, 112, %i1
	add	%i0, 8, %i5
	or	%i1, 1, %l0
	st	%l0, [%i0+8]
.L44:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L44
	 srl	%i2, 16, %g1
	st	%l0, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L45:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L45
	 srl	%i2, 8, %g1
	st	%l0, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L46:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L46
	 nop
	and	%i2, 255, %i2
	st	%l0, [%i0+8]
	st	%i2, [%i0]
.L47:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L47
	 add	%i4, -1, %l0
	cmp	%i4, 0
	be	.L60
	 mov	0, %i2
	xor	%l0, %i2, %g1
.L59:
	st	%g0, [%i0]
	cmp	%g0, %g1
	subx	%g0, -1, %g1
	add	%g1, %g1, %g1
	or	%g1, %i1, %g1
	or	%g1, 1, %g1
	st	%g1, [%i5]
.L49:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L49
	 nop
	ld	[%i0], %g1
	stb	%g1, [%i3+%i2]
	add	%i2, 1, %i2
	cmp	%i2, %i4
	bne	.L59
	 xor	%l0, %i2, %g1
.L60:
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_read, .-ajit_spi_flash_read
	.align 4
	.global ajit_spi_flash_write
	.type	ajit_spi_flash_write, #function
	.proc	020
ajit_spi_flash_write:
	save	%sp, -96, %sp
	sll	%i1, 4, %i1
	mov	6, %g1
	and	%i1, 112, %l0
	st	%g1, [%i0]
	or	%l0, 3, %i1
	add	%i0, 8, %i5
	st	%i1, [%i0+8]
.L62:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L62
	 or	%l0, 1, %l1
	mov	2, %g1
	st	%l1, [%i0+8]
	st	%g1, [%i0]
.L63:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L63
	 srl	%i2, 16, %g1
	st	%l1, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L64:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L64
	 srl	%i2, 8, %g1
	st	%l1, [%i0+8]
	and	%g1, 255, %g1
	st	%g1, [%i0]
.L65:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L65
	 nop
	and	%i2, 255, %i2
	st	%l1, [%i0+8]
	st	%i2, [%i0]
.L66:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L66
	 add	%i4, -1, %l1
	cmp	%i4, 0
	be	.L71
	 mov	0, %i2
	ldub	[%i3+%i2], %g1
.L89:
	st	%g1, [%i0]
	xor	%l1, %i2, %g1
	cmp	%g0, %g1
	subx	%g0, -1, %g1
	add	%g1, %g1, %g1
	or	%g1, %l0, %g1
	or	%g1, 1, %g1
	st	%g1, [%i5]
.L69:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L69
	 nop
	add	%i2, 1, %i2
	cmp	%i2, %i4
	bne,a	.L89
	 ldub	[%i3+%i2], %g1
.L71:
	mov	5, %i2
	mov	1, %i3
	mov	3, %i4
	st	%i2, [%i0]
.L88:
	st	%i3, [%i5]
.L72:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L72
	 nop
	st	%g0, [%i0]
	st	%i4, [%i5]
.L73:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L73
	 nop
	ld	[%i0], %g1
	andcc	%g1, 1, %g0
	be	.L87
	 mov	4, %g1
	call	__ajit_sleep__, 0
	 mov	256, %o0
	b	.L88
	 st	%i2, [%i0]
.L87:
	st	%i1, [%i0+8]
	st	%g1, [%i0]
.L75:
	call	__ajit_sleep__, 0
	 mov	64, %o0
	ld	[%i5], %g1
	andcc	%g1, 1, %g0
	bne	.L75
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_spi_flash_write, .-ajit_spi_flash_write
	.align 4
	.global ajit_spi_get_register
	.type	ajit_spi_get_register, #function
	.proc	014
ajit_spi_get_register:
	sll	%o1, 2, %o1
	ld	[%o1+%o0], %o0
	jmp	%o7+8
	 and	%o0, 0xff, %o0
	.size	ajit_spi_get_register, .-ajit_spi_get_register
	.align 4
	.global ajit_spi_set_register
	.type	ajit_spi_set_register, #function
	.proc	020
ajit_spi_set_register:
	sll	%o1, 2, %o1
	jmp	%o7+8
	 st	%o2, [%o1+%o0]
	.size	ajit_spi_set_register, .-ajit_spi_set_register
	.ident	"GCC: (GNU) 12.1.1 20220507 (Red Hat Cross 12.1.1-2)"
	.section	.note.GNU-stack,"",@progbits
