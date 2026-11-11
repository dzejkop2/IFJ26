/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file semantic.c
 */

#include <stdio.h>
// #include <stdlib.h>
#include "semantic.h"

static int scopeIdCounter = 0;

static void analyzeNode(astNode *node, Scope *scope);
static void analyzeBinaryOp(astNode *node, Scope *scope);
static void analyzeUnaryOp(astNode *node, Scope *scope);
static void analyzeExpr(astNode *node, Scope *scope);
static void analyzeIdentifier(astNode *node, Scope *scope);
static void analyzeVarDecl(astNode *node, Scope *scope);
static void analyzeBlock(astNode *node, Scope *parentScope);
static void analyzeCall(astNode *node, Scope *scope);

astDataType symToSynConvert(Type type) {
	switch (type) {
	case INT:
		return TYPE_INT;
		break;
	case DOUBLE:
		return TYPE_DOUBLE;
		break;
	case STRING:
		return TYPE_STRING;
		break;
	case UNIT:
		return TYPE_UNIT;
		break;
	case BOOLEAN:
		return TYPE_BOOL;
		break;
	default:
		error(INTERNAL_ERROR, "Unknown symbol type");
	}
	return TYPE_NONE;
}

static void analyzeNodeList(astNodeList nodeList, Scope *scope) {
	for (size_t i = 0; i < nodeList.count; i++) {
		analyzeNode(nodeList.items[i], scope);
	}
}

void semanticAnalyze(astNode *root) {
	if (root == NULL || root->type != AST_PROGRAM) {
		error(INTERNAL_ERROR, "Root node is not a program");
	}

	Scope globalScope = {.symbols = initTree(),
						 .parent = NULL,
						 .id = scopeIdCounter++,
						 .depth = 0};

	analyzeNode(root, &globalScope);
}

static void analyzeNode(astNode *node, Scope *scope) {
	if (node == NULL) {
		return;
	}

	switch (node->type) {
	case AST_PROGRAM:
		analyzeNodeList(node->data.program.defs, scope);
		break;

	case AST_FUN_DEF:
		analyzeNode(node->data.funcDef.body, scope);
		break;
	case AST_EXTENSION:
		analyzeNodeList(node->data.extension.funDefs, scope);

		break;
	case AST_PARAM_LIST:
		analyzeNodeList(node->data.paramList.params, scope);
		break;

	case AST_PARAM:
		// No specific analysis needed for parameters
		break;

	case AST_BLOCK:
		analyzeBlock(node, scope);
		break;
	case AST_VAR_DECL:
		analyzeVarDecl(node, scope);
		break;
	case AST_ASSIGN:
		analyzeNode(node->data.assign.target, scope);
		analyzeNode(node->data.assign.value, scope);
		break;
	case AST_RETURN:
		analyzeNode(node->data.ret.value, scope);
		break;

	case AST_WHILE:
		analyzeExpr(node->data.whileLoop.cond, scope);
		if (node->data.whileLoop.cond->dataType != TYPE_BOOL) {
			error(SEMANTIC_TYPE_ERROR, "While condition must be of type bool");
		}
		analyzeNode(node->data.whileLoop.body, scope);
		break;
	case AST_FOR:
		analyzeNode(node->data.forLoop.from, scope);
		analyzeNode(node->data.forLoop.to, scope);
		analyzeNode(node->data.forLoop.step, scope);
		analyzeNode(node->data.forLoop.body, scope);
		analyzeNodeList(node->data.forLoop.guards, scope);
		break;
	case AST_IF:
		analyzeExpr(node->data.ifStmt.cond, scope);
		if (node->data.ifStmt.cond->dataType != TYPE_BOOL) {
			error(SEMANTIC_TYPE_ERROR, "If condition must be of type bool");
		}
		analyzeNode(node->data.ifStmt.thenBody, scope);
		analyzeNode(node->data.ifStmt.elseBody, scope);
		break;
	case AST_EXPR_STMT:
		analyzeNode(node->data.exprStmt.expr, scope);
		break;
	case AST_LIT_INT:
		node->dataType = TYPE_INT;
		break;
	case AST_LIT_DOUBLE:
		node->dataType = TYPE_DOUBLE;
		break;
	case AST_LIT_STRING:
		node->dataType = TYPE_STRING;
		break;
	case AST_LIT_BOOL:
		node->dataType = TYPE_BOOL;
		break;
	case AST_LIT_UNIT:
		node->dataType = TYPE_UNIT;
		break;
	case AST_IDENTIFIER:
		analyzeIdentifier(node, scope);
		break;
	case AST_BINARY_OP:
	case AST_UNARY_OP:
		analyzeExpr(node, scope);
		break;
	case AST_CALL:
		analyzeCall(node, scope);
		break;
	case AST_MEMBER:
		analyzeNode(node->data.member.object, scope);
		break;
	default:
		error(INTERNAL_ERROR, "Unknown AST node type");
		break;
	}
}

