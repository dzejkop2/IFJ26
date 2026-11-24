/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file testAst.c
 */

/*
AI generated test file, for first implementation and simple testing of code
Prompt: "Implement basic tests for the AST"

Tree built below corresponds to:
import ifj26.*
@main def main(): Unit =
	val x: Int = 1 + 2 * 3
	var s = "a\n"
	s = s + "b"
	if x < 10 then
		print(x)
	else
		()
	for i <- 0 until x by 2 if i != 3 do
		print(s.length)
	while -x >= 0.5 do
		return
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ast.h"
#include "astPrint.h"

static int failures = 0;

#define CHECK(cond)                                                            \
	do {                                                                       \
		if (!(cond)) {                                                         \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
			failures++;                                                        \
		}                                                                      \
	} while (0)

static char *dup(const char *str) {
	char *copy = malloc(strlen(str) + 1);
	if (copy == NULL) {
		exit(99);
	}
	strcpy(copy, str);
	return copy;
}

static astNode *printStmt(astNode *arg, int line) {
	astNode *call =
		astNewCall(astNewIdentifier(dup("print"), line, 9), line, 9);
	astListAppend(&call->data.call.args, arg);
	return astNewExprStmt(call, line, 9);
}

static void testListGrowth(void) {
	astNodeList list;
	astListInit(&list);
	CHECK(list.items == NULL && list.count == 0 && list.capacity == 0);

	for (int i = 0; i < 100; i++) {
		astListAppend(&list, astNewIntLit(i, 1, 1));
	}
	CHECK(list.count == 100);
	CHECK(list.capacity >= 100);
	CHECK(list.items[0]->data.intValue == 0);
	CHECK(list.items[99]->data.intValue == 99);

	for (size_t i = 0; i < list.count; i++) {
		astFree(list.items[i]);
	}
	astListFree(&list);
	CHECK(list.items == NULL && list.count == 0 && list.capacity == 0);
}

static void testTypeFromToken(void) {
	CHECK(astTypeFromToken(dt_int) == TYPE_INT);
	CHECK(astTypeFromToken(dt_double) == TYPE_DOUBLE);
	CHECK(astTypeFromToken(dt_string) == TYPE_STRING);
	CHECK(astTypeFromToken(dt_unit) == TYPE_UNIT);
	CHECK(astTypeFromToken(dt_bool) == TYPE_BOOL);
	CHECK(astTypeFromToken(op_add) == TYPE_NONE);
}

static astNode *buildProgram(void) {
	astNode *program = astNewProgram(dup("ifj26"), 1, 1);

	astNode *mainFn = astNewFunDef(dup("main"), true, 2, 7);
	astListAppend(&mainFn->data.funDef.paramLists, astNewParamList(2, 15));
	mainFn->data.funDef.retType = TYPE_UNIT;
	astNode *body = astNewBlock(3, 5);
	mainFn->data.funDef.body = body;
	astListAppend(&program->data.program.defs, mainFn);

	// val x: Int = 1 + 2 * 3
	astNode *mul = astNewBinary(op_mul, astNewIntLit(2, 3, 22),
								astNewIntLit(3, 3, 26), 3, 24);
	astNode *add = astNewBinary(op_add, astNewIntLit(1, 3, 18), mul, 3, 20);
	astListAppend(&body->data.block.stmts,
				  astNewVarDecl(dup("x"), TYPE_INT, add, false, 3, 5));

	// var s = "a\n"
	astListAppend(&body->data.block.stmts,
				  astNewVarDecl(dup("s"), TYPE_NONE,
								astNewStringLit(dup("a\n"), 4, 13), true, 4,
								5));

	// s = s + "b"
	astNode *concat = astNewBinary(op_add, astNewIdentifier(dup("s"), 5, 9),
								   astNewStringLit(dup("b"), 5, 13), 5, 11);
	astListAppend(&body->data.block.stmts,
				  astNewAssign(astNewIdentifier(dup("s"), 5, 5), concat, 5, 7));

	// if x < 10 then print(x) else ()
	astNode *thenBody = astNewBlock(7, 9);
	astListAppend(&thenBody->data.block.stmts,
				  printStmt(astNewIdentifier(dup("x"), 7, 15), 7));
	astNode *elseBody = astNewBlock(9, 9);
	astListAppend(&elseBody->data.block.stmts,
				  astNewExprStmt(astNewUnitLit(9, 9), 9, 9));
	astNode *cond = astNewBinary(op_lowerThen, astNewIdentifier(dup("x"), 6, 8),
								 astNewIntLit(10, 6, 12), 6, 10);
	astListAppend(&body->data.block.stmts,
				  astNewIf(cond, thenBody, elseBody, 6, 5));

	// for i <- 0 until x by 2 if i != 3 do print(s.length)
	astNode *forLoop = astNewFor(NULL, 10, 5);
	forLoop->data.forLoop.iterName = dup("i");
	forLoop->data.forLoop.from = astNewIntLit(0, 10, 14);
	forLoop->data.forLoop.to = astNewIdentifier(dup("x"), 10, 22);
	forLoop->data.forLoop.step = astNewIntLit(2, 10, 27);
	forLoop->data.forLoop.isInclusive = false;
	astListAppend(&forLoop->data.forLoop.guards,
				  astNewBinary(op_notEqual, astNewIdentifier(dup("i"), 10, 32),
							   astNewIntLit(3, 10, 37), 10, 34));
	astNode *forBody = astNewBlock(11, 9);
	astNode *length =
		astNewMember(astNewIdentifier(dup("s"), 11, 15), dup("length"), 11, 16);
	astListAppend(&forBody->data.block.stmts, printStmt(length, 11));
	forLoop->data.forLoop.body = forBody;
	astListAppend(&body->data.block.stmts, forLoop);

	// while -x >= 0.5 do return
	astNode *neg =
		astNewUnary(op_sub, astNewIdentifier(dup("x"), 12, 12), 12, 11);
	astNode *whileCond =
		astNewBinary(op_biggerEqual, neg, astNewDoubleLit(0.5, 12, 17), 12, 14);
	astNode *whileBody = astNewBlock(13, 9);
	astListAppend(&whileBody->data.block.stmts, astNewReturn(NULL, 13, 9));
	astListAppend(&body->data.block.stmts,
				  astNewWhile(whileCond, whileBody, 12, 5));

	return program;
}

static void testProgram(void) {
	astNode *program = buildProgram();

	CHECK(program->type == AST_PROGRAM);
	CHECK(program->dataType == TYPE_NONE);
	CHECK(program->data.program.defs.count == 1);

	astNode *mainFn = program->data.program.defs.items[0];
	CHECK(mainFn->data.funDef.isMain);
	CHECK(mainFn->data.funDef.paramLists.count == 1);
	CHECK(
		mainFn->data.funDef.paramLists.items[0]->data.paramList.params.count ==
		0);

	astNodeList *stmts = &mainFn->data.funDef.body->data.block.stmts;
	CHECK(stmts->count == 6);
	CHECK(stmts->items[0]->type == AST_VAR_DECL);
	CHECK(stmts->items[0]->line == 3 && stmts->items[0]->col == 5);
	CHECK(stmts->items[3]->type == AST_IF);
	CHECK(stmts->items[4]->type == AST_FOR);
	CHECK(stmts->items[5]->type == AST_WHILE);

	// semantic analysis would fill this in, print shows it as type=Int
	stmts->items[0]->data.varDecl.value->dataType = TYPE_INT;

	astPrint(program, 0);
	astFree(program);
}

int main(void) {
	testListGrowth();
	testTypeFromToken();
	testProgram();
	astFree(NULL);

	if (failures != 0) {
		fprintf(stderr, "%d check(s) failed\n", failures);
		return 1;
	}
	fprintf(stderr, "all checks passed\n");
	return 0;
}
