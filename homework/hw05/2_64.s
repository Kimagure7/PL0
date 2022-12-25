	.file	"2.c"
	.text
	.globl	f
	.def	f;	.scl	2;	.type	32;	.endef
	.seh_proc	f
f:
	pushq	%rbp
	.seh_pushreg	%rbp
	pushq	%rbx
	.seh_pushreg	%rbx
	subq	$72, %rsp
	.seh_stackalloc	72
	leaq	128(%rsp), %rbp
	.seh_setframe	%rbp, 128
	.seh_endprologue
	movq	%rcx, %rbx
	movl	$100, (%rbx)
	movl	$24, 4(%rbx)
	movb	$65, 9(%rbx)

	movq	16(%rbx), %rax
	movq	(%rax), %rdx
	movq	%rdx, -96(%rbp)

	movq	8(%rax), %rdx
	movq	%rdx, -88(%rbp)

	movq	16(%rax), %rax
	movq	%rax, -80(%rbp)
	
	leaq	-96(%rbp), %rax
	movq	%rax, %rcx
	call	f
	nop
	addq	$72, %rsp
	popq	%rbx
	popq	%rbp
	ret
	.seh_endproc
	.ident	"GCC: (x86_64-win32-seh-rev0, Built by MinGW-W64 project) 8.1.0"
