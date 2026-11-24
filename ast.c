/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file ast.c
 *
 * AI useage:
 * 	"Make tests for simple AST and check for correct filling"
 *  "Make tests that check if every node is being created correctly"
 * 	"Create stub functions for implementation"
 */

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "error.h"

#define LIST_INIT_CAPACITY 4

void astListInit(astNodeList *list) {
	list->items = NULL;
	list->count = 0;
	list->capacity = 0;
}

void astListAppend(astNodeList *list, astNode *node) {
	if (list->count == list->capacity) {
		size_t newCapacity = list->capacity;
		if (newCapacity == 0)
			newCapacity = LIST_INIT_CAPACITY;
		else
			newCapacity *= 2;

		astNode **newCapTemp =
			realloc(list->items, newCapacity * sizeof(astNode *));
		if (newCapTemp == NULL)
			error(INTERNAL_ERROR, "AST node list realloc memory failed");

		list->items = newCapTemp;
		list->capacity = newCapacity;
	}

	list->items[list->count] = node;
	list->count++;
}

void astListFree(astNodeList *list) {
	free(list->items);
	astListInit(list);
}

static void freeNodeList(astNodeList *list) {
	for (size_t i = 0; i < list->count; i++) {
		astFree(list->items[i]);
	}

	astListFree(list);
}

astNode *astNewNode(astNodeType type, int line, int col) {
	astNode *node = calloc(1, sizeof(astNode));
	if (node == NULL)
		error(INTERNAL_ERROR, "AST node malloc memory failed");

	node->type = type;
	node->dataType = TYPE_NONE;
	node->line = line;
	node->col = col;
	return node;
}

void astFree(astNode *node) {
	if (node == NULL)
		return;

	switch (node->type) {
	case AST_PROGRAM:
		free(node->data.program.importName);
		freeNodeList(&node->data.program.defs);
		break;
	case AST_FUN_DEF:
		free(node->data.funcDef.name);
		freeNodeList(&node->data.funcDef.paramLists);
		astFree(node->data.funcDef.body);
		break;
	case AST_EXTENSION:
		astFree(node->data.extension.receiver);
		freeNodeList(&node->data.extension.funDefs);
		break;
	case AST_PARAM_LIST:
		freeNodeList(&node->data.paramList.params);
		break;
	case AST_PARAM:
		free(node->data.param.name);
		break;

	case AST_BLOCK:
		freeNodeList(&node->data.block.stmts);
		break;
	case AST_VAR_DECL:
		free(node->data.varDecl.name);
		astFree(node->data.varDecl.value);
		break;
	case AST_ASSIGN:
		astFree(node->data.assign.target);
		astFree(node->data.assign.value);
		break;
	case AST_RETURN:
		astFree(node->data.ret.value);
		break;
	case AST_WHILE:
		astFree(node->data.whileLoop.cond);
		astFree(node->data.whileLoop.body);
		break;
	case AST_FOR:
		free(node->data.forLoop.iterName);
		astFree(node->data.forLoop.from);
		astFree(node->data.forLoop.to);
		astFree(node->data.forLoop.step);
		freeNodeList(&node->data.forLoop.guards);
		astFree(node->data.forLoop.body);
		break;
	case AST_EXPR_STMT:
		astFree(node->data.exprStmt.expr);
		break;
	case AST_IF:
		astFree(node->data.ifStmt.cond);
		astFree(node->data.ifStmt.thenBody);
		astFree(node->data.ifStmt.elseBody);
		break;

	case AST_LIT_STRING:
		free(node->data.stringValue);
		break;
	case AST_LIT_INT:
	case AST_LIT_DOUBLE:
	case AST_LIT_BOOL:
	case AST_LIT_UNIT:
		break;
	case AST_IDENTIFIER:
		free(node->data.identifier.name);
		break;
	case AST_BINARY_OP:
		astFree(node->data.binary.left);
		astFree(node->data.binary.right);
		break;
	case AST_UNARY_OP:
		astFree(node->data.unary.operand);
		break;
	case AST_CALL:
		astFree(node->data.call.callee);
		freeNodeList(&node->data.call.args);
		break;
	case AST_MEMBER:
		astFree(node->data.member.object);
		free(node->data.member.name);
		break;
	case AST_INTERP:
		freeNodeList(&node->data.interp.parts);
		break;
	}

	free(node);
}

astNode *astNewProgram(char *importName, int line, int col) {
	astNode *node = astNewNode(AST_PROGRAM, line, col);

	node->data.program.importName = importName;
	astListInit(&node->data.program.defs);

	return node;
}

astNode *astNewFuncDef(char *name, bool isMain, int line, int col) {
	astNode *node = astNewNode(AST_FUN_DEF, line, col);

	node->data.funcDef.name = name;
	astListInit(&node->data.funcDef.paramLists);
	node->data.funcDef.retType = TYPE_NONE;
	node->data.funcDef.body = NULL;
	node->data.funcDef.isMain = isMain;

	return node;
}

astNode *astNewExtension(astNode *receiver, int line, int col) {
	astNode *node = astNewNode(AST_EXTENSION, line, col);

	node->data.extension.receiver = receiver;
	astListInit(&node->data.extension.funDefs);

	return node;
}

