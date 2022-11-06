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

	subl 	$-4,
	pushl	-28(%ebp)
	pushl	-24(%ebp)
	

	pushl 	$.LC0
	call	printf
	addq	$16, %esp
	movl	$0, %eax
	leave
	ret
```

### 二

汇编代码：
1. N=2:
```asm
	.file "test1.c"
	.text
.globl f
	.type f,@function
f:
	pushl %ebp
	movl %esp, %ebp
	movl $100, 8(%ebp)
	movl $16 , 12(%ebp)
	movb $65 , 16(%ebp) 
	movl , %eax
	pushl 
	pushl 
	pushl 
	pushl 
	call f
	addl $16, %esp
	leave
	ret
//当 N=2 时，生成的汇编代码片段。
```