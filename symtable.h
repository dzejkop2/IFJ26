/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.h
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

typedef enum {
    STRING,
    INT,
    DOUBLE,
    UNIT,
    BOOLEAN
} Type;

typedef struct node_t {
    char *name;
    Type type;
    int scope;
    int depth;
    struct node_t *left;
    struct node_t *right;
    int height;
} AVLNode;

AVLNode* initTree(void);
bool isEmpty(AVLNode *root);
AVLNode* searchNode(AVLNode *root, char *string);
AVLNode* insertNode(AVLNode *root, char *name, Type type, int scope, int depth);
AVLNode* deleteNode(AVLNode *node, char *name);

#endif
