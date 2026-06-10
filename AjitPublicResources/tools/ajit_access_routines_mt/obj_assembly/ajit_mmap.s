	.file	"ajit_mmap.c"
	.section	".text"
	.align 4
	.global initPageTableAllocator
	.type	initPageTableAllocator, #function
	.proc	020
initPageTableAllocator:
	save	%sp, -96, %sp
	mov	%i2, %l1
	mov	%i1, %l0
	cmp	%i3, 0
	be	.L2
	 mov	0, %i2
	mov	0, %i4
	mov	0, %i5
	addcc	%l1, %i5, %o2
.L11:
	mov	0, %o0
	call	__ajit_store_word_to_physical_address__, 0
	 addx	%l0, %i4, %o1
	addcc	%i5, 4, %i5
	addx	%i4, 0, %i4
	cmp	%i2, %i4
	bne	.L2
	 cmp	%i3, %i5
	bgu	.L11
	 addcc	%l1, %i5, %o2
.L2:
	std	%l0, [%i0]
	st	%g0, [%i0+8]
	st	%i3, [%i0+12]
	st	%g0, [%i0+16]
	st	%g0, [%i0+20]
	jmp	%i7+8
	 restore
	.size	initPageTableAllocator, .-initPageTableAllocator
	.align 4
	.global allocatePageTableBlock
	.type	allocatePageTableBlock, #function
	.proc	016
allocatePageTableBlock:
	save	%sp, -96, %sp
	mov	0, %g2
	cmp	%i1, 1
	bgu	.L13
	 mov	256, %g3
	mov	0, %g2
	mov	1024, %g3
.L13:
	ldd	[%i0+16], %o4
	addcc	%o5, %g3, %i5
	ldd	[%i0], %o2
	addx	%o4, %g2, %i4
	addcc	%g3, %o3, %o1
	addx	%g2, %o2, %o0
	cmp	%i4, %o0
	bgu	.L17
	 nop
	be	.L19
	 cmp	%i5, %o1
	std	%o4, [%i2]
.L20:
	std	%i4, [%i0+16]
	jmp	%i7+8
	 restore %g0, 0, %o0
.L19:
	bleu,a	.L20
	 std	%o4, [%i2]
.L17:
	jmp	%i7+8
	 restore %g0, 1, %o0
	.size	allocatePageTableBlock, .-allocatePageTableBlock
	.align 4
	.global ajit_mmap_index_into_table
	.type	ajit_mmap_index_into_table, #function
	.proc	016
ajit_mmap_index_into_table:
	cmp	%o0, 2
	be	.L22
	 mov	%o0, %g1
	cmp	%o0, 3
	be	.L23
	 cmp	%g1, 1
	be	.L26
	 mov	0, %o0
	jmp	%o7+8
	 nop
.L26:
	jmp	%o7+8
	 srl	%o1, 24, %o0
.L23:
	srl	%o1, 12, %o1
	jmp	%o7+8
	 and	%o1, 63, %o0
.L22:
	srl	%o1, 18, %o1
	jmp	%o7+8
	 and	%o1, 63, %o0
	.size	ajit_mmap_index_into_table, .-ajit_mmap_index_into_table
	.align 4
	.global ajit_mmap_is_pte
	.type	ajit_mmap_is_pte, #function
	.proc	014
ajit_mmap_is_pte:
	and	%o0, 3, %o0
	xor	%o0, 2, %o0
	cmp	%g0, %o0
	jmp	%o7+8
	 subx	%g0, -1, %o0
	.size	ajit_mmap_is_pte, .-ajit_mmap_is_pte
	.align 4
	.global ajit_mmap_is_ptd
	.type	ajit_mmap_is_ptd, #function
	.proc	014
ajit_mmap_is_ptd:
	and	%o0, 3, %o0
	xor	%o0, 1, %o0
	cmp	%g0, %o0
	jmp	%o7+8
	 subx	%g0, -1, %o0
	.size	ajit_mmap_is_ptd, .-ajit_mmap_is_ptd
	.align 4
	.global ajit_mmap_get_phy_addr_from_ptd
	.type	ajit_mmap_get_phy_addr_from_ptd, #function
	.proc	017
ajit_mmap_get_phy_addr_from_ptd:
	and	%o0, -4, %g1
	sll	%o2, 2, %o2
	sll	%g1, 4, %g1
	srl	%o0, 28, %o0
	jmp	%o7+8
	 or	%o2, %g1, %o1
	.size	ajit_mmap_get_phy_addr_from_ptd, .-ajit_mmap_get_phy_addr_from_ptd
	.align 4
	.global ajit_mmap_get_phy_addr_from_pte
	.type	ajit_mmap_get_phy_addr_from_pte, #function
	.proc	017
