/*
 * IFJ26 semantic tests based on the AST interface shared in the conversation.
 * Generated with AI assistance (GPT-6).
 * User prompt: "Please make me test for every type of node (success, fail,
 * edge cases) from ast.h" followed by this list:
 * AST_PROGRAM, AST_FUN_DEF, AST_EXTENSION, AST_PARAM_LIST, AST_PARAM,
 * AST_BLOCK, AST_VAR_DECL, AST_ASSIGN, AST_RETURN, AST_WHILE, AST_FOR,
 * AST_EXPR_STMT, AST_IF, AST_LIT_INT, AST_LIT_DOUBLE, AST_LIT_STRING,
 * AST_LIT_BOOL, AST_LIT_UNIT, AST_IDENTIFIER, AST_BINARY_OP, AST_UNARY_OP,
 * AST_CALL, AST_MEMBER.
 * Review test expectations against the official IFJ26 language
 * specification before using in your coursework.
 *
 * Each of the 23 AST node kinds has three cases: success/fail/edge.
 * The compiler under test must call error(...) / exit(nonzero) on failure.
 *
 * Build with project's ast.c, semantic implementation and dependencies.
 * The project currently spells the header sematic.h; change the include below
 * if you rename it to semantic.h.
 */

/*
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
   -I. \
   test/test_semantic_ast.c \
   ast.c semantic.c error.c \
   -o test/test_semantic_ast

python3 test/run_semantic_ast_tests.py \
   ./test/test_semantic_ast
*/

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ast.h"
#ifndef AST_SMOKE_ONLY
#include "../semantic.h"
#endif

typedef enum
{
    PASS = 0,
    FAIL = 1,
    EDGE = 2
} TestMode;
typedef enum
{
    CORE = 0,
    VERIFY_SPEC = 1,
    BONUS = 2
} TestTier;

typedef struct
{
    const char *name;
    astNodeType kind;
    TestTier tier;
    const char *what[3];
} Group;

static const Group groups[] = {
    {"AST_PROGRAM", AST_PROGRAM, CORE, {"valid program with one main function", "wrong root kind (AST_BLOCK)", "program with 32 additional functions"}},
    {"AST_FUN_DEF", AST_FUN_DEF, VERIFY_SPEC, {"function returning an Int expression", "duplicate function name", "Unit function with empty block"}},
    {"AST_EXTENSION", AST_EXTENSION, BONUS, {"extension with receiver and method", "duplicate method names in extension", "extension with no methods"}},
    {"AST_PARAM_LIST", AST_PARAM_LIST, VERIFY_SPEC, {"function with one typed parameter", "duplicate parameter names in same list", "empty parameter list"}},
    {"AST_PARAM", AST_PARAM, VERIFY_SPEC, {"typed parameter referenced by body", "parameter with TYPE_NONE", "very long parameter name"}},
    {"AST_BLOCK", AST_BLOCK, VERIFY_SPEC, {"nested block containing declaration", "identifier referenced outside its inner scope", "empty nested block"}},
    {"AST_VAR_DECL", AST_VAR_DECL, CORE, {"Int variable initialized with Int", "Int variable initialized with String", "declaration with inferred type"}},
    {"AST_ASSIGN", AST_ASSIGN, VERIFY_SPEC, {"assign to mutable variable", "assign to immutable val", "assign to outer variable from inner block"}},
    {"AST_RETURN", AST_RETURN, VERIFY_SPEC, {"Int function returns Int", "Int function returns String", "Unit function returns Unit literal"}},
    {"AST_WHILE", AST_WHILE, CORE, {"comparison condition and empty loop body", "Int used directly as while condition", "nested while loops"}},
    {"AST_FOR", AST_FOR, BONUS, {"integer range with explicit step", "String as range lower bound", "range with omitted step and empty guards"}},
    {"AST_EXPR_STMT", AST_EXPR_STMT, VERIFY_SPEC, {"expression statement with function call", "invalid int + string expression", "unused numeric literal as expression statement"}},
    {"AST_IF", AST_IF, CORE, {"comparison as condition", "Int directly as condition", "if without else"}},
    {"AST_LIT_INT", AST_LIT_INT, CORE, {"Int literal initializes Int", "Int literal initializes String", "LLONG_MAX literal"}},
    {"AST_LIT_DOUBLE", AST_LIT_DOUBLE, CORE, {"Double literal initializes Double", "Double literal initializes Int", "DBL_MAX literal"}},
    {"AST_LIT_STRING", AST_LIT_STRING, CORE, {"String literal initializes String", "String literal initializes Int", "empty String literal"}},
    {"AST_LIT_BOOL", AST_LIT_BOOL, BONUS, {"Boolean true as if condition", "Bool literal initializes Int", "Boolean false as if condition"}},
    {"AST_LIT_UNIT", AST_LIT_UNIT, VERIFY_SPEC, {"Unit function returns Unit literal", "Unit literal initializes Int", "Unit literal used as expression statement"}},
    {"AST_IDENTIFIER", AST_IDENTIFIER, VERIFY_SPEC, {"refer to previously declared identifier", "use undefined identifier", "shadowed identifier used in nested block"}},
    {"AST_BINARY_OP", AST_BINARY_OP, CORE, {"Int + Int", "Int + String", "nested precedence tree (1 + 2 * 3)"}},
    {"AST_UNARY_OP", AST_UNARY_OP, CORE, {"unary minus on Int", "unary minus on String", "nested unary minus on zero"}},
    {"AST_CALL", AST_CALL, VERIFY_SPEC, {"call function with one correct argument", "call unknown function", "call zero-argument function"}},
    {"AST_MEMBER", AST_MEMBER, BONUS, {"call known extension member", "access unknown member", "member call on expression receiver"}},
};

