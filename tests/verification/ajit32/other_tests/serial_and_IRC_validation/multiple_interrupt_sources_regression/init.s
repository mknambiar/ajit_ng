.section .text.main
.global main
.global _start
.extern ajit_main

main:
_start:
	mov %g0, %sp
	call ajit_main
	nop
	ta 0
	nop
