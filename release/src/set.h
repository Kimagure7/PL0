#ifndef SET_H
#define SET_H

//符号集合数据结构，以链表的形式
typedef struct snode
{
	int elem;
	struct snode* next;
} snode, *symset;

//以下符号集都是辅助判断的
//phi:{空}，即SYM_NULL
//To be done: SYM_NULL是干啥用的？
//declbegsys:声明的开始符号集合{const,var,procedure,空}
//statbegsys:语句的开始符号集合{begin,call,if,while,空}
//facbegsys:“因子”的开始符号集合{ident，number，(，-}
//relset:条件运算符集合{=,<>,<,>,<=,>=,空}
symset phi, declbegsys, statbegsys, facbegsys, relset;

symset createset(int data, .../* SYM_NULL */);
void destroyset(symset s);
symset uniteset(symset s1, symset s2);
int inset(int elem, symset s);

#endif
// EOF set.h
