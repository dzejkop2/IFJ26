/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.h
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include "ast.h"

typedef enum { VARIABLE, FUNCTION } nodeType;

typedef struct param_info_t {
	char *name;
	astDataType type;
	bool is_mutable;
} argInfo;

typedef enum { TYPE_UNVISITED, TYPE_RESOLVING, TYPE_RESOLVED } typeResolveState;

typedef struct node_param_t {
	char *name;
	astDataType type;
	nodeType node_type;
	bool is_mutable;

	size_t param_count;
	argInfo *param_list;
	typeResolveState resolveState;
	astNode *definition;

	int scope;
	int depth;
} paramInfo;

typedef struct node_t {
	paramInfo params;

	struct node_t *left;
	struct node_t *right;
	int height;
} AVLNode;

AVLNode *initTree(void);
bool symtableIsEmpty(AVLNode *root);
AVLNode *searchNode(AVLNode *root, char *string);
AVLNode *insertNode(AVLNode *root, paramInfo params);
AVLNode *deleteNode(AVLNode *node, char *name);

#endif