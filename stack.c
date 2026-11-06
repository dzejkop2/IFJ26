/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.c
 */

#include "stack.h"
#include <stdio.h>
#include <malloc.h>

void initStack(Stack *s)
{
    s->top = NULL;
}

bool isEmpty(Stack *s)
{
    return s->top == NULL;
}

void push(Stack *s, char *string)
{
    SElem *new_elem_p = (SElem *) malloc(sizeof(SElem));
    if (new_elem_p == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for newElem (SElem).\n");
        return;
    }

    new_elem_p->name = string;
    new_elem_p->next = s->top;
    s->top = new_elem_p;
}

void pop(Stack *s, char *string)
{
    SElem *temp_elem_p;
    if (s->top == NULL)
    {
        temp_elem_p = s->top;
        s->top = s->top->next;
        free(temp_elem_p);
    }
}

char *top(Stack *s)
{
    if (isEmpty(s))
    {
        fprintf(stderr, "Stack is empty.\n");
        return "";
    }

    return s->top->name;
}
