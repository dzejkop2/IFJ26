/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.c
 */

#include "stack.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void initStack(Stack *s) { s->top = NULL; }

bool isEmpty(Stack *s) { return s->top == NULL; }

void push(Stack *s, exprSymbol data) {
	SElem *newElemP = (SElem *)malloc(sizeof(SElem));
	if (newElemP == NULL) {
		error(INTERNAL_ERROR, "Could not allocate memory for newElem (SElem)");
	}

	newElemP->data = data;
	newElemP->next = s->top;
	s->top = newElemP;
}

exprSymbol pop(Stack *s) {
	exprSymbol data = {0};
	SElem *tempElemP;
	if (s->top != NULL) {
		tempElemP = s->top;
		data = tempElemP->data;
		s->top = s->top->next;
		free(tempElemP);
	}

	return data;
}

exprSymbol *top(Stack *s) {
	if (isEmpty(s)) {
		return NULL;
	}

	return &s->top->data;
}