static AVLNode *searchSymbolInScope(Scope *scope, char *name) {
	Scope *currentScope = scope;
	if (currentScope == NULL) {
		return NULL;
	}
	while (currentScope != NULL) {
		AVLNode *symbol = searchNode(currentScope->symbols, name);
		if (symbol != NULL) {
			return symbol;
		}
		currentScope = currentScope->parent;
	}
	error(SEMANTIC_UNDEFINITION_ERROR, "Identifier used before declaration");
	return NULL;
}

static void analyzeBlock(astNode *node, Scope *parentScope) {
	if (node == NULL || node->type != AST_BLOCK) {
		error(INTERNAL_ERROR, "Invalid block node");
		return;
	}

	Scope blockScope = {.symbols = initTree(),
						.parent = parentScope,
						.id = scopeIdCounter++,
						.depth = parentScope->depth + 1};

	analyzeNodeList(node->data.block.stmts, &blockScope);
}

static void analyzeVarDecl(astNode *node, Scope *scope) {
	astNode *valueNode = node->data.varDecl.value;
	astDataType declType = node->data.varDecl.declType;

	if (valueNode != NULL) {
		analyzeNode(node->data.varDecl.value, scope);
		if (declType != TYPE_NONE && valueNode->dataType != declType) {
			error(SEMANTIC_TYPE_ERROR, "Variable declaration type mismatch");
		}
		node->dataType = declType;
		return;
	}

	if (declType == TYPE_NONE) {
		error(SEMANTIC_TYPE_ERROR,
			  "Variable declaration without type and without value");
		return;
	}

	AVLNode *existingSymbol =
		searchSymbolInScope(scope, node->data.varDecl.name);

	if (existingSymbol != NULL) {
		error(SEMANTIC_REDEFINITION_ERROR, "Variable redeclaration");
		return;
	}
	node->dataType = declType;
	return;
}

static void analyzeIdentifier(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_IDENTIFIER) {
		error(INTERNAL_ERROR, "Invalid identifier node");
		return;
	}

	AVLNode *symbol = searchSymbolInScope(scope, node->data.identifier.name);
	if (symbol == NULL) {
		error(SEMANTIC_UNDEFINITION_ERROR,
			  "Identifier used before declaration");
		return;
	}
	switch (symbol->type) {
	case INT:
		node->dataType = TYPE_INT;
		break;
	case DOUBLE:
		node->dataType = TYPE_DOUBLE;
		break;
	case STRING:
		node->dataType = TYPE_STRING;
		break;
	case UNIT:
		node->dataType = TYPE_UNIT;
		break;
	case BOOLEAN:
		node->dataType = TYPE_BOOL;
		break;
	default:
		error(INTERNAL_ERROR, "Unknown symbol type");
	}
}

static void analyzeExpr(astNode *node, Scope *scope) {
	if (node == NULL) {
		return;
	}

	switch (node->type) {
	case AST_BINARY_OP:
		analyzeExpr(node->data.binary.left, scope);
		analyzeExpr(node->data.binary.right, scope);
		analyzeBinaryOp(node, scope);
		break;
	case AST_UNARY_OP:
		analyzeExpr(node->data.unary.operand, scope);
		analyzeUnaryOp(node, scope);
		break;
	case AST_CALL:
		analyzeCall(node, scope);
		break;
	case AST_MEMBER:
		analyzeExpr(node->data.member.object, scope);
		break;
	case AST_LIT_INT:

		node->dataType = TYPE_INT;
		break;
	case AST_LIT_DOUBLE:
		node->dataType = TYPE_DOUBLE;
		break;
	case AST_LIT_STRING:
		node->dataType = TYPE_STRING;
		break;
	case AST_LIT_BOOL:
		node->dataType = TYPE_BOOL;
		break;
	case AST_LIT_UNIT:
		node->dataType = TYPE_UNIT;
		break;
	default:
		// No specific analysis needed for other expression types
		break;
	}
}

