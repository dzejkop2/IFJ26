/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.h
 */

#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

typedef struct selem_t
{
    char *name;
    struct selem_t *next;
} SElem;

typedef struct stack_t
{
    SElem *top;
} Stack;

void initStack(Stack *s);
bool isEmpty(Stack *s);
void push(Stack *s, char *string);
void pop(Stack *s, char *string);
char *top(Stack *s);

#endif
