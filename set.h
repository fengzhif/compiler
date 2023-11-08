/*
 * Topic: 
 * Highlight:
 *  Shawn Jia  in University of sci&tech of China
 * @LastEditTime: 2022-12-05 15:33:46
 */
#ifndef SET_H
#define SET_H

//集合
typedef struct snode
{
	int elem;
	struct snode* next;
} snode, *symset;

symset phi;//非法标识
symset declbegsys;//类型推断
symset statbegsys;//statement语句的first集合
symset facbegsys;//表达式的first集合
symset  relset;//关系运算符

symset createset(int data, .../* SYM_NULL */);
void destroyset(symset s);
symset uniteset(symset s1, symset s2);
int inset(int elem, symset s);

#endif
// EOF set.h
