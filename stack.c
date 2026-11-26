/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.c
 */

#include "stack.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Creates empty stack
 *
 * @param s     Pointer to the stack
 */
void initStack(Stack *s) { s->top = NULL; }

/**
 * @brief Checks if the stack is empty
 *
 * @param s     Pointer to the stack
 *
 * @return true if top is NULL, false otherwise
 */
bool isEmpty(Stack *s) { return s->top == NULL; }

/**
 * @brief Pushes data to the top of the stack
 *
 * @param s      Pointer to the stack
 * @param data   Expresion symbol to be pushed
 */
void push(Stack *s, exprSymbol data) {
	SElem *newElemP = (SElem *)malloc(sizeof(SElem));
	if (newElemP == NULL) {
		error(INTERNAL_ERROR, "Could not allocate memory for newElem (SElem)");
	}

	newElemP->data = data;
	newElemP->next = s->top;
	s->top = newElemP;
}

/**
 * @brief Removes the top element from the stack and returns its data
 *
 * @param s      Pointer to the stack
 *
 * @return data of the removed element, otherwise empty data
 */
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

/**
 * @brief Returns a pointer to the top elements data without removing it
 *
 * @param s      Pointer to the stack
 *
 * @return pointer to the data on top, otherwise NULL if the stack is empty
 */
exprSymbol *top(Stack *s) {
	if (isEmpty(s)) {
		return NULL;
	}

	return &s->top->data;
}
