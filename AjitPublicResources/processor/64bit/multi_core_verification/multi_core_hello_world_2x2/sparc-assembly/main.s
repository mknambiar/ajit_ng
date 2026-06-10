	.file	"main.c"
	.section	".text"
.Ltext0:
	.cfi_sections	.debug_frame
	.file 0 "/home/Manoj/Documents/ajit-toolchain-marshal/AjitPublicResources/processor/64bit/multi_core_verification/multi_core_hello_world_2x2" "main.c"
	.global C0
	.section	".bss"
	.align 4
	.type	C0, #object
	.size	C0, 4
C0:
	.skip	4
	.global C1
	.align 4
	.type	C1, #object
	.size	C1, 4
C1:
	.skip	4
	.global C2
	.align 4
	.type	C2, #object
	.size	C2, 4
C2:
	.skip	4
	.global C3
	.align 4
	.type	C3, #object
	.size	C3, 4
C3:
	.skip	4
	.section	".text"
	.align 4
	.global main_0
	.type	main_0, #function
	.proc	020
main_0:
.LFB0:
	.file 1 "main.c"
	.loc 1 8 1
	.cfi_startproc
	save	%sp, -96, %sp
	.cfi_window_save
	.cfi_register 15, 31
	.cfi_def_cfa_register 30
	.loc 1 9 5
	sethi	%hi(C0), %g1
	or	%g1, %lo(C0), %g1
	mov	1, %g2
	st	%g2, [%g1]
	.loc 1 10 1
	nop
	restore
	jmp	%o7+8
	 nop
	.cfi_endproc
.LFE0:
	.size	main_0, .-main_0
	.align 4
	.global main_1
	.type	main_1, #function
	.proc	020
main_1:
.LFB1:
	.loc 1 13 1
	.cfi_startproc
	save	%sp, -96, %sp
	.cfi_window_save
	.cfi_register 15, 31
	.cfi_def_cfa_register 30
	.loc 1 14 5
	sethi	%hi(C1), %g1
	or	%g1, %lo(C1), %g1
	mov	1, %g2
	st	%g2, [%g1]
	.loc 1 15 1
	nop
	restore
	jmp	%o7+8
	 nop
	.cfi_endproc
.LFE1:
	.size	main_1, .-main_1
	.align 4
	.global main_2
	.type	main_2, #function
	.proc	020
main_2:
.LFB2:
	.loc 1 18 1
	.cfi_startproc
	save	%sp, -96, %sp
	.cfi_window_save
	.cfi_register 15, 31
	.cfi_def_cfa_register 30
	.loc 1 19 5
	sethi	%hi(C2), %g1
	or	%g1, %lo(C2), %g1
	mov	1, %g2
	st	%g2, [%g1]
	.loc 1 20 1
	nop
	restore
	jmp	%o7+8
	 nop
	.cfi_endproc
.LFE2:
	.size	main_2, .-main_2
	.align 4
	.global main_3
	.type	main_3, #function
	.proc	020
main_3:
.LFB3:
	.loc 1 23 1
	.cfi_startproc
	save	%sp, -96, %sp
	.cfi_window_save
	.cfi_register 15, 31
	.cfi_def_cfa_register 30
	.loc 1 24 5
	sethi	%hi(C3), %g1
	or	%g1, %lo(C3), %g1
	mov	1, %g2
	st	%g2, [%g1]
	.loc 1 25 1
	nop
	restore
	jmp	%o7+8
	 nop
	.cfi_endproc
.LFE3:
	.size	main_3, .-main_3
