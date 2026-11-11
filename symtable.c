/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.c
 */

#include "symtable.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

AVLNode* initTree(void)
{
    return NULL;
}

bool isEmpty(AVLNode *root)
{
    return root == NULL;
}

AVLNode* searchNode(AVLNode *root, char *string)
{
    if (isEmpty(root))
        return NULL;

    AVLNode *temp_node = root;
    int res;
    while (temp_node != NULL)
    {
        res = strcmp(temp_node->params.name, string);

        if (res == 0)
            break;

        if (res > 0)
            temp_node = temp_node->left;
        else
            temp_node = temp_node->right;
    }

    return temp_node;
}

int getHeight(AVLNode *root)
{
    if (!isEmpty(root))
        return root->height;
    return 0;
}

int getBalanceFactor(AVLNode *root)
{
    if (!isEmpty(root))
        return getHeight(root->left) - getHeight(root->right);
    return 0;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

AVLNode* leftRotate(AVLNode *node)
{
    if (node == NULL)
        return NULL;

    AVLNode *temp_node_b = node->right;
    AVLNode *temp_node_y = temp_node_b->left;

    temp_node_b->left = node;
    node->right = temp_node_y;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    temp_node_b->height = 1 + max(getHeight(temp_node_b->left), getHeight(temp_node_b->right));

    return temp_node_b;
}

AVLNode* rightRotate(AVLNode *node)
{
    if (node == NULL)
        return NULL;

    AVLNode *temp_node_a = node->left;
    AVLNode *temp_node_y = temp_node_a->right;

    temp_node_a->right = node;
    node->left = temp_node_y;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    temp_node_a->height = 1 + max(getHeight(temp_node_a->left), getHeight(temp_node_a->right));

    return temp_node_a;
}

AVLNode* createNode(paramInfo params)
{
    AVLNode *temp_node = (AVLNode*) malloc(sizeof(AVLNode));
    if (temp_node == NULL)
    {
        fprintf(stderr, "Error (createNode): Could not allocate memory for temp_node (AVLNode).\n");
        return NULL;
    }

    temp_node->params.name = malloc(strlen(params.name) + 1);
    if (temp_node->params.name == NULL)
    {
        fprintf(stderr, "Error (createNode): Could not allocate memory for temp_node->name (char *).\n");
        free(temp_node);
        return NULL;
    }

    strcpy(temp_node->params.name, params.name);
    temp_node->params.type = params.type;
    temp_node->params.is_mutable = params.is_mutable;
    temp_node->params.node_type = params.node_type;
    temp_node->params.param_count = params.param_count;
    temp_node->params.scope = params.scope;
    temp_node->params.depth = params.depth;

    temp_node->height = 1;
    temp_node->left = NULL;
    temp_node->right = NULL;

    return temp_node;
}

AVLNode* insertNode(AVLNode *root, paramInfo params)
{
    if (root == NULL)
        return createNode(params);

    int res = strcmp(root->params.name, params.name);
    if (res > 0)
    {
        root->left = insertNode(root->left, params);
    }
    else if (res < 0)
    {
        root->right = insertNode(root->right, params);
    }
    else
    {
        return root;
    }

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));

    int bf = getBalanceFactor(root);

    if (bf > 1)
    {
        if (getBalanceFactor(root->left) < 0)
            root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    if (bf < -1)
    {
        if (getBalanceFactor(root->right) > 0)
            root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

AVLNode* getSuccessor(AVLNode *node)
{
    node = node->right;

    while (node->left != NULL)
    {
        node = node->left;
    }

    return node;
}

AVLNode* deleteNode(AVLNode *root, char *name)
{
    if (root == NULL)
        return root;

    int res = strcmp(root->params.name, name);
    if (res > 0)
    {
        root->left = deleteNode(root->left, name);
    }
    else if (res < 0)
    {
        root->right = deleteNode(root->right, name);
    }
    else
    {
        AVLNode *temp_node;
        if (root->left == NULL)
        {
            temp_node = root->right;
            free(root->params.name);
            free(root);
            return temp_node;
        }
        else if (root->right == NULL)
        {
            temp_node = root->left;
            free(root->params.name);
            free(root);
            return temp_node;
        }

        // find inorder successor
        temp_node = getSuccessor(root);

        char *temp_name = malloc(strlen(temp_node->params.name) + 1);
        if (temp_name == NULL)
        {
            fprintf(stderr, "Error (deleteNode): Could not allocate memory for temp_name (char *).\n");
            return root;
        }
        free(root->params.name);

        root->params.name = temp_name;
        strcpy(root->params.name, temp_node->params.name);
        root->params.type = temp_node->params.type;
        root->params.is_mutable = temp_node->params.is_mutable;
        root->params.node_type = temp_node->params.node_type;
        root->params.param_count = temp_node->params.param_count;
        root->params.scope = temp_node->params.scope;
        root->params.depth = temp_node->params.depth;

        root->right = deleteNode(root->right, root->params.name);
    }

    root->height = 1 + max(getHeight(root->left), getHeight(root->right));

    int bf = getBalanceFactor(root);

    if (bf > 1)
    {
        if (getBalanceFactor(root->left) < 0)
            root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    if (bf < -1)
    {
        if (getBalanceFactor(root->right) > 0)
            root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}
