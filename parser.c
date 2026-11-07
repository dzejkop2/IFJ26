/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file parser.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "error.h"
#include <string.h>

static parserState parser;

/*
 * DEBUG PRINTS
 * Set PARSER_DEBUG to 0 (or compile with -DPARSER_DEBUG=0) to turn them off.
 * They go to stderr, so they never mix with the generated code on stdout.
 */
#ifndef PARSER_DEBUG
#define PARSER_DEBUG 1
#endif

#if PARSER_DEBUG
#include "test/tokenDump.h"

static void parserDebugToken(const char *label, token tok) {
    fprintf(stderr, "%s %s", label, specName(tok.specType));

    if ((tok.specType == id_idtName || tok.specType == lit_string || tok.specType == intp_part) && tok.value.word != NULL) {
        fprintf(stderr, " '%s'", tok.value.word);
    } else if (tok.specType == lit_int) {
        fprintf(stderr, " %lld", tok.value.num);
    } else if (tok.specType == lit_double) {
        fprintf(stderr, " %g", tok.value.decNum);
    }

    fprintf(stderr, " (%d:%d)\n", tok.line, tok.col);
}

// prints the function that is being entered and the token it starts on
#define PARSER_TRACE() do { \
        fprintf(stderr, "[parser] >> %-16s", __func__); \
        parserDebugToken(" at", parser.current); \
    } while (0)
// prints every token the parser reads
#define PARSER_DEBUG_TOKEN(tok) do { \
        fprintf(stderr, "[parser]    "); \
        parserDebugToken("token", (tok)); \
    } while (0)
#else
#define PARSER_TRACE() ((void)0)
#define PARSER_DEBUG_TOKEN(tok) ((void)0)
#endif


typedef enum {
    PREC_ADD, PREC_SUB, PREC_MUL, PREC_DIV,
    PREC_LT, PREC_LE, PREC_GT, PREC_GE,
    PREC_EQ, PREC_NE,
    PREC_AND, PREC_OR,
    PREC_UPLUS, PREC_UMINUS, PREC_NOT,
    PREC_LBR, PREC_RBR,
    PREC_ID,
    PREC_END,
    PREC_COUNT,

    PREC_CALL = PREC_COUNT,
    PREC_MEMBER,
} precIndex;

