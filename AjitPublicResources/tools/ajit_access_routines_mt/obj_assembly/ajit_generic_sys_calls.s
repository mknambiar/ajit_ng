	.file	"ajit_generic_sys_calls.c"
	.section	".text"
	.align 4
	.global ajit_sys_get_core_and_thread_id
	.type	ajit_sys_get_core_and_thread_id, #function
	.proc	020
ajit_sys_get_core_and_thread_id:
	save	%sp, -96, %sp
	call	ajit_sys_read_asr, 0
	 mov	29, %o0
	srl	%o0, 8, %g1
	stb	%g1, [%i0]
	stb	%o0, [%i1]
	jmp	%i7+8
	 restore
	.size	ajit_sys_get_core_and_thread_id, .-ajit_sys_get_core_and_thread_id
	.align 4
	.global ajit_sys_get_clock_time
	.type	ajit_sys_get_clock_time, #function
	.proc	017
ajit_sys_get_clock_time:
	save	%sp, -96, %sp
	call	ajit_sys_read_asr, 0
	 mov	30, %o0
	mov	%o0, %i1
	call	ajit_sys_read_asr, 0
	 mov	31, %o0
	jmp	%i7+8
	 restore %g0, %o0, %o0
	.size	ajit_sys_get_clock_time, .-ajit_sys_get_clock_time
	.align 4
	.global ajit_sys_sleep
	.type	ajit_sys_sleep, #function
	.proc	020
ajit_sys_sleep:
	save	%sp, -96, %sp
	call	ajit_sys_read_asr, 0
	 mov	30, %o0
	mov	%o0, %i5
	call	ajit_sys_read_asr, 0
	 mov	31, %o0
	addcc	%i5, %i0, %i3
	addx	%o0, 0, %i2
	cmp	%i2, %o0
	bleu	.L11
	 nop
.L9:
	call	ajit_sys_read_asr, 0
	 mov	30, %o0
	mov	%o0, %i5
	call	ajit_sys_read_asr, 0
	 mov	31, %o0
	cmp	%i2, %o0
	bgu	.L9
	 nop
	bne	.L13
	 cmp	%i3, %i5
	bgu	.L9
	 nop
.L13:
	jmp	%i7+8
	 restore
.L11:
	bne	.L13
	 cmp	%i3, %i5
	bgu	.L9
	 nop
	jmp	%i7+8
	 restore
	.size	ajit_sys_sleep, .-ajit_sys_sleep
	.ident	"GCC: (GNU) 12.1.1 20220507 (Red Hat Cross 12.1.1-2)"
	.section	.note.GNU-stack,"",@progbits
