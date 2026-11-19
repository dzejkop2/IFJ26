/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.c
 */

#include "symtable.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

AVLNode *initTree(void) { return NULL; }

bool symtableIsEmpty(AVLNode *root) { return root == NULL; }

AVLNode *searchNode(AVLNode *root, char *string) {
	if (symtableIsEmpty(root))
		return NULL;

	AVLNode *temp_node = root;
	int res;
	while (temp_node != NULL) {
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

int getHeight(AVLNode *root) {
	if (!symtableIsEmpty(root))
		return root->height;
	return 0;
}

int getBalanceFactor(AVLNode *root) {
	if (!symtableIsEmpty(root))
		return getHeight(root->left) - getHeight(root->right);
	return 0;
}

int max(int a, int b) { return a > b ? a : b; }

AVLNode *leftRotate(AVLNode *node) {
	if (node == NULL)
		return NULL;

	AVLNode *temp_node_b = node->right;
	AVLNode *temp_node_y = temp_node_b->left;

	temp_node_b->left = node;
	node->right = temp_node_y;

	node->height = 1 + max(getHeight(node->left), getHeight(node->right));

	temp_node_b->height =
		1 + max(getHeight(temp_node_b->left), getHeight(temp_node_b->right));

	return temp_node_b;
}

AVLNode *rightRotate(AVLNode *node) {
	if (node == NULL)
		return NULL;

	AVLNode *temp_node_a = node->left;
	AVLNode *temp_node_y = temp_node_a->right;

	temp_node_a->right = node;
	node->left = temp_node_y;

	node->height = 1 + max(getHeight(node->left), getHeight(node->right));

	temp_node_a->height =
		1 + max(getHeight(temp_node_a->left), getHeight(temp_node_a->right));

	return temp_node_a;
}

AVLNode *createNode(paramInfo params) {
	AVLNode *temp_node = (AVLNode *)malloc(sizeof(AVLNode));
	if (temp_node == NULL) {
		fprintf(stderr, "Error (createNode): Could not allocate memory for "
						"temp_node (AVLNode).\n");
		return NULL;
	}

	temp_node->params.name = malloc(strlen(params.name) + 1);
	if (temp_node->params.name == NULL) {
		fprintf(stderr, "Error (createNode): Could not allocate memory for "
						"temp_node->params.name (char *).\n");
		free(temp_node);
		return NULL;
	}

	strcpy(temp_node->params.name, params.name);
	temp_node->params.type = params.type;
	temp_node->params.is_mutable = params.is_mutable;
	temp_node->params.param_count = params.param_count;
	temp_node->params.node_type = params.node_type;
	temp_node->params.scope = params.scope;
	temp_node->params.depth = params.depth;
	temp_node->params.resolveState = params.resolveState;
	temp_node->params.definition = params.definition;
	temp_node->params.receiver = params.receiver;

	if (params.node_type == VARIABLE) {
		temp_node->params.param_count = 0;
		temp_node->params.param_list = NULL;
	} else if (params.node_type == FUNCTION) {
		if (params.param_count != 0) {
			temp_node->params.param_list =
				malloc(sizeof(argInfo) * params.param_count);
			if (temp_node->params.param_list == NULL) {
				fprintf(stderr,
						"Error (createNode): Could not allocate memory for "
						"temp_node->params.param_list (argInfo *).\n");
				free(temp_node->params.name);
				free(temp_node);
				return NULL;
			}

			for (size_t i = 0; i < params.param_count; i++) {
				temp_node->params.param_list[i].is_mutable =
					params.param_list[i].is_mutable;
				temp_node->params.param_list[i].type =
					params.param_list[i].type;

				temp_node->params.param_list[i].name =
					malloc(strlen(params.param_list[i].name) + 1);
				if (temp_node->params.param_list[i].name == NULL) {
					fprintf(stderr,
							"Error (createNode): Could not allocate memory for "
							"temp_node->params.param_list->name (char *).\n");

					for (size_t j = 0; j < i; j++) {
						free(temp_node->params.param_list[j].name);
					}

					free(temp_node->params.param_list);
					free(temp_node->params.name);
					free(temp_node);
					return NULL;
				}

				strcpy(temp_node->params.param_list[i].name,
					   params.param_list[i].name);
			}
		} else {
			temp_node->params.param_list = NULL;
		}
	}

	temp_node->height = 1;
	temp_node->left = NULL;
	temp_node->right = NULL;

	return temp_node;
}

AVLNode *insertNode(AVLNode *root, paramInfo params) {
	if (root == NULL)
		return createNode(params);

	int res = strcmp(root->params.name, params.name);
	if (res > 0) {
		root->left = insertNode(root->left, params);
	} else if (res < 0) {
		root->right = insertNode(root->right, params);
	} else {
		return root;
	}

	root->height = 1 + max(getHeight(root->left), getHeight(root->right));

	int bf = getBalanceFactor(root);

	if (bf > 1) {
		if (getBalanceFactor(root->left) < 0)
			root->left = leftRotate(root->left);
		return rightRotate(root);
	}

	if (bf < -1) {
		if (getBalanceFactor(root->right) > 0)
			root->right = rightRotate(root->right);
		return leftRotate(root);
	}

	return root;
}

void deleteNode(AVLNode *node) {
	free(node->params.name);

	if (node->params.node_type == FUNCTION) {
		if (node->params.param_count != 0) {
			for (size_t i = 0; i < node->params.param_count; i++) {
				free(node->params.param_list[i].name);
			}
			free(node->params.param_list);
		}
	}

	free(node);
}

void destroyAVL(AVLNode *root) {
	if (root == NULL)
		return;

	if (root->left != NULL)
		destroyAVL(root->left);
	if (root->right != NULL)
		destroyAVL(root->right);

	deleteNode(root);
}