ajit_mmap_get_phy_addr_from_pte:
	cmp	%o1, 3
	be	.L37
	 cmp	%o1, 2
	be	.L38
	 cmp	%o1, 1
	bne	.L39
	 srl	%o0, 8, %o1
	sethi	%hi(-16777216), %g1
	andn	%o2, %g1, %o2
	srl	%o0, 28, %o0
	sll	%o1, 12, %o1
	jmp	%o7+8
	 or	%o2, %o1, %o1
.L38:
	sethi	%hi(-16384), %g1
	andn	%o2, %g1, %o2
	srl	%o0, 8, %o1
.L39:
	srl	%o0, 28, %o0
	sll	%o1, 12, %o1
	jmp	%o7+8
	 or	%o2, %o1, %o1
.L37:
	and	%o2, 4095, %o2
	srl	%o0, 8, %o1
	srl	%o0, 28, %o0
	sll	%o1, 12, %o1
	jmp	%o7+8
	 or	%o2, %o1, %o1
	.size	ajit_mmap_get_phy_addr_from_pte, .-ajit_mmap_get_phy_addr_from_pte
	.align 4
	.global ajit_mmap_make_ptd
	.type	ajit_mmap_make_ptd, #function
	.proc	016
ajit_mmap_make_ptd:
	sll	%o1, 26, %o1
	srl	%o2, 6, %o2
	or	%o1, %o2, %o2
	sll	%o2, 2, %o2
	jmp	%o7+8
	 or	%o2, 1, %o0
	.size	ajit_mmap_make_ptd, .-ajit_mmap_make_ptd
	.align 4
	.global ajit_mmap_make_pte
	.type	ajit_mmap_make_pte, #function
	.proc	016
ajit_mmap_make_pte:
	sll	%o3, 20, %o3
	sll	%o1, 2, %o1
	srl	%o4, 12, %o4
	or	%o3, %o4, %o4
	sll	%o4, 8, %o4
	or	%o4, %o1, %o4
	jmp	%o7+8
	 or	%o4, 2, %o0
	.size	ajit_mmap_make_pte, .-ajit_mmap_make_pte
	.align 4
	.global ajit_lookup_mmap
	.type	ajit_lookup_mmap, #function
	.proc	04
ajit_lookup_mmap:
	save	%sp, -96, %sp
	stb	%g0, [%i4]
	st	%g0, [%i5]
	st	%g0, [%i5+4]
	ld	[%fp+92], %l1
	sll	%i2, 2, %i2
	st	%g0, [%l1]
	st	%g0, [%l1+4]
	addcc	%i1, %i2, %g3
	ld	[%fp+96], %l0
	addx	%i0, 0, %g2
	st	%g0, [%l0]
	mov	%g2, %i0
	mov	%g3, %i1
	mov	0, %i2
.L48:
	mov	%i0, %o0
	call	__ajit_load_word_from_physical_address__, 0
	 mov	%i1, %o1
	stb	%i2, [%i4]
	std	%i0, [%l1]
	and	%o0, 3, %g3
	cmp	%g3, 1
	bne	.L43
	 st	%o0, [%l0]
.L56:
	add	%i2, 1, %g2
	and	%g2, 0xff, %g1
	cmp	%g1, 2
	be	.L44
	 mov	%g2, %i2
	cmp	%g1, 3
	be	.L45
	 cmp	%g1, 1
	be	.L55
	 mov	1, %i0
.L60:
	jmp	%i7+8
	 restore
.L55:
	and	%o0, -4, %g2
	srl	%i3, 24, %g1
	sll	%g2, 4, %g2
	sll	%g1, 2, %g1
	srl	%o0, 28, %i0
	or	%g1, %g2, %i1
	mov	%i0, %o0
	call	__ajit_load_word_from_physical_address__, 0
	 mov	%i1, %o1
	stb	%i2, [%i4]
	std	%i0, [%l1]
	and	%o0, 3, %g3
	cmp	%g3, 1
	be	.L56
	 st	%o0, [%l0]
.L43:
	cmp	%g3, 2
	bne	.L60
	 mov	1, %i0
	and	%i2, 0xff, %g1
	cmp	%g1, 3
	be	.L57
	 cmp	%g1, 2
	be	.L58
	 cmp	%g1, 1
	bne	.L59
	 srl	%o0, 8, %g1
	sethi	%hi(-16777216), %g1
	andn	%i3, %g1, %i3
.L50:
	srl	%o0, 8, %g1
.L59:
	srl	%o0, 28, %o0
	sll	%g1, 12, %g1
	st	%o0, [%i5]
	or	%g1, %i3, %g1
	st	%g1, [%i5+4]
	jmp	%i7+8
	 restore %g0, 0, %o0
.L45:
	and	%o0, -4, %g2
	srl	%i3, 10, %g1
	sll	%g2, 4, %g2
	and	%g1, 252, %g1
	srl	%o0, 28, %i0
	b	.L48
	 or	%g1, %g2, %i1
