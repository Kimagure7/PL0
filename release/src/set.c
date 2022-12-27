

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "set.h"

//合并表头分别为s1与s2的两个链表，结点按照数值大小升序排列
//合并后的结果链表存入新的空间，并且以NULL为结尾
symset uniteset(symset s1, symset s2)
{
	symset s;
	snode *p;

	s1 = s1->next;
	s2 = s2->next;

	s = p = (snode *)malloc(sizeof(snode));
	while (s1 && s2)
	{
		p->next = (snode *)malloc(sizeof(snode));
		p = p->next;
		if (s1->elem < s2->elem)
		{
			p->elem = s1->elem;
			s1 = s1->next;
		}
		else
		{
			p->elem = s2->elem;
			s2 = s2->next;
		}
	}

	while (s1)
	{
		p->next = (snode *)malloc(sizeof(snode));
		p = p->next;
		p->elem = s1->elem;
		s1 = s1->next;
	}

	while (s2)
	{
		p->next = (snode *)malloc(sizeof(snode));
		p = p->next;
		p->elem = s2->elem;
		s2 = s2->next;
	}

	p->next = NULL;

	return s;
} // uniteset

//向以s为表头的链表中，插入生成并关键字为elem的结点，且插入后链表结点仍按关键字升序排列
void setinsert(symset s, int elem)
{
	snode *p = s;
	snode *q;

	while (p->next && p->next->elem < elem)
	{
		p = p->next;
	}

	q = (snode *)malloc(sizeof(snode));
	q->elem = elem;
	q->next = p->next;
	p->next = q;
} // setinsert

//创建以elem,...为结点关键字的链表，结点按照升序排列，且该链表具有一个无意义的表头结点
symset createset(int elem, ... /* SYM_NULL */)
{
	va_list list;
	symset s;

	s = (snode *)malloc(sizeof(snode));
	s->next = NULL;

	va_start(list, elem);
	while (elem)
	{
		setinsert(s, elem);
		elem = va_arg(list, int);
	}
	va_end(list);
	return s;
} // createset

//销毁链表，所有关键字设置为一个负的定值，并free掉所有空间（包括表头）
void destroyset(symset s)
{
	snode *p;

	while (s)
	{
		p = s;
		p->elem = -1000000;
		s = s->next;
		free(p);
	}
} // destroyset

//查询以s为表头的链表中是否存在关键字为elem的结点，若存在则返回1，否则返回0
int inset(int elem, symset s)
{
	s = s->next;
	while (s && s->elem < elem)
		s = s->next;

	if (s && s->elem == elem)
		return 1;
	else
		return 0;
} // inset

// EOF set.c
