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

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                 \
        }                                                               \
    } while (0)


static char *dup(const char *str){
    char *copy = malloc(strlen(str) + 1);
    if (copy == NULL){
        exit(99);
    }
    strcpy(copy, str);
    return copy;
}


static astNode *printStmt(astNode *arg, int line){
    astNode *call = ast_newCall(ast_newIdentifier(dup("print"), line, 9), line, 9);
    ast_listAppend(&call->data.call.args, arg);
    return ast_newExprStmt(call, line, 9);
}

static void testListGrowth(void){
    astNodeList list;
    ast_listInit(&list);
    CHECK(list.items == NULL && list.count == 0 && list.capacity == 0);

    for (int i = 0; i < 100; i++){
        ast_listAppend(&list, ast_newIntLit(i, 1, 1));
    }
    CHECK(list.count == 100);
    CHECK(list.capacity >= 100);
    CHECK(list.items[0]->data.intValue == 0);
    CHECK(list.items[99]->data.intValue == 99);

    for (size_t i = 0; i < list.count; i++){
        ast_free(list.items[i]);
    }
    ast_listFree(&list);
    CHECK(list.items == NULL && list.count == 0 && list.capacity == 0);
}

static void testTypeFromToken(void){
    CHECK(ast_typeFromToken(dt_int) == TYPE_INT);
    CHECK(ast_typeFromToken(dt_double) == TYPE_DOUBLE);
    CHECK(ast_typeFromToken(dt_string) == TYPE_STRING);
    CHECK(ast_typeFromToken(dt_unit) == TYPE_UNIT);
    CHECK(ast_typeFromToken(dt_bool) == TYPE_BOOL);
    CHECK(ast_typeFromToken(op_add) == TYPE_NONE);
}

static astNode *buildProgram(void){
    astNode *program = ast_newProgram(dup("ifj26"), 1, 1);

    astNode *mainFn = ast_newFunDef(dup("main"), true, 2, 7);
    ast_listAppend(&mainFn->data.funDef.paramLists, ast_newParamList(2, 15));
    mainFn->data.funDef.retType = TYPE_UNIT;
    astNode *body = ast_newBlock(3, 5);
    mainFn->data.funDef.body = body;
    ast_listAppend(&program->data.program.defs, mainFn);

    // val x: Int = 1 + 2 * 3
    astNode *mul = ast_newBinary(op_mul, ast_newIntLit(2, 3, 22), ast_newIntLit(3, 3, 26), 3, 24);
    astNode *add = ast_newBinary(op_add, ast_newIntLit(1, 3, 18), mul, 3, 20);
    ast_listAppend(&body->data.block.stmts, ast_newVarDecl(dup("x"), TYPE_INT, add, false, 3, 5));

    // var s = "a\n"
    ast_listAppend(&body->data.block.stmts,
                   ast_newVarDecl(dup("s"), TYPE_NONE, ast_newStringLit(dup("a\n"), 4, 13), true, 4, 5));

    // s = s + "b"
    astNode *concat = ast_newBinary(op_add, ast_newIdentifier(dup("s"), 5, 9),
                                    ast_newStringLit(dup("b"), 5, 13), 5, 11);
    ast_listAppend(&body->data.block.stmts,
                   ast_newAssign(ast_newIdentifier(dup("s"), 5, 5), concat, 5, 7));

    // if x < 10 then print(x) else ()
    astNode *thenBody = ast_newBlock(7, 9);
    ast_listAppend(&thenBody->data.block.stmts, printStmt(ast_newIdentifier(dup("x"), 7, 15), 7));
    astNode *elseBody = ast_newBlock(9, 9);
    ast_listAppend(&elseBody->data.block.stmts, ast_newExprStmt(ast_newUnitLit(9, 9), 9, 9));
    astNode *cond = ast_newBinary(op_lowerThen, ast_newIdentifier(dup("x"), 6, 8),
                                  ast_newIntLit(10, 6, 12), 6, 10);
    ast_listAppend(&body->data.block.stmts, ast_newIf(cond, thenBody, elseBody, 6, 5));

    // for i <- 0 until x by 2 if i != 3 do print(s.length)
    astNode *forLoop = ast_newFor(NULL, 10, 5);
    forLoop->data.forLoop.iterName = dup("i");
    forLoop->data.forLoop.from = ast_newIntLit(0, 10, 14);
    forLoop->data.forLoop.to = ast_newIdentifier(dup("x"), 10, 22);
    forLoop->data.forLoop.step = ast_newIntLit(2, 10, 27);
    forLoop->data.forLoop.isInclusive = false;
    ast_listAppend(&forLoop->data.forLoop.guards,
                   ast_newBinary(op_notEqual, ast_newIdentifier(dup("i"), 10, 32),
                                 ast_newIntLit(3, 10, 37), 10, 34));
    astNode *forBody = ast_newBlock(11, 9);
    astNode *length = ast_newMember(ast_newIdentifier(dup("s"), 11, 15), dup("length"), 11, 16);
    ast_listAppend(&forBody->data.block.stmts, printStmt(length, 11));
    forLoop->data.forLoop.body = forBody;
    ast_listAppend(&body->data.block.stmts, forLoop);

    // while -x >= 0.5 do return
    astNode *neg = ast_newUnary(op_sub, ast_newIdentifier(dup("x"), 12, 12), 12, 11);
    astNode *whileCond = ast_newBinary(op_biggerEqual, neg, ast_newDoubleLit(0.5, 12, 17), 12, 14);
    astNode *whileBody = ast_newBlock(13, 9);
    ast_listAppend(&whileBody->data.block.stmts, ast_newReturn(NULL, 13, 9));
    ast_listAppend(&body->data.block.stmts, ast_newWhile(whileCond, whileBody, 12, 5));

    return program;
}

static void testProgram(void){
    astNode *program = buildProgram();

    CHECK(program->type == AST_PROGRAM);
    CHECK(program->dataType == TYPE_NONE);
    CHECK(program->data.program.defs.count == 1);

    astNode *mainFn = program->data.program.defs.items[0];
    CHECK(mainFn->data.funDef.isMain);
    CHECK(mainFn->data.funDef.paramLists.count == 1);
    CHECK(mainFn->data.funDef.paramLists.items[0]->data.paramList.params.count == 0);

    astNodeList *stmts = &mainFn->data.funDef.body->data.block.stmts;
    CHECK(stmts->count == 6);
    CHECK(stmts->items[0]->type == AST_VAR_DECL);
    CHECK(stmts->items[0]->line == 3 && stmts->items[0]->col == 5);
    CHECK(stmts->items[3]->type == AST_IF);
    CHECK(stmts->items[4]->type == AST_FOR);
    CHECK(stmts->items[5]->type == AST_WHILE);

    // semantic analysis would fill this in, print shows it as type=Int
    stmts->items[0]->data.varDecl.value->dataType = TYPE_INT;

    ast_print(program, 0);
    ast_free(program);
}

int main(void){
    testListGrowth();
    testTypeFromToken();
    testProgram();
    ast_free(NULL);

    if (failures != 0){
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    fprintf(stderr, "all checks passed\n");
    return 0;
}
