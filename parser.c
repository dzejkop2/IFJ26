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
        return parseFuncDef(false, false);
    } else if(parserCheck(kw_main)) {
        parserAdvance();
        skipNL();
        return parseFuncDef(true, false);
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
    printf("Parsing extension body\n");
    (void)extension;
}

astNode *parseFuncDef(bool isMain, bool isExtension) {
    PARSER_TRACE();
    if(!parserMatch(kw_def)) error(SYNTAX_ERROR, "Expected 'def'");
    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected function name after 'def'");
    
    astNode *funcDef = parserTrackUnattached(ast_newFuncDef(parser.current.value.word, isMain, parser.current.line, parser.current.col));
    parserAdvance();

    if(!isExtension && !parserCheck(br_start)) error(SYNTAX_ERROR, "Expected '(' after function name");

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
    return NULL;
}

astNode *parseBlock(void) {
    PARSER_TRACE();

    astNode *block = parserTrackUnattached(ast_newBlock(parser.current.line, parser.current.col));

    while(!parserCheck(ps_dedent) && !parserCheck(ps_eof)) {
        skipNL();
        parseStmt(&block->data.block.stmts);
    }

    return block;
}

void parseStmt(astNodeList *stmts) {
    PARSER_TRACE();

    astNode *stmt = NULL;

    if(parserCheck(kw_var)) {
        stmt = parseVarDecl(true);
    } else if(parserCheck(kw_val)) {
        stmt = parseVarDecl(false);
    } else if(parserCheck(kw_return)) {
        // Stmt -> kw_return RetVal
        stmt = parseReturn();
    } else if(parserCheck(kw_while)) {
        // Stmt -> kw_while Expr kw_do Body
        stmt = parseWhile();
    } else if(parserCheck(kw_if)) {
        // Stmt -> If
        stmt = parseIf();
    } else if(parserCheck(kw_for)) {
        // Stmt -> For
        stmt = parseFor();
    } else {
        // Stmt -> Expr AssignTail
        stmt = parseSimple();
    }

    if(stmt == NULL) error(SYNTAX_ERROR, "Invalid statement");

    parserListAppend(stmts, stmt);
}

astNode *parseSimple(void) {
    PARSER_TRACE();
    return NULL;
}

astNode *parseInline(void) {
    PARSER_TRACE();
    return NULL;
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
    return NULL;
}

astNode *parseWhile(void) {
    PARSER_TRACE();
    return NULL;
}

astNode *parseValue(void) {
    PARSER_TRACE();
    return NULL;
}

astNode *parseIf(void) {
    PARSER_TRACE();
    return NULL;
}

astNode *parseFor(void) {
    PARSER_TRACE();
    return NULL;
}

void parseForHeader(astNode *forNode) {
    PARSER_TRACE();
    (void)forNode;
}

void parseGenerator(astNode *forNode) {
    PARSER_TRACE();
    (void)forNode;
}

void parseGuards(astNodeList *guards) {
    PARSER_TRACE();
    (void)guards;
}

void parseGuardLines(astNodeList *guards) {
    PARSER_TRACE();
    (void)guards;
}

astNode *parseExpr(void) {
    PARSER_TRACE();
    return NULL;
}

bool isExprStart(specTokenType specType) {
    (void)specType;
    return false;
}

