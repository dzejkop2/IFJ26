/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file sematic.h
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "error.h"
#include "ast.h"
#include "scanner.h"
#include "symtable.h"

void semanticAnalyze(astNode *root);

typedef struct Scope {
	AVLNode *symbols;
	struct Scope *parent;

	int id;
	int depth;
} Scope;

typedef struct FunctionInfo {
	astDataType returnType;
	astNode *definition;
} FunctionInfo;

#endif