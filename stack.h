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

/**
 * @brief Kind of a symbol on the stack (used by the precedence analysis)
 */
typedef enum
{
    EXPR_TERM,      // terminal (operator, bracket, operand token, $)
    EXPR_NONTERM,   // E (already reduced expression)
    EXPR_HANDLE,    // '<' start of a handle
} exprSymbolKind;

/**
 * @brief One symbol stored on the stack
 * @param kind  terminal / nonterminal / handle start
 * @param prec  index into the precedence table (terminals only)
 * @param tok   token the symbol was created from (operator type, line, col)
 * @param node  AST node of an operand / E (not owned by the stack)
 */
typedef struct
{
    exprSymbolKind kind;
    int prec;
    token tok;
    astNode *node;
} exprSymbol;

typedef struct selem_t
{
    exprSymbol data;
    struct selem_t *next;
} SElem;

typedef struct stack_t
{
    SElem *top;
} Stack;

void initStack(Stack *s);
bool isEmpty(Stack *s);
void push(Stack *s, exprSymbol data);
exprSymbol pop(Stack *s);
exprSymbol *top(Stack *s);

#endif