.Letext0:
	.file 2 "/usr/lib/gcc/sparc64-linux-gnu/12/include/stdint-gcc.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.uaword	0xe3
	.uahalf	0x5
	.byte	0x1
	.byte	0x4
	.uaword	.Ldebug_abbrev0
	.uleb128 0x4
	.uaword	.LASF13
	.byte	0x1d
	.uaword	.LASF0
	.uaword	.LASF1
	.uaword	.Ltext0
	.uaword	.Letext0-.Ltext0
	.uaword	.Ldebug_line0
	.uleb128 0x1
	.byte	0x1
	.byte	0x6
	.uaword	.LASF2
	.uleb128 0x1
	.byte	0x2
	.byte	0x5
	.uaword	.LASF3
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.asciz	"int"
	.uleb128 0x1
	.byte	0x8
	.byte	0x5
	.uaword	.LASF4
	.uleb128 0x1
	.byte	0x1
	.byte	0x8
	.uaword	.LASF5
	.uleb128 0x1
	.byte	0x2
	.byte	0x7
	.uaword	.LASF6
	.uleb128 0x6
	.uaword	.LASF14
	.byte	0x2
	.byte	0x34
	.byte	0x19
	.uaword	0x5c
	.uleb128 0x1
	.byte	0x4
	.byte	0x7
	.uaword	.LASF7
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.uaword	.LASF8
	.uleb128 0x2
	.asciz	"C0"
	.byte	0x2
	.uaword	0x50
	.uleb128 0x5
	.byte	0x3
	.uaword	C0
	.uleb128 0x2
	.asciz	"C1"
	.byte	0x3
	.uaword	0x50
	.uleb128 0x5
	.byte	0x3
	.uaword	C1
	.uleb128 0x2
	.asciz	"C2"
	.byte	0x4
	.uaword	0x50
	.uleb128 0x5
	.byte	0x3
	.uaword	C2
	.uleb128 0x2
	.asciz	"C3"
	.byte	0x5
	.uaword	0x50
	.uleb128 0x5
	.byte	0x3
	.uaword	C3
	.uleb128 0x3
	.uaword	.LASF9
	.byte	0x16
	.uaword	.LFB3
	.uaword	.LFE3-.LFB3
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x3
	.uaword	.LASF10
	.byte	0x11
	.uaword	.LFB2
	.uaword	.LFE2-.LFB2
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x3
	.uaword	.LASF11
	.byte	0xc
	.uaword	.LFB1
	.uaword	.LFE1-.LFB1
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x3
	.uaword	.LASF12
	.byte	0x7
	.uaword	.LFB0
	.uaword	.LFE0-.LFB0
	.uleb128 0x1
	.byte	0x9c
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0x21
	.sleb128 11
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0x21
	.sleb128 6
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x6
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.uaword	0x1c
	.uahalf	0x2
	.uaword	.Ldebug_info0
	.byte	0x4
	.byte	0
	.uahalf	0
	.uahalf	0
	.uaword	.Ltext0
	.uaword	.Letext0-.Ltext0
	.uaword	0
	.uaword	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF4:
	.asciz	"long long int"
.LASF7:
	.asciz	"unsigned int"
.LASF12:
	.asciz	"main_0"
.LASF11:
	.asciz	"main_1"
.LASF10:
	.asciz	"main_2"
.LASF9:
	.asciz	"main_3"
.LASF8:
	.asciz	"long long unsigned int"
.LASF5:
	.asciz	"unsigned char"
.LASF14:
	.asciz	"uint32_t"
.LASF6:
	.asciz	"short unsigned int"
.LASF2:
	.asciz	"signed char"
.LASF3:
	.asciz	"short int"
.LASF13:
	.asciz	"GNU C17 12.1.1 20220507 (Red Hat Cross 12.1.1-2) -mptr32 -mno-stack-bias -mlong-double-64 -m32 -mcpu=v8 -g -ffreestanding"
	.section	.debug_line_str,"MS",@progbits,1
.LASF1:
	.asciz	"/home/Manoj/Documents/ajit-toolchain-marshal/AjitPublicResources/processor/64bit/multi_core_verification/multi_core_hello_world_2x2"
.LASF0:
	.asciz	"main.c"
	.ident	"GCC: (GNU) 12.1.1 20220507 (Red Hat Cross 12.1.1-2)"
	.section	.note.GNU-stack,"",@progbits