.L44:
	and	%o0, -4, %g2
	srl	%i3, 16, %g1
	sll	%g2, 4, %g2
	and	%g1, 252, %g1
	srl	%o0, 28, %i0
	b	.L48
	 or	%g1, %g2, %i1
.L57:
	b	.L50
	 and	%i3, 4095, %i3
.L58:
	sethi	%hi(-16384), %g1
	b	.L50
	 andn	%i3, %g1, %i3
	.size	ajit_lookup_mmap, .-ajit_lookup_mmap
	.align 4
	.global ajit_mmap_operation
	.type	ajit_mmap_operation, #function
	.proc	04
ajit_mmap_operation:
	save	%sp, -128, %sp
	add	%fp, -20, %g1
	st	%g1, [%sp+96]
	add	%fp, -16, %g1
	mov	%i1, %o0
	mov	%i2, %o1
	ld	[%fp+100], %l0
	ld	[%fp+104], %i2
	ldub	[%fp+95], %i1
	st	%g1, [%sp+92]
	add	%fp, -8, %o5
	add	%fp, -21, %o4
	ld	[%fp+96], %o3
	call	ajit_lookup_mmap, 0
	 mov	%i4, %o2
	cmp	%o0, 0
	bne	.L62
	 cmp	%i3, 1
	be	.L84
	 cmp	%i3, 2
	be	.L64
	 mov	0, %o0
	ld	[%fp-20], %o0
.L64:
	ld	[%fp-16], %o1
	call	__ajit_store_word_to_physical_address__, 0
	 ld	[%fp-12], %o2
.L65:
	jmp	%i7+8
	 restore %g0, 0, %o0
.L62:
	bne	.L65
	 ldub	[%fp-21], %g1
	and	%g1, 0xff, %g2
	cmp	%g2, %i5
	bgu	.L65
	 ldd	[%fp-16], %l4
	add	%g1, 1, %l6
.L88:
	mov	0, %g2
	and	%l6, 0xff, %i3
	cmp	%i3, 1
	bleu	.L68
	 mov	1024, %g3
	mov	0, %g2
	mov	256, %g3
.L68:
	ldd	[%i0+16], %l2
	addcc	%l3, %g3, %o5
	ldd	[%i0], %o0
	addx	%l2, %g2, %o4
	addcc	%g3, %o1, %o3
	addx	%g2, %o0, %o2
	cmp	%o4, %o2
	bgu	.L79
	 nop
	bne	.L87
	 and	%g1, 0xff, %g1
	cmp	%o5, %o3
	bgu	.L79
	 nop
.L87:
	cmp	%g1, %i5
	be	.L85
	 std	%o4, [%i0+16]
	sll	%l2, 26, %g1
	srl	%l3, 6, %o0
	or	%g1, %o0, %o0
	sll	%o0, 2, %o0
	or	%o0, 1, %o0
.L72:
	st	%o0, [%fp-20]
	mov	%l4, %o1
	call	__ajit_store_word_to_physical_address__, 0
	 mov	%l5, %o2
	cmp	%i3, 2
	be	.L73
	 cmp	%i3, 3
	be	.L74
	 cmp	%i3, 1
	mov	%l2, %l4
	be	.L86
	 mov	%l3, %l5
.L75:
	cmp	%i3, %i5
	bgu	.L65
	 mov	%l6, %g1
	b	.L88
	 add	%g1, 1, %l6
.L84:
	sll	%l0, 20, %l0
	srl	%i2, 12, %i2
	sll	%i1, 2, %i1
	or	%l0, %i2, %i2
	sll	%i2, 8, %o0
	or	%o0, %i1, %o0
	b	.L64
	 or	%o0, 2, %o0
.L79:
	jmp	%i7+8
	 restore %g0, 1, %o0
.L73:
	ld	[%fp+96], %g1
	srl	%g1, 16, %g1
	and	%g1, 252, %g1
	addcc	%l3, %g1, %l5
	b	.L75
	 addx	%l2, 0, %l4
.L74:
	ld	[%fp+96], %g1
	srl	%g1, 10, %g1
	and	%g1, 252, %g1
	addcc	%l3, %g1, %l5
	b	.L75
	 addx	%l2, 0, %l4
.L86:
	ld	[%fp+96], %g1
	srl	%g1, 24, %g1
	sll	%g1, 2, %g1
	addcc	%l3, %g1, %l5
	b	.L75
	 addx	%l2, 0, %l4
.L85:
	sll	%l0, 20, %g3
	srl	%i2, 12, %g1
	sll	%i1, 2, %g2
	or	%g3, %g1, %g1
	sll	%g1, 8, %o0
	or	%o0, %g2, %o0
	b	.L72
	 or	%o0, 2, %o0
	.size	ajit_mmap_operation, .-ajit_mmap_operation
	.ident	"GCC: (GNU) 12.1.1 20220507 (Red Hat Cross 12.1.1-2)"
	.section	.note.GNU-stack,"",@progbits
