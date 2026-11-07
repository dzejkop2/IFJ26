/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.c
 */

#include "stack.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void initStack(Stack *s)
{
    s->top = NULL;
}

bool isEmpty(Stack *s)
{
    return s->top == NULL;
}

void push(Stack *s, exprSymbol data)
{
    SElem *new_elem_p = (SElem *) malloc(sizeof(SElem));
    if (new_elem_p == NULL)
    {
        error(INTERNAL_ERROR, "Could not allocate memory for newElem (SElem)");
    }

    new_elem_p->data = data;
    new_elem_p->next = s->top;
    s->top = new_elem_p;
}

exprSymbol pop(Stack *s)
{
    exprSymbol data = { 0 };
    SElem *temp_elem_p;
    if (s->top != NULL)
    {
        temp_elem_p = s->top;
        data = temp_elem_p->data;
        s->top = s->top->next;
        free(temp_elem_p);
    }

    return data;
}

exprSymbol *top(Stack *s)
{
    if (isEmpty(s))
    {
        return NULL;
    }

    return &s->top->data;
}