const char precedenceTable[PREC_COUNT][PREC_COUNT] = {
    /*        +    -    *    /    <    <=   >    >=   ==   !=   &&   ||   u+   u-   !    (    )    id   $  */
    /* +  */ {'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* -  */ {'>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* *  */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* /  */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* <  */ {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* <= */ {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* >  */ {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* >= */ {'<', '<', '<', '<', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* == */ {'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* != */ {'<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* && */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* || */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* u+ */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* u- */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* !  */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '<', '<', '>', '<', '>'},
    /* (  */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', ' '},
    /* )  */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', '>', ' ', '>'},
    /* id */ {'>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', '>', ' ', '>'},
    /* $  */ {'<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', '<', ' '},
};

void parserCleanup(void) {
    if (!parser.active) return;
    parser.active = false;

    while (parser.unattachedCount > 0) {
        parser.unattachedCount--;
        ast_free(parser.unattached[parser.unattachedCount]);
    }
    free(parser.unattached);
    parser.unattached = NULL;
    parser.unattachedCapacity = 0;

    while (!isEmpty(&parser.exprStack)) pop(&parser.exprStack);
}


astNode *parserTrackUnattached(astNode *node) {
    if (node == NULL) return NULL;

    if (parser.unattachedCount == parser.unattachedCapacity) {
        size_t newCapacity = parser.unattachedCapacity == 0 ? 16 : parser.unattachedCapacity * 2;
        astNode **newUnattached = realloc(parser.unattached, newCapacity * sizeof(astNode *));
        if (newUnattached == NULL) {
            ast_free(node);
            error(INTERNAL_ERROR, "Parser unattached node stack realloc memory failed");
        }
        parser.unattached = newUnattached;
        parser.unattachedCapacity = newCapacity;
    }

    parser.unattached[parser.unattachedCount++] = node;
    return node;
}

astNode *parserMarkAttached(astNode *node) {
    if (node == NULL) return NULL;

    for (size_t i = parser.unattachedCount; i > 0; i--) {
        if (parser.unattached[i - 1] == node) {
            memmove(&parser.unattached[i - 1], &parser.unattached[i], (parser.unattachedCount - i) * sizeof(astNode *));
            parser.unattachedCount--;
            return node;
        }
    }
    return node;
}

void parserListAppend(astNodeList *list, astNode *node) {
    ast_listAppend(list, node);
    parserMarkAttached(node);
}

astNode *parse(void) {
    static bool cleanupRegistered = false;
    if (!cleanupRegistered) {
        if (atexit(parserCleanup) != 0) error(INTERNAL_ERROR, "Failed to register parser cleanup");
        cleanupRegistered = true;
    }

    memset(&parser, 0, sizeof(parser));
    parser.active = true;

    scannerInit();

    astNode *program = parseProgram();

    if(program == NULL) {
        error(SYNTAX_ERROR, "Invalid program structure");
    } else {
        printf("Program parsed successfully.\n");
    }

    parserMarkAttached(program);
    parserCleanup();
    scannerFree();
    return program;
}


void parserAdvance(void) {
    parser.current = getToken();
    PARSER_DEBUG_TOKEN(parser.current);
}

bool parserCheck(specTokenType specType) {
    return parser.current.specType == specType;
}

bool parserMatch(specTokenType specType) {
    if (!parserCheck(specType)) return false;
    parserAdvance();
    return true;
}

void skipNL(void) {
    while(parserCheck(ps_newLine)) {
        parserAdvance();
    }
}

astNode *parseProgram(void) {
    PARSER_TRACE();
    parserAdvance();
    skipNL();

    if(!parserCheck(kw_import)) error(SYNTAX_ERROR, "Expected 'import' at the start of the program");
    int line = parser.current.line;
    int col = parser.current.col;
    parserAdvance();

    if(!parserCheck(id_idtName) || strcmp(parser.current.value.word, "ifj26") != 0)
        error(SYNTAX_ERROR, "Expected 'ifj26' after 'import'");

    astNode *program = parserTrackUnattached(ast_newProgram(NULL, line, col));
    program->data.program.importName = parser.current.value.word;
    parserAdvance();

    if(!parserMatch(sn_dot)) error(SYNTAX_ERROR, "Expected '.' after 'import ifj26'");
    if(!parserMatch(op_mul)) error(SYNTAX_ERROR, "Expected '*' after 'import ifj26.'");
    if(!parserCheck(ps_newLine)) error(SYNTAX_ERROR, "Expected newline after 'import ifj26.*'");

    parseGlobalDefs(&program->data.program.defs);

    if(!parserCheck(ps_eof)) error(SYNTAX_ERROR, "Expected end of file");

    return program;
}

void parseGlobalDefs(astNodeList *defs) {
    PARSER_TRACE();
    bool mainFound = false;

    skipNL();
    while(!parserCheck(ps_eof)) {
        astNode *def = parseGlobalDef();
        if(def == NULL) error(SYNTAX_ERROR, "Invalid global definition");

        parserListAppend(defs, def);
        parserMarkAttached(def);

        if(def->type == AST_FUN_DEF && def->data.funcDef.isMain) {
            if(mainFound) {
                error(SYNTAX_ERROR, "Multiple main functions found");
            }
            mainFound = true;
        }

        skipNL();
    }
}

astNode *parseGlobalDef(void) {
    PARSER_TRACE();
    if(parserCheck(kw_extension)) {
        return parseExtension();
    } else if(parserCheck(kw_def)) {
        return parseFuncDef(false);
    } else if(parserCheck(kw_main)) {
        parserAdvance();
        skipNL();
        return parseFuncDef(true);
    }

    return NULL;
}

astNode *parseExtension(void) {
    PARSER_TRACE();
    int line = parser.current.line;
    int col = parser.current.col;
    parserAdvance();

    if(!parserMatch(br_start)) error(SYNTAX_ERROR, "Expected '(' after 'extension'");
    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected a identifier in extension");

    astNode *extension = parserTrackUnattached(ast_newExtension(NULL, line, col));
    extension->data.extension.receiver = parserMarkAttached(parseParam());

    if(!parserMatch(br_end)) error(SYNTAX_ERROR, "Expected ')' after extension");

    parseExtBody(extension);

    return extension;
}

void parseExtBody(astNode *extension) {
    PARSER_TRACE();

    astNodeList *funcDefs = &extension->data.extension.funDefs;

    // ExtBody -> ps_newLine ps_indent ExtFuncDef ExtFuncDefs ps_dedent
    if(parserMatch(ps_newLine)) {
        if(!parserMatch(ps_indent)) error(SYNTAX_ERROR, "Expected indented extension body");

        // ExtFuncDefs -> ExtFuncDef ExtFuncDefs | ε   (at least one function before it)
        do {
            if(!parserCheck(kw_def)) error(SYNTAX_ERROR, "Expected 'def' in extension body");

            parserListAppend(funcDefs, parseFuncDef(false));
            skipNL();
        } while(!parserCheck(ps_dedent) && !parserCheck(ps_eof));

        if(!parserMatch(ps_dedent)) error(SYNTAX_ERROR, "Expected end of extension body");
        return;
    }

    // ExtBody -> ExtFuncDef   (extension (x: Int) def foo: Int = x on one line)
    if(!parserCheck(kw_def)) error(SYNTAX_ERROR, "Expected 'def' after extension");
    parserListAppend(funcDefs, parseFuncDef(false));
}

astNode *parseFuncDef(bool isMain) {
    PARSER_TRACE();
    if(!parserMatch(kw_def)) error(SYNTAX_ERROR, "Expected 'def'");
    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected function name after 'def'");
    
    astNode *funcDef = parserTrackUnattached(ast_newFuncDef(parser.current.value.word, isMain, parser.current.line, parser.current.col));
    parserAdvance();

    // ParamLists can be empty: def answer: Int = 42

    parseParamLists(&funcDef->data.funcDef.paramLists);

    funcDef->data.funcDef.retType = parseTypeOpt();

    if(!parserMatch(op_assign)) error(SYNTAX_ERROR, "Expected '=' in function definition");

    if(parserMatch(ps_newLine)) {
        if(!parserMatch(ps_indent)) error(SYNTAX_ERROR, "Expected indented function body");

        funcDef->data.funcDef.body = parserMarkAttached(parseBlock());
        if(funcDef->data.funcDef.body == NULL) error(SYNTAX_ERROR, "Expected function body");

        if(!parserMatch(ps_dedent)) error(SYNTAX_ERROR, "Expected end of function body");
    }
    else {
        funcDef->data.funcDef.body = parserMarkAttached(parseValue());
        if(funcDef->data.funcDef.body == NULL) error(SYNTAX_ERROR, "Expected function body");
    }

    return funcDef;
}

void parseParamLists(astNodeList *paramLists) {
    PARSER_TRACE();
    while(parserCheck(br_start)) {
        astNode *paramList = parserTrackUnattached(ast_newParamList(parser.current.line, parser.current.col));
        parserListAppend(paramLists, paramList);
        parserAdvance();

        parseParams(&paramList->data.paramList.params);

        if(!parserMatch(br_end)) error(SYNTAX_ERROR, "Expected ')' to end parameter list");
    }
}

void parseParams(astNodeList *params) {
    PARSER_TRACE();
    if(!parserCheck(id_idtName)) return;

    parserListAppend(params, parseParam());

    while(parserMatch(sn_comma)) {
        if(!parserCheck(id_idtName)) break;

        parserListAppend(params, parseParam());
    }
}

astNode *parseParam(void) {
    PARSER_TRACE();
    if(!parserCheck(id_idtName)) return NULL;

    astNode *param = parserTrackUnattached(ast_newParam(NULL, TYPE_NONE, parser.current.line, parser.current.col));
    param->data.param.name = parser.current.value.word;
    parserAdvance();

    if(!parserMatch(sn_colon)) error(SYNTAX_ERROR, "Expected ':' after parameter name");

    param->data.param.type = parseType();
    if(param->data.param.type == TYPE_NONE) error(SYNTAX_ERROR, "Expected type of parameter");

    return param;
}

astDataType parseTypeOpt(void) {
    PARSER_TRACE();
    if(!parserMatch(sn_colon)) return TYPE_NONE;

    astDataType type = parseType();
    if(type == TYPE_NONE) error(SYNTAX_ERROR, "Expected type after ':'");
    return type;
}

astDataType parseType(void) {
    PARSER_TRACE();
    astDataType type = ast_typeFromToken(parser.current.specType);
    if(type != TYPE_NONE) parserAdvance();
    return type;
}

astNode *parseBody(void) {
    PARSER_TRACE();

    if(parserMatch(ps_newLine)) {
        if(!parserMatch(ps_indent)) error(SYNTAX_ERROR, "Expected indented block");

        astNode *block = parseBlock();
        if(!parserMatch(ps_dedent)) error(SYNTAX_ERROR, "Expected dedent after block");
        return block;
    }

    return parseStmt();
}

astNode *parseBlock(void) {
    PARSER_TRACE();

    astNode *block = parserTrackUnattached(ast_newBlock(parser.current.line, parser.current.col));

    skipNL();
    while(!parserCheck(ps_dedent) && !parserCheck(ps_eof)) {
        parserListAppend(&block->data.block.stmts, parseStmt());
        skipNL();
    }

    return block;
}

astNode *parseStmt(void) {
    PARSER_TRACE();

    astNode *stmt = NULL;

    if(parserCheck(kw_var)) {
        stmt = parseVarDecl(true);
    } else if(parserCheck(kw_val)) {
        stmt = parseVarDecl(false);
    } else if(parserCheck(kw_return)) {
        stmt = parseReturn();
    } else if(parserCheck(kw_while)) {
        stmt = parseWhile();
    } else if(parserCheck(kw_if)) {
        stmt = parseIf();
    } else if(parserCheck(kw_for)) {
        stmt = parseFor();
    } else {
        stmt = parseSimple();
    }

    if(stmt == NULL) error(SYNTAX_ERROR, "Invalid statement");

    return stmt;
}

astNode *parseSimple(void) {
    PARSER_TRACE();

    astNode *expr = parseExpr();
    if(expr == NULL) error(SYNTAX_ERROR, "Expected expression in statement");

    if(parserCheck(op_assign)) {
        astNode *assign = parserTrackUnattached(ast_newAssign(expr, NULL, parser.current.line, parser.current.col));
        parserMarkAttached(expr);
        parserAdvance();

        assign->data.assign.value = parserMarkAttached(parseValue());
        if(assign->data.assign.value == NULL) error(SYNTAX_ERROR, "Expected value in assignment");

        return assign;
    }

    astNode *exprStmt = parserTrackUnattached(ast_newExprStmt(expr, expr->line, expr->col));
    parserMarkAttached(expr);

    if(!parserCheck(ps_newLine) && !parserCheck(sn_semiColon))
        error(SYNTAX_ERROR, "Expected end of line after statement");
    parserAdvance();
    return exprStmt;
}

astNode *parseInline(void) {
    PARSER_TRACE();

    if(parserCheck(kw_return)) {
        astNode *ret = parserTrackUnattached(ast_newReturn(NULL, parser.current.line, parser.current.col));
        parserAdvance();

        ret->data.ret.value = parserMarkAttached(parseExpr());
        return ret;
    }

    astNode *expr = parseExpr();
    if(expr == NULL) return NULL;

    if(parserCheck(op_assign)) {
        astNode *assign = parserTrackUnattached(ast_newAssign(expr, NULL, parser.current.line, parser.current.col));
        parserMarkAttached(expr);
        parserAdvance();

        assign->data.assign.value = parserMarkAttached(parseExpr());
        if(assign->data.assign.value == NULL) error(SYNTAX_ERROR, "Expected value in assignment");

        return assign;
    }

    astNode *exprStmt = parserTrackUnattached(ast_newExprStmt(expr, expr->line, expr->col));
    parserMarkAttached(expr);
    return exprStmt;
}

astNode *parseVarDecl(bool isMutable) {
    PARSER_TRACE();
    
    parserAdvance();
    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected variable name after 'var' or 'val'");
    astNode *var = parserTrackUnattached(ast_newVarDecl(parser.current.value.word, TYPE_NONE, NULL, isMutable, parser.current.line, parser.current.col));
    parserAdvance();

    var->data.varDecl.declType = parseTypeOpt();

    if(!parserMatch(op_assign)) error(SYNTAX_ERROR, "Expected '=' in variable declaration");

    var->data.varDecl.value = parserMarkAttached(parseValue());
    if(var->data.varDecl.value == NULL) error(SYNTAX_ERROR, "Expected value in variable declaration");

    return var;
}

astNode *parseReturn(void) {
    PARSER_TRACE();

    astNode *ret = parserTrackUnattached(ast_newReturn(NULL, parser.current.line, parser.current.col));
    parserAdvance();

    if(parserCheck(ps_newLine) || parserCheck(sn_semiColon)) {
        ret->data.ret.value = NULL;
        parserAdvance();
        return ret;
    }

    ret->data.ret.value = parserMarkAttached(parseValue());
    return ret;
}

astNode *parseWhile(void) {
    PARSER_TRACE();
    astNode *whileNode = parserTrackUnattached(ast_newWhile(NULL,NULL, parser.current.line, parser.current.col));
    parserAdvance();

    whileNode->data.whileLoop.cond = parserMarkAttached(parseExpr());
    if(whileNode->data.whileLoop.cond == NULL) error(SYNTAX_ERROR, "Expected condition in while loop");

    if(parserMatch(kw_do) != true) error(SYNTAX_ERROR, "Expected 'do' after while loop condition");
    
    whileNode->data.whileLoop.body = parserMarkAttached(parseBody());
    if(whileNode->data.whileLoop.body == NULL) error(SYNTAX_ERROR, "Expected body in while loop");  

    return whileNode;
}

astNode *parseValue(void) {
    PARSER_TRACE();

    if(parserCheck(kw_if)) {
        astNode* value = parseIf();

        if(value->data.ifStmt.elseBody == NULL) error(SYNTAX_ERROR, "Expected else statement when if is used as a value");

        return value;
    }
    astNode* value = parseExpr();

    if(value != NULL && !parserMatch(ps_newLine) && !parserMatch(sn_semiColon))
        error(SYNTAX_ERROR, "Expected end of line after value");

    return value;
}

astNode *parseIf(void) {
    PARSER_TRACE();

    astNode *ifNode = parserTrackUnattached(ast_newIf(NULL, NULL, NULL, parser.current.line, parser.current.col));
    parserAdvance();

    ifNode->data.ifStmt.cond = parserMarkAttached(parseExpr());
    if(ifNode->data.ifStmt.cond == NULL) error(SYNTAX_ERROR, "Expected condition in if statement");

    if(!parserMatch(kw_then)) error(SYNTAX_ERROR, "Expected 'then' after if condition");

    if(parserCheck(ps_newLine)) {
        ifNode->data.ifStmt.thenBody = parserMarkAttached(parseBody());

        if(!parserMatch(kw_else)) return ifNode;
    } else {
        ifNode->data.ifStmt.thenBody = parserMarkAttached(parseInline());
        if(ifNode->data.ifStmt.thenBody == NULL) error(SYNTAX_ERROR, "Expected statement after 'then'");

        if(parserMatch(ps_newLine) || parserMatch(sn_semiColon)) return ifNode;

        if(!parserMatch(kw_else)) error(SYNTAX_ERROR, "Expected 'else' or end of line after then branch");
    }

    ifNode->data.ifStmt.elseBody = parserMarkAttached(parseBody());

    return ifNode;
}

// Used for For loop special words
bool parserCheckWord(const char *word) {
    return parserCheck(id_idtName) && strcmp(parser.current.value.word, word) == 0;
}

astNode *parseFor(void) {
    PARSER_TRACE();

    astNode *forNode = parserTrackUnattached(ast_newFor(NULL, parser.current.line, parser.current.col));
    parserAdvance();

    parseForHeader(forNode);

    if(!parserMatch(kw_do)) error(SYNTAX_ERROR, "Expected 'do' after for header");

    forNode->data.forLoop.body = parserMarkAttached(parseBody());
    if(forNode->data.forLoop.body == NULL) error(SYNTAX_ERROR, "Expected body in for loop");

    return forNode;
}

void parseForHeader(astNode *forNode) {
    PARSER_TRACE();

    if(parserMatch(ps_newLine)) {
        if(!parserMatch(ps_indent)) error(SYNTAX_ERROR, "Expected indented for header");

        parseGenerator(forNode);
        if(!parserMatch(ps_newLine)) error(SYNTAX_ERROR, "Expected end of line after for generator");

        parseGuardLines(&forNode->data.forLoop.guards);

        if(!parserMatch(ps_dedent)) error(SYNTAX_ERROR, "Expected end of for header");
        return;
    }

    parseGenerator(forNode);
    parseGuards(&forNode->data.forLoop.guards);
}

void parseGenerator(astNode *forNode) {
    PARSER_TRACE();

    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected loop variable after 'for'");
    forNode->data.forLoop.iterName = parser.current.value.word;
    parserAdvance();

    if(!parserMatch(ct_arrowLeft)) error(SYNTAX_ERROR, "Expected '<-' after loop variable");

    forNode->data.forLoop.from = parserMarkAttached(parseExpr());
    if(forNode->data.forLoop.from == NULL) error(SYNTAX_ERROR, "Expected start of range in for");

    if(parserCheckWord("to")) {
        forNode->data.forLoop.isInclusive = true;
    } else if(parserCheckWord("until")) {
        forNode->data.forLoop.isInclusive = false;
    } else {
        error(SYNTAX_ERROR, "Expected 'to' or 'until' in for range");
    }
    parserAdvance();

    forNode->data.forLoop.to = parserMarkAttached(parseExpr());
    if(forNode->data.forLoop.to == NULL) error(SYNTAX_ERROR, "Expected end of range in for");

    if(parserCheckWord("by")) {
        parserAdvance();

        forNode->data.forLoop.step = parserMarkAttached(parseExpr());
        if(forNode->data.forLoop.step == NULL) error(SYNTAX_ERROR, "Expected step after 'by'");
    }
}

void parseGuards(astNodeList *guards) {
    PARSER_TRACE();

    while(parserMatch(kw_if)) {
        astNode *guard = parseExpr();
        if(guard == NULL) error(SYNTAX_ERROR, "Expected condition after 'if' in for");

        parserListAppend(guards, guard);
    }
}

void parseGuardLines(astNodeList *guards) {
    PARSER_TRACE();

    while(parserMatch(kw_if)) {
        astNode *guard = parseExpr();
        if(guard == NULL) error(SYNTAX_ERROR, "Expected condition after 'if' in for");

        parserListAppend(guards, guard);

        if(!parserMatch(ps_newLine)) error(SYNTAX_ERROR, "Expected end of line after for guard");
    }
}

bool exprIsTerm(const exprSymbol *symbol, int prec) {
    return symbol->kind == EXPR_TERM && symbol->prec == prec;
}

int exprTopTerminalPrec(void) {
    if (top(&parser.exprStack)->kind == EXPR_TERM) return top(&parser.exprStack)->prec;

    exprSymbol e = pop(&parser.exprStack);
    int prec = top(&parser.exprStack)->prec;
    push(&parser.exprStack, e);
    return prec;
}

bool exprAfterOperand(void) {
    const exprSymbol *topSymbol = top(&parser.exprStack);
    return topSymbol->kind == EXPR_NONTERM || exprIsTerm(topSymbol, PREC_ID) || exprIsTerm(topSymbol, PREC_RBR);
}

int exprClassify(bool afterOperand, int parenDepth) {
    switch (parser.current.specType) {
        case op_add:
            return afterOperand ? PREC_ADD : PREC_UPLUS;
        case op_sub:
            return afterOperand ? PREC_SUB : PREC_UMINUS;
        case op_not:        
            return afterOperand ? PREC_END : PREC_NOT;

        case op_mul:
            return afterOperand ? PREC_MUL : PREC_END;
        case op_div:
            return afterOperand ? PREC_DIV : PREC_END;
        case op_lowerThen:
            return afterOperand ? PREC_LT  : PREC_END;
        case op_lowerEqual:
            return afterOperand ? PREC_LE  : PREC_END;
        case op_biggerThen:
            return afterOperand ? PREC_GT  : PREC_END;
        case op_biggerEqual:
            return afterOperand ? PREC_GE  : PREC_END;
        case op_equal:
            return afterOperand ? PREC_EQ  : PREC_END;
        case op_notEqual:
            return afterOperand ? PREC_NE  : PREC_END;
        case op_and:
            return afterOperand ? PREC_AND : PREC_END;
        case op_or:
            return afterOperand ? PREC_OR  : PREC_END;

        case br_start:
            return afterOperand ? PREC_CALL : PREC_LBR;
        case br_end:
            return parenDepth > 0 ? PREC_RBR : PREC_END;
        case sn_dot:
            return afterOperand ? PREC_MEMBER : PREC_END;

        case id_idtName:
        case lit_int:
        case lit_double:
        case lit_string:
        case lit_true:
        case lit_false:
            return afterOperand ? PREC_END : PREC_ID;

        default:
            return PREC_END;
    }
}

astNode *exprMakeOperand(void) {
    token tok = parser.current;

    switch (tok.specType) {
        case id_idtName:
            return ast_newIdentifier(tok.value.word, tok.line, tok.col);
        case lit_int:
            return ast_newIntLit(tok.value.num, tok.line, tok.col);
        case lit_double:
            return ast_newDoubleLit(tok.value.decNum, tok.line, tok.col);
        case lit_string:
            return ast_newStringLit(tok.value.word, tok.line, tok.col);
        case lit_true:
            return ast_newBoolLit(true, tok.line, tok.col);
        case lit_false:
            return ast_newBoolLit(false, tok.line, tok.col);
        default:
            return NULL;
    }
}

void exprShift(int prec, bool withHandle) {
    if (withHandle) {
        exprSymbol handle;
        handle.kind = EXPR_HANDLE;

        if (top(&parser.exprStack)->kind == EXPR_NONTERM) {
            exprSymbol e = pop(&parser.exprStack);
            push(&parser.exprStack, handle);
            push(&parser.exprStack, e);
        } else {
            push(&parser.exprStack, handle);
        }
    }

    exprSymbol symbol;
    symbol.kind = EXPR_TERM;
    symbol.prec = prec;
    symbol.tok = parser.current;
    symbol.node = NULL;
    
    if (prec == PREC_ID) symbol.node = parserTrackUnattached(exprMakeOperand());

    push(&parser.exprStack, symbol);
    parserAdvance();
}

void exprReduce(void) {
    exprSymbol fromTop[3];
    size_t count = 0;

    while (top(&parser.exprStack)->kind != EXPR_HANDLE) {
        if (exprIsTerm(top(&parser.exprStack), PREC_END) || count == 3) error(SYNTAX_ERROR, "Invalid expression");
        fromTop[count++] = pop(&parser.exprStack);
    }
    pop(&parser.exprStack);

    exprSymbol *h[3];
    for (size_t i = 0; i < count; i++) 
        h[i] = &fromTop[count - 1 - i];

    astNode *result = NULL;

    if (count == 1 && exprIsTerm(h[0], PREC_ID)) {
        result = h[0]->node;
    } else if (count == 2 && exprIsTerm(h[0], PREC_LBR) && exprIsTerm(h[1], PREC_RBR)) {
        result = parserTrackUnattached(
            ast_newUnitLit(h[0]->tok.line, 
            h[0]->tok.col)
        );
    } else if (count == 3 && exprIsTerm(h[0], PREC_LBR) && h[1]->kind == EXPR_NONTERM && exprIsTerm(h[2], PREC_RBR)) {
        result = h[1]->node;
    } else if (count == 2 && h[0]->kind == EXPR_TERM && h[1]->kind == EXPR_NONTERM &&
               (h[0]->prec == PREC_UPLUS || h[0]->prec == PREC_UMINUS || h[0]->prec == PREC_NOT)) {
        result = parserTrackUnattached(
            ast_newUnary(h[0]->tok.specType, h[1]->node, h[0]->tok.line, h[0]->tok.col)
        );
        parserMarkAttached(h[1]->node);
    } else if (count == 3 && h[0]->kind == EXPR_NONTERM && h[1]->kind == EXPR_TERM && h[2]->kind == EXPR_NONTERM &&
               h[1]->prec >= PREC_ADD && h[1]->prec <= PREC_OR) {
        result = parserTrackUnattached(
            ast_newBinary(h[1]->tok.specType, h[0]->node, h[2]->node, h[1]->tok.line, h[1]->tok.col)
        );
        parserMarkAttached(h[0]->node);
        parserMarkAttached(h[2]->node);
    } else {
        error(SYNTAX_ERROR, "Invalid expression");
    }

    exprSymbol e;
    e.kind = EXPR_NONTERM;
    e.node = result;
    e.tok.line = result->line;
    e.tok.col = result->col;
    push(&parser.exprStack, e);
}

void exprParseArgs(astNodeList *args) {
    if (!parserCheck(br_end)) {
        do {
            if (parserCheck(br_end)) break;

            astNode *arg = parseExpr();
            if (arg == NULL) error(SYNTAX_ERROR, "Expected argument in function call");
            parserListAppend(args, arg);
        } while (parserMatch(sn_comma));
    }

    if (!parserMatch(br_end)) error(SYNTAX_ERROR, "Expected ')' after function call arguments");
}

void exprPostfix(int kind) {
    while (true) {
        int topPrec = exprTopTerminalPrec();
        if (topPrec != PREC_ID && topPrec != PREC_RBR) break;
        exprReduce();
    }

    exprSymbol operandSymbol = pop(&parser.exprStack);
    astNode *operand = operandSymbol.node;
    int line = parser.current.line;
    int col = parser.current.col;
    parserAdvance();

    if (kind == PREC_MEMBER) {
        if (!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected identifier after '.'");

        astNode *member = parserTrackUnattached(ast_newMember(operand, parser.current.value.word, line, col));
        parserMarkAttached(operand);
        operandSymbol.node = member;
        parserAdvance();
    } else {
        astNode *call = parserTrackUnattached(ast_newCall(operand, line, col));
        parserMarkAttached(operand);
        operandSymbol.node = call;

        exprParseArgs(&call->data.call.args);
    }

    push(&parser.exprStack, operandSymbol);
}

astNode *parseExpr(void) {
    PARSER_TRACE();

    exprSymbol end;
    end.kind = EXPR_TERM;
    end.prec = PREC_END;
    end.tok = parser.current;
    end.node = NULL;

    push(&parser.exprStack, end);
    int parenDepth = 0;

    while (true) {
        int incoming = exprClassify(exprAfterOperand(), parenDepth);

        if (incoming == PREC_CALL || incoming == PREC_MEMBER) {
            exprPostfix(incoming);
            continue;
        }

        int topPrec = exprTopTerminalPrec();
        if (topPrec == PREC_END && incoming == PREC_END) break;

        switch (precedenceTable[topPrec][incoming]) {
            case '<':
                if (incoming == PREC_LBR) parenDepth++;
                exprShift(incoming, true);
                break;
            case '=':
                if (incoming == PREC_RBR) parenDepth--;
                exprShift(incoming, false);
                break;
            case '>':
                exprReduce();
                break;
            default:
                if (topPrec == PREC_LBR && incoming == PREC_END) error(SYNTAX_ERROR, "Expected ')' in expression");
                error(SYNTAX_ERROR, "Invalid expression");
        }
    }

    astNode *result = NULL;
    if (top(&parser.exprStack)->kind == EXPR_NONTERM) result = pop(&parser.exprStack).node;
    pop(&parser.exprStack);

    return result;
}

bool isExprStart(specTokenType specType) {
    switch (specType) {
        case id_idtName:
        case lit_int:
        case lit_double:
        case lit_string:
        case lit_true:
        case lit_false:
        case br_start:
        case op_add:
        case op_sub:
        case op_not:
            return true;
        default:
            return false;
    }
}