static void analyzeCall(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_CALL) {
		error(INTERNAL_ERROR, "Invalid function call node");
		return;
	}
	if (node->data.call.callee->type != AST_IDENTIFIER) {
		error(INTERNAL_ERROR, "Unsupported callee type");
	}

	AVLNode *callFunc = searchSymbolInScope(
		scope, node->data.call.callee->data.identifier.name);
	if (callFunc == NULL) {
		error(SEMANTIC_UNDEFINITION_ERROR, "Function used before declaration");
		return;
	}

	if (callFunc->nodeType != FUNCTION) {
		error(SEMANTIC_TYPE_ERROR, "Identifier is not a function");
		return;
	}

	analyzeNodeList(node->data.call.args, scope);

	if (node->data.call.args.count != callFunc->function.paramCount) {
		error(SEMANTIC_TYPE_ERROR, "Function call argument count mismatch");
		return;
	}
	if (node->data.call.args.count == 0) {
		node->dataType = symToSynConvert(callFunc->type);
		return;
	}
	for (size_t i = 0; i < node->data.call.args.count; i++) {
		if (node->data.call.args.items[i]->dataType !=
			symToSynConvert(callFunc->function.params[i].type)) {
			error(SEMANTIC_TYPE_ERROR, "Function call argument type mismatch");
			return;
		}
	}
	node->dataType = symToSynConvert(callFunc->type);
	return;
}

static void analyzeBinaryOp(astNode *node, Scope *scope) {
	if (node == NULL || node->type != AST_BINARY_OP) {
		error(INTERNAL_ERROR, "Invalid binary operation");
		return;
	}

	analyzeExpr(node->data.binary.left, scope);
	analyzeExpr(node->data.binary.right, scope);

	astDataType leftType = node->data.binary.left->dataType;
	astDataType rightType = node->data.binary.right->dataType;
	switch (node->data.binary.op) {
	case op_add:
		if (leftType == TYPE_STRING && rightType == TYPE_STRING) {
			node->dataType = TYPE_STRING;
		} else if (leftType == TYPE_INT && rightType == TYPE_INT) {
			node->dataType = TYPE_INT;
		} else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
				   (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
				   (leftType == TYPE_INT && rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_DOUBLE;
		} else {
			error(SEMANTIC_TYPE_ERROR, "Invalid types for addition");
		}
		break;
	case op_sub:
	case op_mul:
	case op_div:
		if (leftType == TYPE_INT && rightType == TYPE_INT) {
			node->dataType = TYPE_INT;
		} else if ((leftType == TYPE_DOUBLE && rightType == TYPE_DOUBLE) ||
				   (leftType == TYPE_DOUBLE && rightType == TYPE_INT) ||
				   (leftType == TYPE_INT && rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_DOUBLE;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for arithmetic operation");
		}
		break;

	case op_equal:
	case op_notEqual:
		if (((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			 (rightType == TYPE_INT || rightType == TYPE_DOUBLE)) ||
			leftType == rightType) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for comparison operation");
		}
		break;
	case op_biggerThen:
	case op_lowerThen:
	case op_lowerEqual:
	case op_biggerEqual:
		if ((leftType == TYPE_INT || leftType == TYPE_DOUBLE) &&
			(rightType == TYPE_INT || rightType == TYPE_DOUBLE)) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid types for comparison operation");
		}
		break;
	case op_and:
	case op_or:
		if (leftType == TYPE_BOOL && rightType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR, "Invalid types for logical operation");
		}
		break;
	default:
		// Handle other binary operations as needed
		break;
	}

	// Here you can implement type checking for binary operations
	// For example, if both operands are integers, the result is an integer
}

static void analyzeUnaryOp(astNode *node, Scope *scope) {

	if (node == NULL || node->type != AST_UNARY_OP) {
		error(INTERNAL_ERROR, "Invalid unary operation");
		return;
	}

	analyzeExpr(node->data.unary.operand, scope);

	astDataType operandType = node->data.unary.operand->dataType;

	switch (node->data.unary.op) {
	case op_not:
		if (operandType == TYPE_BOOL) {
			node->dataType = TYPE_BOOL;
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid type for logical NOT operation");
		}
		break;
	case op_sub:
		if (operandType == TYPE_INT || operandType == TYPE_DOUBLE) {
			node->dataType =
				operandType; // Result type is the same as operand type
		} else {
			error(SEMANTIC_TYPE_ERROR,
				  "Invalid type for unary negation operation");
		}
		break;
	case op_add:
		if (operandType != TYPE_INT && operandType != TYPE_DOUBLE) {
			error(SEMANTIC_TYPE_ERROR, "Invalid type for unary plus operation");
		} else {
			node->dataType =
				operandType; // Result type is the same as operand type
		}
		break;
	default:
		// Handle other unary operations as needed
		break;
	}
}
