/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file stack.h
 */

#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include "scanner.h"
#include "ast.h"

typedef enum {
	EXPR_TERM,
	EXPR_NONTERM,
	EXPR_HANDLE, // marks the start of the rule to reduce
} exprSymbolKind;

typedef struct {
	exprSymbolKind kind;
	int prec; // precedence table index
	token tok;
	astNode *node; // AST node of a reduced expression
	bool isArgList;
} exprSymbol;

typedef struct selem_t {
	exprSymbol data;
	struct selem_t *next;
} SElem;

typedef struct stack_t {
	SElem *top;
} Stack;

void initStack(Stack *s);
bool isEmpty(Stack *s);
void push(Stack *s, exprSymbol data);
exprSymbol pop(Stack *s);
exprSymbol *top(Stack *s);

#endif