#define GROUP_COUNT (sizeof(groups) / sizeof(groups[0]))

static char *owned(const char *value)
{
    size_t n = strlen(value) + 1;
    char *copy = malloc(n);
    if (copy == NULL)
    {
        fprintf(stderr, "Test setup out of memory\n");
        exit(2);
    }
    memcpy(copy, value, n);
    return copy;
}

static astNode *integer(long long value) { return ast_newIntLit(value, 1, 1); }
static astNode *real(double value) { return ast_newDoubleLit(value, 1, 1); }
static astNode *string(const char *value) { return ast_newStringLit(owned(value), 1, 1); }
static astNode *boolean(bool value) { return ast_newBoolLit(value, 1, 1); }
static astNode *unit(void) { return ast_newUnitLit(1, 1); }
static astNode *ident(const char *name) { return ast_newIdentifier(owned(name), 1, 1); }
static astNode *binary(specTokenType op, astNode *a, astNode *b)
{
    return ast_newBinary(op, a, b, 1, 1);
}
static astNode *unary(specTokenType op, astNode *a)
{
    return ast_newUnary(op, a, 1, 1);
}
static astNode *block(void) { return ast_newBlock(1, 1); }
static astNode *statement(astNode *expr) { return ast_newExprStmt(expr, 1, 1); }
static astNode *decl(const char *name, astDataType type, astNode *value, bool mutable)
{
    return ast_newVarDecl(owned(name), type, value, mutable, 1, 1);
}
static astNode *assign(const char *name, astNode *value)
{
    return ast_newAssign(ident(name), value, 1, 1);
}
static astNode *compare(void) { return binary(op_lowerThen, integer(1), integer(2)); }
static astNode *func(const char *name, astDataType result, astNode *body)
{
    astNode *f = ast_newFunDef(owned(name), false, 1, 1);
    f->data.funDef.retType = result;
    f->data.funDef.body = body;
    return f;
}
static astNode *extension(const char *receiverName)
{
    return ast_newExtension(ast_newParam(owned(receiverName), TYPE_INT, 1, 1), 1, 1);
}
static astNode *call(const char *name)
{
    return ast_newCall(ident(name), 1, 1);
}
static void add(astNodeList *list, astNode *child) { ast_listAppend(list, child); }
static void stmt(astNode *b, astNode *child) { add(&b->data.block.stmts, child); }

