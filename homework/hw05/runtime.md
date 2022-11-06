### 一

1. 输出:36313032 2016
2. 汇编代码如下 

```asm
	.section .rodata
.LC0:
	.ascii "%x %s\n"
	.text
.globl	main
	.type main,@function
main:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	andl 	$-16, %esp #此行在内下三行完全无法理解
	movl 	$0, %eax
	subl 	%eax, %esp
	movb	$50, -24(%ebp)

	movb	$48, -23(%ebp)
	movb	$49, -22(%ebp)
	movb	$54, -21(%ebp)
	movb	$0, -20(%ebp)
	leal	-24(%ebp), %eax

	movl	%eax, -28(%ebp)

	#以下三行无法确定
	movl	-24(%ebp), %edx
	movl	-28(%ebp), %rdx
	movl	%eax, %esi

	pushl 	$.LC0
	call	printf
	addq	$16, %esp
	movl	$0, %eax
	leave
	ret
```

