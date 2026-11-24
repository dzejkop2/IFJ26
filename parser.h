/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file parser.h
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "scanner.h"
#include "ast.h"
#include "stack.h"

/**
 * @brief Parser struct that is created in the beginning of the syntax
 * 		  analysis cycle
 */
typedef struct {
	token current;
	astNode **unattached;
	size_t unattachedCount;
	size_t unattachedCapacity;
	Stack exprStack;
	bool active;
} parserState;

/**
 * @brief Precedence table indexes, worded for better usage
 */
typedef enum {
	PREC_ADD,
	PREC_SUB,
	PREC_MUL,
	PREC_DIV,
	PREC_LT,
	PREC_LE,
	PREC_GT,
	PREC_GE,
	PREC_EQ,
	PREC_NE,
	PREC_UPLUS,
	PREC_UMINUS,
	PREC_LBR,
	PREC_RBR,
	PREC_ID,
	PREC_NOT,
	PREC_AND,
	PREC_OR,
	PREC_COMMA,
	PREC_DOT,
	PREC_END,
	PREC_COUNT,
} precIndex;

/**
 * @brief Syntax analysis main function
 * @return Root node of the generated AST, if syntax analysis was successful
 */
astNode *parse(void);

/**
 * @brief Helper function, gets the next token from scanner
 */
void parserAdvance(void);
/**
 * @brief Helper function, checks for a specific token
 * @param specType Expected token
 * @return If specific token was found
 */
bool parserCheck(specTokenType specType);
/**
 * @brief Helper function, checks for a specific token, advances parser
 * @param specType Expected token
 * @return If specific token was found
 */
bool parserMatch(specTokenType specType);

/**
 * @brief Parse the Program grammar rule
 * @return Pointer to the root AST node
 *
 * First node that is created, parses the import keyword
 */
astNode *parseProgram(void);
/**
 * @brief Parse the GlobalDefs grammar rule
 * @param defs Pointer to a astNodeList where definitions will be added to
 */
void parseGlobalDefs(astNodeList *defs);
/**
 * @brief Parse the GlobalDef grammar rule
 * @return Pointer to a astNode of a function or extension definition
 */
astNode *parseGlobalDef(void);
/**
 * @brief Parse the Extention grammar rules
 * @return Pointer to a astNode of extension
 *
 * Extension is a special case of a function
 * EXTKW
 */
astNode *parseExtension(void);
/**
 * @brief Parse the Extention body grammar rules
 * @param extension Pointer to the extension whose body is being parsed
 */
void parseExtBody(astNode *extension);
/**
 * @brief Parse the Function grammar rules
 * @param isMain Check if the function is main
 * @return Pointer to a astNode of function
 */
astNode *parseFuncDef(bool isMain);
/**
 * @brief Parse the Parameter lists grammar rules
 * @param paramLists Pointer to a astNodeList that contains multiple parameter
 * lists
 *
 * PARAMLISTS, function can have multiple parameter lists -> foo()()(), is valid
 */
void parseParamLists(astNodeList *paramLists);
/**
 * @brief Parse the Parameter list grammar rules
 * @param params Pointer to a astNodeList that contains parameters of a function
 */
void parseParams(astNodeList *params);
/**
 * @brief Parse the Parameter grammar rules
 * @return Pointer to a astNode representing the parameter
 */
astNode *parseParam(void);
/**
 * @brief Parse the Type Option grammar rules
 * @return Returns type NONE if no type is specified, else calls parseType and
 * returns that type
 */
astDataType parseTypeOpt(void);
/**
 * @brief Parse the Type grammar rules
 * @return Returns type parsed
 */
astDataType parseType(void);

/**
 * @brief Parse the Body grammar rules
 * @return Returns a pointer to a astNode representing the body
 */
astNode *parseBody(void);
/**
 * @brief Parse the Block grammar rules
 * @return Returns a pointer to a astNode representing the code block
 */
astNode *parseBlock(void);
/**
 * @brief Parse the Statement grammar rules
 * @return Returns a pointer to a astNode representing the statement
 */
astNode *parseStmt(void);
/**
 * @brief Parse the Simple grammar rules
 * @return Returns a pointer to a astNode representing the simple statement
 *
 * Simple statements are expressions
 */
astNode *parseSimple(void);
/**
 * @brief Parse the Inline grammar rules
 * @return Returns a pointer to a astNode representing the inline statement
 *
 * Inline statements are returns and assignments that are inline, used for
 * inline functions and conditions
 */
astNode *parseInline(void);
/**
 * @brief Parse the Variable declaration grammar rules
 * @param isMutable Checks if the declared variable can be changed or not (val /
 * var)
 * @return Returns a pointer to a astNode representing the var declaration
 */
astNode *parseVarDecl(bool isMutable);
/**
 * @brief Parse the Return grammar rules
 * @return Returns a pointer to a astNode representing the return statement
 */
astNode *parseReturn(void);
/**
 * @brief Parse the While grammar rules
 * @return Returns a pointer to a astNode representing the while loop
 */
astNode *parseWhile(void);
/**
 * @brief Parse the Value grammar rules
 * @return Returns a pointer to a astNode representing the value
 *
 * Value is a expression or a if statement CONTROL ext
 */
astNode *parseValue(void);
/**
 * @brief Parse the If head grammar rules
 * @return Returns a pointer to a astNode representing the If statements head
 *
 * If statement head is the "if (expr) then" part of the condition
 */
astNode *parseIfHead(void);
/**
 * @brief Parse the If grammar rules
 * @param isValue Check if the If statement is used as a value in code
 * @return Returns a pointer to a astNode representing the If statement
 *
 * If is the body of the statement and the else (if exists)
 */
astNode *parseIf(bool isValue);
/**
 * @brief Parse the For grammar rules
 * @return Returns a pointer to a astNode representing the For statement
 */
astNode *parseFor(void);
/**
 * @brief Helper function that parses the header of the For cycle
 * @param forNode Pointer to the For astNode that the header is stored into
 *
 * Header is either inline or on multiple lines.
 */
void parseForHeader(astNode *forNode);
/**
 * @brief Helper function that parses the generator of the For cycle
 * @param forNode Pointer to the For astNode that the generator is stored into
 *
 * Generator is the "i <- from to/until to (by step)" part of the For cycle
 */
void parseGenerator(astNode *forNode);
/**
 * @brief Helper function that parses the guards of the For cycle
 * @param guards Pointer to a astNodeList where guard conditions will be added
 * to
 *
 * Guards are the inline "if (expr)" conditions after the generator
 */
void parseGuards(astNodeList *guards);
/**
 * @brief Helper function that parses the guards of a multi line For header
 * @param guards Pointer to a astNodeList where guard conditions will be added
 * to
 */
void parseGuardLines(astNodeList *guards);
/**
 * @brief Parse the Interpolated string grammar rules
 * @return Returns a pointer to a astNode representing the interpolated string
 *
 * Parts of the string can be string literals, identifiers or
 * expressions inside of "${ }"
 */
astNode *parseInterp(void);
/**
 * @brief Parse the If expression grammar rules
 * @return Returns a pointer to a astNode representing the If expression
 *
 * If expression is used as a value, so both branches are values and
 * the else branch is required
 */
astNode *parseIfExpr(void);
/**
 * @brief Helper function that parses either an expression or an If expression
 * @return Returns a pointer to a astNode representing the expression or If
 * expression
 */
astNode *parseExprOrIf(void);

/**
 * @brief Parse an expression using precedence analysis
 * @return Returns a pointer to a astNode representing the expression,
 * 		   NULL if no expression was found
 */
astNode *parseExpr(void);

#endif