static astNode *program(void)
{
    astNode *p = ast_newProgram(owned("ifj26"), 1, 1);
    astNode *m = ast_newFunDef(owned("main"), true, 1, 1);
    m->data.funDef.retType = TYPE_UNIT;
    m->data.funDef.body = block();
    add(&p->data.program.defs, m);
    return p;
}
static astNode *mainBlock(astNode *p)
{
    return p->data.program.defs.items[0]->data.funDef.body;
}
static astNode *withStmt(astNode *s)
{
    astNode *p = program();
    stmt(mainBlock(p), s);
    return p;
}
static astNode *withDecl(astDataType type, astNode *value)
{
    return withStmt(decl("x", type, value, true));
}
static void addFunc(astNode *p, astNode *f)
{
    add(&p->data.program.defs, f);
}
static void param(astNode *function, const char *name, astDataType type)
{
    astNode *list;
    if (function->data.funDef.paramLists.count == 0)
    {
        list = ast_newParamList(1, 1);
        add(&function->data.funDef.paramLists, list);
    }
    else
    {
        list = function->data.funDef.paramLists.items[0];
    }
    add(&list->data.paramList.params, ast_newParam(owned(name), type, 1, 1));
}

static astNode *build(astNodeType kind, TestMode mode)
{
    astNode *p, *f, *b, *loop, *e, *c, *ext;

    switch (kind)
    {
    case AST_PROGRAM:
        if (mode == FAIL)
            return block(); /* wrong root: semanticAnalyze must reject */
        p = program();
        if (mode == EDGE)
        {
            for (int i = 0; i < 32; i++)
            {
                char name[32];
                snprintf(name, sizeof(name), "fn_%02d", i);
                addFunc(p, func(name, TYPE_INT, integer(i)));
            }
        }
        return p;

    case AST_FUN_DEF:
        p = program();
        addFunc(p, func("f", TYPE_INT, integer(7)));
        if (mode == FAIL)
            addFunc(p, func("f", TYPE_INT, integer(8)));
        if (mode == EDGE)
            addFunc(p, func("empty", TYPE_UNIT, block()));
        return p;

    case AST_EXTENSION:
        p = program();
        ext = extension("self");
        if (mode != EDGE)
            add(&ext->data.extension.funDefs, func("m", TYPE_INT, integer(2)));
        if (mode == FAIL)
            add(&ext->data.extension.funDefs, func("m", TYPE_INT, integer(3)));
        add(&p->data.program.defs, ext);
        return p;

    case AST_PARAM_LIST:
        p = program();
        f = func("f", TYPE_INT, integer(2));
        if (mode != EDGE)
        {
            param(f, "a", TYPE_INT);
        }
        else
        {
            add(&f->data.funDef.paramLists, ast_newParamList(1, 1));
        }
        if (mode == FAIL)
            param(f, "a", TYPE_INT);
        addFunc(p, f);
        return p;

    case AST_PARAM:
        p = program();
        f = func("f", TYPE_INT, integer(1));
        if (mode == EDGE)
        {
            char name[256];
            memset(name, 'p', sizeof(name) - 1);
            name[255] = '\0';
            param(f, name, TYPE_INT);
        }
        else
        {
            param(f, "arg", mode == FAIL ? TYPE_NONE : TYPE_INT);
            if (mode == PASS)
            {
                ast_free(f->data.funDef.body);
                f->data.funDef.body = ident("arg");
            }
        }
        addFunc(p, f);
        return p;

    case AST_BLOCK:
        p = program();
        b = block();
        if (mode == PASS)
            stmt(b, decl("x", TYPE_INT, integer(1), true));
        if (mode == FAIL)
            stmt(b, decl("hidden", TYPE_INT, integer(1), true));
        stmt(mainBlock(p), ast_newIf(compare(), b, NULL, 1, 1));
        if (mode == FAIL)
            stmt(mainBlock(p), statement(ident("hidden")));
        return p;

    case AST_VAR_DECL:
        if (mode == PASS)
            return withDecl(TYPE_INT, integer(12));
        if (mode == FAIL)
            return withDecl(TYPE_INT, string("oops"));
        return withDecl(TYPE_NONE, integer(12));

    case AST_ASSIGN:
        p = program();
        b = mainBlock(p);
        stmt(b, decl("x", TYPE_INT, integer(1), mode != FAIL));
        if (mode == EDGE)
        {
            e = block();
            stmt(e, assign("x", integer(8)));
            stmt(b, ast_newIf(compare(), e, NULL, 1, 1));
        }
        else
        {
            stmt(b, assign("x", integer(8)));
        }
        return p;

    case AST_RETURN:
        p = program();
        f = func("f", mode == EDGE ? TYPE_UNIT : TYPE_INT,
                 mode == FAIL ? ast_newReturn(string("bad"), 1, 1) : ast_newReturn(mode == EDGE ? unit() : integer(1), 1, 1));
        addFunc(p, f);
        return p;

    case AST_WHILE:
        loop = ast_newWhile(mode == FAIL ? integer(1) : compare(), block(), 1, 1);
        if (mode == EDGE)
            stmt(loop->data.whileLoop.body, ast_newWhile(compare(), block(), 1, 1));
        return withStmt(loop);

    case AST_FOR:
        loop = ast_newFor(owned("i"), 1, 1);
        loop->data.forLoop.from = mode == FAIL ? string("bad") : integer(0);
        loop->data.forLoop.to = integer(3);
        if (mode != EDGE)
            loop->data.forLoop.step = integer(1);
        loop->data.forLoop.body = block();
        return withStmt(loop);

    case AST_EXPR_STMT:
        if (mode == FAIL)
            return withStmt(statement(binary(op_add, integer(1), string("bad"))));
        if (mode == EDGE)
            return withStmt(statement(integer(3)));
        p = program();
        addFunc(p, func("f", TYPE_UNIT, block()));
        stmt(mainBlock(p), statement(call("f")));
        return p;

    case AST_IF:
        return withStmt(ast_newIf(mode == FAIL ? integer(5) : compare(),
                                  block(), mode == EDGE ? NULL : block(), 1, 1));

    case AST_LIT_INT:
        if (mode == FAIL)
            return withDecl(TYPE_STRING, integer(12));
        return withDecl(TYPE_INT, integer(mode == EDGE ? LLONG_MAX : 12));

    case AST_LIT_DOUBLE:
        if (mode == FAIL)
            return withDecl(TYPE_INT, real(2.5));
        return withDecl(TYPE_DOUBLE, real(mode == EDGE ? DBL_MAX : 2.5));

    case AST_LIT_STRING:
        if (mode == FAIL)
            return withDecl(TYPE_INT, string("hello"));
        return withDecl(TYPE_STRING, string(mode == EDGE ? "" : "hello"));

    case AST_LIT_BOOL:
        if (mode == FAIL)
            return withDecl(TYPE_INT, boolean(true));
        return withStmt(ast_newIf(boolean(mode == PASS), block(), NULL, 1, 1));

    case AST_LIT_UNIT:
        if (mode == FAIL)
            return withDecl(TYPE_INT, unit());
        if (mode == EDGE)
            return withStmt(statement(unit()));
        p = program();
        addFunc(p, func("f", TYPE_UNIT, ast_newReturn(unit(), 1, 1)));
        return p;

    case AST_IDENTIFIER:
        if (mode == FAIL)
            return withStmt(statement(ident("not_defined")));
        p = program();
        b = mainBlock(p);
        stmt(b, decl("x", TYPE_INT, integer(1), true));
        if (mode == EDGE)
        {
            e = block();
            stmt(e, decl("x", TYPE_INT, integer(2), true));
            stmt(e, statement(ident("x")));
            stmt(b, ast_newIf(compare(), e, NULL, 1, 1));
        }
        else
        {
            stmt(b, statement(ident("x")));
        }
        return p;

    case AST_BINARY_OP:
        if (mode == FAIL)
            return withDecl(TYPE_INT, binary(op_add, integer(1), string("bad")));
        e = mode == EDGE ? binary(op_add, integer(1), binary(op_mul, integer(2), integer(3))) : binary(op_add, integer(1), integer(2));
        return withDecl(TYPE_INT, e);

    case AST_UNARY_OP:
        if (mode == FAIL)
            return withDecl(TYPE_INT, unary(op_sub, string("bad")));
        e = mode == EDGE ? unary(op_sub, unary(op_sub, integer(0))) : unary(op_sub, integer(1));
        return withDecl(TYPE_INT, e);

    case AST_CALL:
        if (mode == FAIL)
            return withStmt(statement(call("unknown_function")));
        p = program();
        f = func("f", TYPE_INT, integer(1));
        if (mode == PASS)
            param(f, "n", TYPE_INT);
        addFunc(p, f);
        c = call("f");
        if (mode == PASS)
            add(&c->data.call.args, integer(4));
        stmt(mainBlock(p), statement(c));
        return p;

    case AST_MEMBER:
        p = program();
        stmt(mainBlock(p), decl("obj", TYPE_INT, integer(3), true));
        if (mode == PASS)
        {
            ext = extension("self");
            add(&ext->data.extension.funDefs, func("m", TYPE_INT, integer(1)));
            add(&p->data.program.defs, ext);
            e = ast_newMember(ident("obj"), owned("m"), 1, 1);
            c = ast_newCall(e, 1, 1);
            stmt(mainBlock(p), statement(c));
        }
        else if (mode == FAIL)
        {
            stmt(mainBlock(p), statement(ast_newMember(ident("obj"), owned("missing"), 1, 1)));
        }
        else
        {
            ext = extension("self");
            add(&ext->data.extension.funDefs, func("m", TYPE_INT, integer(1)));
            add(&p->data.program.defs, ext);
            e = ast_newMember(binary(op_add, integer(1), integer(2)), owned("m"), 1, 1);
            stmt(mainBlock(p), statement(ast_newCall(e, 1, 1)));
        }
        return p;

    default:
        fprintf(stderr, "Unsupported AST test kind %d\n", (int)kind);
        exit(2);
    }
}

