	.file	"1.c"
	.text
	.def	__main;	.scl	2;	.type	32;	.endef
	.section .rdata,"dr"
.LC0:
	.ascii "%x %s\12\0"
	.text
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	pushq	%rbp
	.seh_pushreg	%rbp
	movq	%rsp, %rbp
	.seh_setframe	%rbp, 0
	subq	$48, %rsp
	.seh_stackalloc	48
	.seh_endprologue
	call	__main
	movb	$50, -16(%rbp)
	movb	$48, -15(%rbp)
	movb	$49, -14(%rbp)
	movb	$54, -13(%rbp)
	movb	$0, -12(%rbp)
	# 上面初始化2016\0
	leaq	-16(%rbp), %rax #取出rbp的值 -16 赋给%rax
	#对应c = (char *)&data;
	movq	%rax, -8(%rbp) #一个指针64位
	movl	-16(%rbp), %eax #movl一个字节 一个int4个字节
	movq	-8(%rbp), %rdx
	movq	%rdx, %r8
	movl	%eax, %edx
	leaq	.LC0(%rip), %rcx
	call	printf
	#以下为退出复原
	movl	$0, %eax
	addq	$48, %rsp
	popq	%rbp
	ret
	.seh_endproc
	.ident	"GCC: (x86_64-win32-seh-rev0, Built by MinGW-W64 project) 8.1.0"
	.def	printf;	.scl	2;	.type	32;	.endef
