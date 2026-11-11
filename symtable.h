/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.h
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { TYPE_STRING, TYPE_INT, TYPE_DOUBLE, TYPE_UNIT, TYPE_BOOL } Type;

typedef enum { VARIABLE, FUNCTION } nodeType;

typedef struct param_info_t {
	char *name;
	Type type;
	bool is_mutable;
	nodeType node_type;
    size_t param_count;

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
bool isEmpty(AVLNode *root);
AVLNode *searchNode(AVLNode *root, char *string);
AVLNode *insertNode(AVLNode *root, paramInfo params);
AVLNode *deleteNode(AVLNode *node, char *name);

#endif