static const char *tierName(TestTier tier)
{
    return tier == CORE ? "core" : tier == VERIFY_SPEC ? "verify-spec"
                                                       : "bonus";
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--list") == 0)
    {
        for (size_t i = 0; i < GROUP_COUNT; ++i)
        {
            for (int mode = 0; mode < 3; ++mode)
            {
                const char *tag = mode == PASS ? "success" : mode == FAIL ? "fail"
                                                                          : "edge";
                const char *expected = mode == FAIL ? "nonzero" : "zero";
                printf("%zu\t%s\t%s\t%s\t%s\t%s\n",
                       i * 3 + (size_t)mode, groups[i].name, tag, expected,
                       tierName(groups[i].tier), groups[i].what[mode]);
            }
        }
        return 0;
    }

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s --list | CASE_ID\n", argv[0]);
        return 2;
    }
    char *end = NULL;
    long index = strtol(argv[1], &end, 10);
    if (*argv[1] == '\0' || *end != '\0' || index < 0 || (unsigned long)index >= GROUP_COUNT * 3)
    {
        fprintf(stderr, "Bad CASE_ID; use --list\n");
        return 2;
    }
    const Group *g = &groups[(size_t)index / 3];
    TestMode mode = (TestMode)((size_t)index % 3);
    astNode *root = build(g->kind, mode);
#ifndef AST_SMOKE_ONLY
    semanticAnalyze(root);
#endif
    ast_free(root);
    return 0;
}