astNode *astNewParamList(int line, int col) {
	astNode *node = astNewNode(AST_PARAM_LIST, line, col);

	astListInit(&node->data.paramList.params);

	return node;
}

astNode *astNewParam(char *name, astDataType type, int line, int col) {
	astNode *node = astNewNode(AST_PARAM, line, col);

	node->data.param.name = name;
	node->data.param.type = type;

	return node;
}

astNode *astNewBlock(int line, int col) {
	astNode *node = astNewNode(AST_BLOCK, line, col);

	astListInit(&node->data.block.stmts);

	return node;
}

astNode *astNewVarDecl(char *name, astDataType declType, astNode *value,
					   bool isMutable, int line, int col) {
	astNode *node = astNewNode(AST_VAR_DECL, line, col);

	node->data.varDecl.name = name;
	node->data.varDecl.declType = declType;
	node->data.varDecl.value = value;
	node->data.varDecl.isMutable = isMutable;

	return node;
}

astNode *astNewAssign(astNode *target, astNode *value, int line, int col) {
	astNode *node = astNewNode(AST_ASSIGN, line, col);

	node->data.assign.target = target;
	node->data.assign.value = value;

	return node;
}

astNode *astNewReturn(astNode *value, int line, int col) {
	astNode *node = astNewNode(AST_RETURN, line, col);

	node->data.ret.value = value;

	return node;
}

astNode *astNewWhile(astNode *cond, astNode *body, int line, int col) {
	astNode *node = astNewNode(AST_WHILE, line, col);

	node->data.whileLoop.cond = cond;
	node->data.whileLoop.body = body;

	return node;
}

astNode *astNewFor(char *iterName, int line, int col) {
	astNode *node = astNewNode(AST_FOR, line, col);

	node->data.forLoop.iterName = iterName;
	node->data.forLoop.from = NULL;
	node->data.forLoop.to = NULL;
	node->data.forLoop.step = NULL;
	node->data.forLoop.isInclusive = false;
	astListInit(&node->data.forLoop.guards);
	node->data.forLoop.body = NULL;

	return node;
}

astNode *astNewIf(astNode *cond, astNode *thenBody, astNode *elseBody, int line,
				  int col) {
	astNode *node = astNewNode(AST_IF, line, col);

	node->data.ifStmt.cond = cond;
	node->data.ifStmt.thenBody = thenBody;
	node->data.ifStmt.elseBody = elseBody;

	return node;
}

astNode *astNewExprStmt(astNode *expr, int line, int col) {
	astNode *node = astNewNode(AST_EXPR_STMT, line, col);

	node->data.exprStmt.expr = expr;

	return node;
}

astNode *astNewIntLit(long long value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_INT, line, col);

	node->data.intValue = value;

	return node;
}

astNode *astNewDoubleLit(double value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_DOUBLE, line, col);

	node->data.doubleValue = value;

	return node;
}

astNode *astNewStringLit(char *value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_STRING, line, col);

	node->data.stringValue = value;

	return node;
}

astNode *astNewBoolLit(bool value, int line, int col) {
	astNode *node = astNewNode(AST_LIT_BOOL, line, col);

	node->data.boolValue = value;

	return node;
}

astNode *astNewUnitLit(int line, int col) {
	return astNewNode(AST_LIT_UNIT, line, col);
}

astNode *astNewIdentifier(char *name, int line, int col) {
	astNode *node = astNewNode(AST_IDENTIFIER, line, col);

	node->data.identifier.name = name;

	return node;
}

astNode *astNewBinary(specTokenType op, astNode *left, astNode *right, int line,
					  int col) {
	astNode *node = astNewNode(AST_BINARY_OP, line, col);

	node->data.binary.op = op;
	node->data.binary.left = left;
	node->data.binary.right = right;

	return node;
}

astNode *astNewUnary(specTokenType op, astNode *operand, int line, int col) {
	astNode *node = astNewNode(AST_UNARY_OP, line, col);

	node->data.unary.op = op;
	node->data.unary.operand = operand;

	return node;
}

astNode *astNewCall(astNode *callee, int line, int col) {
	astNode *node = astNewNode(AST_CALL, line, col);

	node->data.call.callee = callee;
	astListInit(&node->data.call.args);

	return node;
}

astNode *astNewMember(astNode *object, char *name, int line, int col) {
	astNode *node = astNewNode(AST_MEMBER, line, col);

	node->data.member.object = object;
	node->data.member.name = name;

	return node;
}

astNode *astNewInterp(int line, int col) {
	astNode *node = astNewNode(AST_INTERP, line, col);

	astListInit(&node->data.interp.parts);

	return node;
}

astDataType astTypeFromToken(specTokenType specType) {
	switch (specType) {
	case dt_int:
		return TYPE_INT;
	case dt_double:
		return TYPE_DOUBLE;
	case dt_string:
		return TYPE_STRING;
	case dt_unit:
		return TYPE_UNIT;
	case dt_bool:
		return TYPE_BOOL;
	default:
		return TYPE_NONE;
	}
}
