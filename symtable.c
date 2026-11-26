/**
 * @name ifj26
 * @authors Marek Goliaš (xgoliam00)
 * @file symtable.c
 */

#include "symtable.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>

/**
 * @brief Creates empty tree
 *
 * @return NULL -> represents an empty tree
 */
AVLNode *initTree(void) { return NULL; }

/**
 * @brief Checks if the tree is empty
 *
 * @param root     Pointer to the root node of the tree
 *
 * @return true if the tree contains no nodes, false otherwise
 */
bool symtableIsEmpty(AVLNode *root) { return root == NULL; }

/**
 * @brief Searches the tree for a symbol with the provided name
 *
 * @param root     Pointer to the root node of the tree
 * @param string   Name of the symbol to find
 *
 * @return node that contains the name, otherwise NULL
 */
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

/**
 * @brief Gets height of tree or subtree based on provided node
 *
 * @param root     Pointer to the node in the tree
 *
 * @return height of subtree or tree if its not empty, 0 otherwise
 */
int getHeight(AVLNode *root) {
	if (!symtableIsEmpty(root))
		return root->height;
	return 0;
}

/**
 * @brief Gets balance factor of the node
 *
 * In a valid AVL tree it is always in the range [-1, 1].
 * A positive value means the node is left-heavy,
 * a negative value means right-heavy.
 *
 * @param root     Pointer to the node in the tree
 *
 * @return height of left subtree minus height of right subtree,
 * otherwise 0 if the node is NULL
 */
int getBalanceFactor(AVLNode *root) {
	if (!symtableIsEmpty(root))
		return getHeight(root->left) - getHeight(root->right);
	return 0;
}

/**
 * @brief Gets greater value from two integers
 *
 * Helper function for height calculation
 *
 * @param a     First integer
 * @param b     Second integer
 *
 * @return a if a is greater than b, otherwise b
 */
int max(int a, int b) { return a > b ? a : b; }

/**
 * @brief Rotates subtree to the left
 *
 * Helper function for insert
 *
 * @param node     Pointer to the node in the tree
 *
 * @return new root of the subtree, otherwise NULL if node is NULL
 */
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

/**
 * @brief Rotates subtree to the right
 *
 * Helper function for insert
 *
 * @param node     Pointer to the node in the tree
 *
 * @return new root of the subtree, otherwise NULL if node is NULL
 */
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

/**
 * @brief Creates new node and fills it from the provided symbol info
 *
 * Helper function for insert
 *
 * @param params   Symbol information to store in the node
 *
 * @return pointer to the new node, otherwise NULL if memory allocation failed
 */
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

/**
 * @brief Inserts symbol into the tree and rebalances it
 *
 * @param root     Pointer to the root node of the tree
 * @param params   Symbol information to insert
 *
 * @return new root of the tree
 */
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

/**
 * @brief Frees a single node and its allocated memory
 *
 * @param node     Node to be freed
 */
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

/**
 * @brief Frees the whole tree
 *
 * @param root     Pointer to the root node of the tree
 */
void destroyAVL(AVLNode *root) {
	if (root == NULL)
		return;

	if (root->left != NULL)
		destroyAVL(root->left);
	if (root->right != NULL)
		destroyAVL(root->right);

	deleteNode(root);
}
