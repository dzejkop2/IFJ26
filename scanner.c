/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.c
 *
 * AI usage:
 * Consultations and brainstorming about lexer design (indentation handling,
 * interpolated strings, mode stack), debugging failing tests, writing tests,
 * reviewing and improving comments. "Check if my lexer finite state machine
 * matches the specification" "Generate a large set of tests for the scanner
 * covering all token types, edge cases and lexical errors (exit code 1)" "Write
 * me a comment for this function"
 */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "scanner.h"
#include "error.h"
#include "dynamicString.h"

#define REALLOC_MEM 4096
#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))
#define MODE_STACK_MAX 64

stdInput input;
dynamicString dynStr;

int bufferPeeker(int offset);
int bufferAdvance(void);
void scannerFree(void);
void scannerInit(void);
void scanner_dtor(void);
void indentPush(int spaces);
void indentResolve(int spaces);
void storeSign(void);
void modeInit(void);
static token peeked;
static int hasPeeked = 0;

int *indentStack;
size_t indentTop;
size_t indentCapacity;
int pendingIndent;
int pendingDedent;
static int parenDepth = 0;
// int hasPeeked = 0;
int lastWasNL = 1;
int scannerActive = 0;

token makeNumber(token helpToken, specTokenType numType);
token makeString(token helpToken);

// keywords
const keyword keywords[] = {{"import", abs_keyword, kw_import},
							{"return", abs_keyword, kw_return},
							{"do", abs_keyword, kw_do},
							{"while", abs_keyword, kw_while},
							{"else", abs_keyword, kw_else},
							{"if", abs_keyword, kw_if},
							{"then", abs_keyword, kw_then},
							{"var", abs_keyword, kw_var},
							{"val", abs_keyword, kw_val},
							{"def", abs_keyword, kw_def},
							{"@main", abs_keyword, kw_main},
							{"for", abs_keyword, kw_for},
							{"extension", abs_keyword, kw_extension},
							{"true", abs_literal, lit_true},
							{"false", abs_literal, lit_false},
							{"Int", abs_dataType, dt_int},
							{"Double", abs_dataType, dt_double},
							{"String", abs_dataType, dt_string},
							{"Unit", abs_dataType, dt_unit},
							{"Boolean", abs_dataType, dt_bool},
							{"case", abs_reserved, res_case},
							{"class", abs_reserved, res_class},
							{"List", abs_reserved, res_list},
							{"match", abs_reserved, res_match},
							{"type", abs_reserved, res_type},
							{NULL, 0, 0}};

// mode stack handler
static mode modeStack[MODE_STACK_MAX];
static int modeTop = 0;

/**
 * @brief Stack algorithm modePush - pushes modeHelper on top of modeStack
 *
 * @param modeHelper mode that is pushed on top of the stack
 */
void modePush(mode modeHelper) {
	if (modeTop >= MODE_STACK_MAX) {
		error(INTERNAL_ERROR, "Interpolation nesting too deep");
	}
	modeStack[modeTop++] = modeHelper;
}

/**
 * @brief Stack algorithm modePop - removes the top value of modeStack by
 * decreasing modeTop
 *
 * The bottom MODE_NORMAL is never removed.
 */
void modeInit(void) {
	modeTop = 0;
	modePush(MODE_NORMAL);
}

/**
 * @brief Stack algorithm modePop - makes a pop of the top value by trucking
 * down modeTop value and decresing it
 */
void modePop(void) {
	if (modeTop > 1) {
		modeTop--;
	}
}

/**
 * @brief Stack algorithm modeCurr - returns top value
 *
 * @return current top value
 */
mode modeCurr(void) { return modeStack[modeTop - 1]; }

int idType(int typeHelper) {
	if ((typeHelper >= 'a' && typeHelper <= 'z') ||
		(typeHelper >= 'A' && typeHelper <= 'Z')) {
		return 1;
	} else if (typeHelper >= '0' && typeHelper <= '9') {
		return 2;
	} else if (typeHelper == '_') {
		return 3;
	} else {
		return 0;
	}
}

/**
 * @brief Helper function that reallocates memory of input, initialise other
 * input parts to starting values
 *
 * @param sourceFile poiter to source file
 */
void loadInput(FILE *sourceFile) {
	size_t reallocHelperCapacity = REALLOC_MEM;
	input.loadedInput = malloc(reallocHelperCapacity);
	if (input.loadedInput == NULL) {
		error(INTERNAL_ERROR, "Scanners help buffer malloc memory failed");
	}
	input.lenght = 0;
	input.pos = 0;
	input.line = 1;
	input.col = 1;
	int count = 0;
	while ((count = fread(input.loadedInput + input.lenght, sizeof(char),
						  reallocHelperCapacity - input.lenght, sourceFile)) >
		   0) {
		input.lenght = input.lenght + count;
		if (input.lenght == reallocHelperCapacity) {
			reallocHelperCapacity = reallocHelperCapacity * 2;
			char *reallocHelper =
				realloc(input.loadedInput, reallocHelperCapacity);
			if (reallocHelper == NULL) {
				error(INTERNAL_ERROR,
					  "Scanners help buffer realloc memory failed");
			}
			input.loadedInput = reallocHelper;
		}
	}
}

/**
 * @brief Function that takes and argument and peek ahead of curr position to a
 * loadedInput
 *
 * @param offset value that is used to peek ahead of curr position
 * @return value of integer that is changed to unsigned char, if there is an EOF
 * than -1
 */
int bufferPeeker(int offset) {
	if ((input.pos + offset) >= input.lenght) {
		return EOF;
	} else {
		return (unsigned char)input.loadedInput[input.pos + offset];
	}
}

/**
 * @brief Helper function that calls bufferPeeker() and handles returning
 * values, moves input position and updates line and col
 *
 * @return if there is an EOF than -1, else integer representation of returning
 * char
 */
int bufferAdvance(void) {
	int peekHelper = bufferPeeker(0);
	if (peekHelper == EOF) {
		return EOF;
	}
	input.pos++;
	if (peekHelper == '\n') {
		input.line++;
		input.col = 1;
	} else {
		input.col++;
	}
	return peekHelper;
}

/**
 * @brief Function that initialise values and calling init parts of heleper
 * functions that are neccessery for running a scanner itself
 */
void scannerInit(void) {
	static int atExitTriggered = 0;
	if (!atExitTriggered) {
		atexit(scannerFree);
		atExitTriggered = 1;
	}
	loadInput(stdin);
	dynStr_ctor(&dynStr);
	indentStack = NULL;
	indentTop = 0;
	indentCapacity = 0;
	pendingIndent = 0;
	pendingDedent = 0;
	scannerActive = 1;
	indentPush(0);
	modeInit();
}

/**
 * @brief Main logic function that is implemntation of lexer finite state
 * machine
 *
 * @return the next token with its position, abs_type, specType or keyword type
 */
token getTokenChecked(void) {
	states state = STATE_START;
	states returnState = STATE_START;
	token helpToken;
	helpToken.value.word = NULL;
	dynStr_freeBuff(&dynStr);
	int commentDepth = 0;
	int newlinePending = 0;
	int hasNL = 0;
	int spaces = 0;
	int tabAtStart = 0;
	int indentDone = 0;
	int intpLen = 0;

	if (pendingDedent > 0 || pendingIndent > 0) {
		helpToken.absType = abs_position;
		if (pendingDedent > 0) {
			pendingDedent--;
			helpToken.specType = ps_dedent;
		} else {
			pendingIndent--;
			helpToken.specType = ps_indent;
		}
		helpToken.line = input.line;
		helpToken.col = input.col;

		return makeToken(helpToken);
	}

	switch (modeCurr()) {
	case MODE_INTERP:
	case MODE_INTERP_MULTI:
		helpToken.line = input.line;
		helpToken.col = input.col;
		state = STATE_INTERP_BODY;
		break;
	case MODE_NORMAL:
	case MODE_EXPR:
		break;
	}

	while (1) {
		int currentSign = bufferPeeker(0);

		switch (state) {
		case STATE_START:
			helpToken.line = input.line;
			helpToken.col = input.col;

			if (currentSign == ' ' || currentSign == '\t' ||
				currentSign == '\r') {
				bufferAdvance();
			} else if (currentSign == EOF) {
				if (modeCurr() != MODE_NORMAL) {
					error(LEXICAL_ERROR, "Unterminated interpolated string");
				}
				helpToken.absType = abs_position;
				if (!lastWasNL) {
					helpToken.specType = ps_newLine;
					return makeToken(helpToken);
				}
				if (indentTop > 1) {
					indentTop--;
					helpToken.specType = ps_dedent;
					return makeToken(helpToken);
				}
				helpToken.specType = ps_eof;
				helpToken.line = input.line;
				helpToken.col = input.col;
				state = STATE_NEWLINE;
				return makeToken(helpToken);
			} else if ((currentSign >= 'A' && currentSign <= 'Z') ||
					   (currentSign >= 'a' && currentSign <= 'z') ||
					   currentSign == '@') {
				storeSign();
				state = STATE_IDENTIFIER;
			} else if (currentSign == '_') {
				storeSign();
				state = STATE_UNDERSCORE;

				// abs_sign
			} else if (currentSign == '\n') {
				bufferAdvance();
				newlinePending = 1;
				spaces = 0;
				tabAtStart = 0;
				indentDone = 0;
				state = STATE_NEWLINE;
			} else if (currentSign == '"') {
				bufferAdvance();
				state = STATE_QUOTES_START;
			} else if (currentSign == '(') {
				bufferAdvance();
				parenDepth++;
				helpToken.absType = abs_bracet;
				helpToken.specType = br_start;
				return makeToken(helpToken);
			} else if (currentSign == ')') {
				if (parenDepth > 0) {
					parenDepth--;
				}
				bufferAdvance();
				helpToken.absType = abs_bracet;
				helpToken.specType = br_end;
				return makeToken(helpToken);
			} else if (currentSign == '}') {
				if (modeCurr() != MODE_EXPR) {
					error(LEXICAL_ERROR, "Unexpected '}'");
				}
				bufferAdvance();
				modePop();
				helpToken.absType = abs_interp;
				helpToken.specType = intp_exprEnd;
				return makeToken(helpToken);
			} else if (currentSign == ',') {
				bufferAdvance();
				helpToken.absType = abs_sign;
				helpToken.specType = sn_comma;
				return makeToken(helpToken);
			} else if (currentSign == ':') {
				bufferAdvance();
				helpToken.absType = abs_sign;
				helpToken.specType = sn_colon;
				return makeToken(helpToken);
			} else if (currentSign == ';') {
				bufferAdvance();
				helpToken.absType = abs_sign;
				helpToken.specType = sn_semiColon;
				return makeToken(helpToken);
			} else if (currentSign == '+') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_add;
				return makeToken(helpToken);
			} else if (currentSign == '-') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_sub;
				return makeToken(helpToken);
			} else if (currentSign == '*') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_mul;
				return makeToken(helpToken);
			} else if (currentSign == '/') {
				bufferAdvance();
				state = STATE_DIV;
			} else if (currentSign == '=') {
				bufferAdvance();
				state = STATE_ASSIGN;
			} else if (currentSign == '>') {
				bufferAdvance();
				state = STATE_BIGGER_THEN;
			} else if (currentSign == '<') {
				bufferAdvance();
				state = STATE_LOWER_THEN;
			} else if (currentSign == '!') {
				bufferAdvance();
				state = STATE_NOT;
			} else if (currentSign == '&') {
				bufferAdvance();
				state = STATE_AMP;
			} else if (currentSign == '|') {
				bufferAdvance();
				state = STATE_PIPE;
			} else if (currentSign == '.') {
				storeSign();
				state = STATE_DOUBLE_DOT;
			} else if (currentSign >= '1' && currentSign <= '9') {
				storeSign();
				state = STATE_INT;
			} else if (currentSign == '0') {
				storeSign();
				state = STATE_ZERO;
			} else {
				error(LEXICAL_ERROR, "Unexpected character");
			}
			break;

		// abs_identifier
		case STATE_IDENTIFIER:
			if (((int)currentSign >= '0' && (int)currentSign <= '9') ||
				(currentSign >= 'A' && currentSign <= 'Z') ||
				(currentSign >= 'a' && currentSign <= 'z') ||
				currentSign == '_') {
				storeSign();
			} else {
				char *word = dynStr_output(&dynStr);

				// differentiating betwween s" and s""" notation
				if (strcmp(word, "s") == 0 && currentSign == '"') {
					free(word);
					bufferAdvance();
					if (bufferPeeker(0) == '"' && bufferPeeker(1) == '"') {
						bufferAdvance();
						bufferAdvance();
						modePush(MODE_INTERP_MULTI);
					} else {
						modePush(MODE_INTERP);
					}
					helpToken.absType = abs_interp;
					helpToken.specType = intp_begin;
					return makeToken(helpToken);
				}

				const keyword *foundKeyword = findKeyword(word);
				if (foundKeyword != NULL) {
					free(word);
					helpToken.absType = foundKeyword->absType;
					helpToken.specType = foundKeyword->specType;
				} else if (word[0] == '@') {
					free(word);
					error(LEXICAL_ERROR, "Only '@main' can start with '@'");
				} else {
					helpToken.absType = abs_identifier;
					helpToken.specType = id_idtName;
					helpToken.value.word = word;
				}

				return makeToken(helpToken);
			}
			break;

		case STATE_UNDERSCORE:
			if (((int)currentSign >= '0' && (int)currentSign <= '9') ||
				(currentSign >= 'A' && currentSign <= 'Z') ||
				(currentSign >= 'a' && currentSign <= 'z') ||
				currentSign == '_') {
				storeSign();
				state = STATE_IDENTIFIER;
			} else {
				helpToken.absType = abs_sign;
				helpToken.specType = sn_underscore;
				return makeToken(helpToken);
			}
			break;

		// abs_op
		case STATE_DIV:
			if (currentSign == '/') {
				bufferAdvance();
				returnState = STATE_START;
				state = STATE_LINECOMMENT;
			} else if (currentSign == '*') {
				bufferAdvance();
				commentDepth = 1;
				returnState = STATE_START;
				state = STATE_BLOCKCOMMENT_MID;
			} else {
				helpToken.absType = abs_op;
				helpToken.specType = op_div;
				return makeToken(helpToken);
			}
			break;

		case STATE_ASSIGN:
			if (currentSign == '=') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_equal;
				return makeToken(helpToken);
			}
			helpToken.absType = abs_op;
			helpToken.specType = op_assign;
			return makeToken(helpToken);

		case STATE_BIGGER_THEN:
			if (currentSign == '=') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_biggerEqual;
				return makeToken(helpToken);
			}
			helpToken.absType = abs_op;
			helpToken.specType = op_biggerThen;
			return makeToken(helpToken);

		case STATE_LOWER_THEN:
			if (currentSign == '=') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_lowerEqual;
				return makeToken(helpToken);
			}
			if (currentSign == '-') {
				bufferAdvance();
				helpToken.absType = abs_control;
				helpToken.specType = ct_arrowLeft;
				return makeToken(helpToken);
			}
			helpToken.absType = abs_op;
			helpToken.specType = op_lowerThen;
			return makeToken(helpToken);

		case STATE_NOT:
			if (currentSign == '=') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_notEqual;
				return makeToken(helpToken);
			}
			helpToken.absType = abs_op;
			helpToken.specType = op_not;
			return makeToken(helpToken);

		case STATE_AMP:
			if (currentSign == '&') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_and;
				return makeToken(helpToken);
			}
			error(LEXICAL_ERROR, "Expected '&&'");
			break;

		case STATE_PIPE:
			if (currentSign == '|') {
				bufferAdvance();
				helpToken.absType = abs_op;
				helpToken.specType = op_or;
				return makeToken(helpToken);
			}
			error(LEXICAL_ERROR, "Expected '||'");
			break;

		// abs_comments
		case STATE_LINECOMMENT:
			if (currentSign == EOF) {
				if (newlinePending) {
					spaces = 0;
					tabAtStart = 0;
					indentDone = 0;
					state = STATE_NEWLINE;
				} else {
					state = STATE_START;
				}
			} else if (currentSign == '\n') {
				newlinePending = 1;
				bufferAdvance();
				spaces = 0;
				tabAtStart = 0;
				indentDone = 0;
				state = STATE_NEWLINE;
			} else {
				bufferAdvance();
			}
			break;

		case STATE_BLOCKCOMMENT_MID:
			if (currentSign == EOF) {
				error(LEXICAL_ERROR, "Unterminated block comment");
			} else if (currentSign == '*') {
				bufferAdvance();
				state = STATE_BLOCKCOMMENT_END;
			} else if (currentSign == '/') {
				bufferAdvance();
				state = STATE_BLOCKCOMMENT_SLASH;
			} else {
				if (currentSign == '\n') {
					hasNL++;
					state = STATE_BLOCKCOMMENT_MID;
				}
				bufferAdvance();
			}
			break;

		case STATE_BLOCKCOMMENT_END:
			if (currentSign == EOF) {
				error(LEXICAL_ERROR, "Unterminated block comment");
			} else if (currentSign == '*') {
				bufferAdvance();
			} else if (currentSign == '/') {
				bufferAdvance();
				commentDepth--;
				if (commentDepth > 0) {
					state = STATE_BLOCKCOMMENT_MID;
				} else if (hasNL) {
					state = STATE_BLOCKCOMMENT_AFTER_NL;
				} else {
					state = returnState;
				}
			} else {
				if (currentSign == '\n') {
					hasNL = 1;
				}
				bufferAdvance();
				state = STATE_BLOCKCOMMENT_MID;
			}
			break;

		case STATE_BLOCKCOMMENT_SLASH:
			if (currentSign == EOF) {
				error(LEXICAL_ERROR, "Unterminated block comment");
			} else if (currentSign == '\n') {
				bufferAdvance();
				hasNL++;
				state = STATE_BLOCKCOMMENT_MID;
			} else if (currentSign == '/') {
				bufferAdvance();
				state = STATE_BLOCKCOMMENT_SLASH;
			} else if (currentSign == '*') {
				bufferAdvance();
				commentDepth++;
				state = STATE_BLOCKCOMMENT_MID;
			} else {
				bufferAdvance();
				state = STATE_BLOCKCOMMENT_MID;
			}
			break;

		case STATE_BLOCKCOMMENT_AFTER_NL:
			if (currentSign == ' ' || currentSign == '\t' ||
				currentSign == '\r') {
				bufferAdvance();
			} else if (currentSign == '\n') {
				bufferAdvance();
				spaces = 0;
				tabAtStart = 0;
				indentDone = 0;
				state = STATE_NEWLINE;
			} else if (currentSign == EOF) {
				spaces = 0;
				tabAtStart = 0;
				indentDone = 0;
				state = STATE_NEWLINE;
			} else if (currentSign == '/' && bufferPeeker(1) == '/') {
				bufferAdvance();
				bufferAdvance();
				state = STATE_LINECOMMENT;
			} else if (currentSign == '/' && bufferPeeker(1) == '*') {
				bufferAdvance();
				bufferAdvance();
				commentDepth = 1;
				state = STATE_BLOCKCOMMENT_MID;
			} else {
				error(LEXICAL_ERROR, "Token after currMulti-line block comment "
									 "on the same line");
			}
			break;

		// abs_position
		case STATE_NEWLINE:
			if (currentSign == ' ') {
				bufferAdvance();
				if (!indentDone) {
					spaces++;
				}
			} else if (currentSign == '\n') {
				tabAtStart = 0;
				indentDone = 0;
				bufferAdvance();
				spaces = 0;
			} else if (currentSign == '\r') {
				bufferAdvance();
			} else if (currentSign == '\t') {
				if (!indentDone) {
					tabAtStart++;
				}
				bufferAdvance();
			} else if (currentSign == '/' && bufferPeeker(1) == '/') {
				state = STATE_LINECOMMENT;
			} else if (currentSign == '/' && bufferPeeker(1) == '*') {
				bufferAdvance();
				bufferAdvance();
				commentDepth = 1;
				hasNL = 0;
				indentDone = 1;
				returnState = STATE_NEWLINE;
				state = STATE_BLOCKCOMMENT_MID;
			} else {
				if (modeCurr() != MODE_NORMAL) {
					error(LEXICAL_ERROR,
						  "Newline inside interpolated expression");
				}
				if (tabAtStart > 0 && currentSign != EOF) {
					error(LEXICAL_ERROR, "Tab in indentation");
				}
				if (parenDepth > 0 && currentSign != EOF) {
					if (spaces <= indentStack[indentTop - 1]) {
						error(LEXICAL_ERROR,
							  "Continuation line must be indented more");
					}
					state = STATE_START;
					break;
				}
				if (currentSign != EOF) {
					indentResolve(spaces);
				}
				if (lastWasNL && pendingIndent == 0 && pendingDedent == 0) {
					state = STATE_START;
					break;
				}
				helpToken.absType = abs_position;
				helpToken.specType = ps_newLine;
				return makeToken(helpToken);
			}
			break;

		// num literals
		case STATE_DOUBLE_DOT:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE;
			} else {
				helpToken.absType = abs_sign;
				helpToken.specType = sn_dot;
				return makeToken(helpToken);
			}
			break;

		case STATE_ZERO:
			if (currentSign == '.' && (bufferPeeker(1) >= '0') &&
				(bufferPeeker(1) <= '9')) {
				storeSign();
				state = STATE_INT_DOT;
			} else if (currentSign == 'e' || currentSign == 'E') {
				storeSign();
				state = STATE_NUM_EXP;
			} else if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_ZERO_DIGITS;
			} else if (currentSign == 'd' || currentSign == 'D') {
				storeSign();
				state = STATE_DOUBLE_SUFFIX;
			} else {
				return makeNumber(helpToken, lit_int);
			}
			break;

		case STATE_ZERO_DIGITS:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_ZERO_DIGITS;
			} else if (currentSign == '.' && (bufferPeeker(1) >= '0') &&
					   (bufferPeeker(1) <= '9')) {
				storeSign();
				state = STATE_INT_DOT;
			} else if (currentSign == 'e' || currentSign == 'E') {
				storeSign();
				state = STATE_NUM_EXP;
			} else {
				error(LEXICAL_ERROR, "Non-zero integer cannot start with 0");
			}
			break;

		case STATE_INT:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
			} else if (currentSign == '_') {
				bufferAdvance();
				state = STATE_INT_UNDERSCORE;
			} else if (currentSign == '.' && (bufferPeeker(1) >= '0') &&
					   (bufferPeeker(1) <= '9')) {
				storeSign();
				state = STATE_INT_DOT;
			} else if (currentSign == 'e' || currentSign == 'E') {
				storeSign();
				state = STATE_NUM_EXP;
			} else if (currentSign == 'd' || currentSign == 'D') {
				storeSign();
				state = STATE_DOUBLE_SUFFIX;
			} else {
				return makeNumber(helpToken, lit_int);
			}
			break;

		case STATE_INT_UNDERSCORE:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_INT;
			} else {
				error(LEXICAL_ERROR, "Expected digit after '_' in number");
			}
			break;

		case STATE_INT_DOT:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE;
			} else {
				error(LEXICAL_ERROR, "Expected digit after '.' in number");
			}
			break;

		case STATE_DOUBLE:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
			} else if (currentSign == '_') {
				bufferAdvance();
				state = STATE_DOUBLE_UNDERSCORE;
			} else if (currentSign == 'e' || currentSign == 'E') {
				storeSign();
				state = STATE_NUM_EXP;
			} else if (currentSign == 'd' || currentSign == 'D') {
				bufferAdvance();
				state = STATE_DOUBLE_SUFFIX;
			} else {
				return makeNumber(helpToken, lit_double);
			}
			break;

		case STATE_DOUBLE_UNDERSCORE:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE;
			} else {
				error(LEXICAL_ERROR, "Expected digit after '_' in number");
			}
			break;

		case STATE_NUM_EXP:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE_EXP;
			} else if (currentSign == '+' || currentSign == '-') {
				storeSign();
				state = STATE_SIGN_NUM_EXP;
			} else {
				error(LEXICAL_ERROR, "Expected digit or sign in exponent");
			}
			break;

		case STATE_SIGN_NUM_EXP:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE_EXP;
			} else {
				error(LEXICAL_ERROR, "Expected digit in exponent");
			}
			break;

		case STATE_DOUBLE_EXP:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
			} else if (currentSign == '_') {
				bufferAdvance();
				state = STATE_EXP_UNDERSCORE;
			} else if (currentSign == 'd' || currentSign == 'D') {
				bufferAdvance();
				state = STATE_DOUBLE_SUFFIX;
			} else {
				return makeNumber(helpToken, lit_double);
			}
			break;

		case STATE_EXP_UNDERSCORE:
			if (currentSign >= '0' && currentSign <= '9') {
				storeSign();
				state = STATE_DOUBLE_EXP;
			} else {
				error(LEXICAL_ERROR, "Expected digit after '_' in number");
			}
			break;

		case STATE_DOUBLE_SUFFIX:
			return makeNumber(helpToken, lit_double);

		// strings
		case STATE_QUOTES_START:
			if (currentSign == '"') {
				bufferAdvance();
				state = STATE_STRING_EMPTY;
			} else {
				state = STATE_QUOTES_BODY;
			}
			break;

		case STATE_QUOTES_BODY:
			if (currentSign == EOF || currentSign == '\n') {
				error(LEXICAL_ERROR, "Unterminated string");
			} else if (currentSign == '"') {
				bufferAdvance();
				state = STATE_STRING_END;
			} else if (currentSign == '\\') {
				bufferAdvance();
				state = STATE_QUOTES_ESCAPE;
			} else {
				storeSign();
			}
			break;

		case STATE_QUOTES_ESCAPE:
			if (currentSign == 'n') {
				dynStr_load(&dynStr, '\n');
			} else if (currentSign == 'r') {
				dynStr_load(&dynStr, '\r');
			} else if (currentSign == 't') {
				dynStr_load(&dynStr, '\t');
			} else if (currentSign == '"' || currentSign == '\\') {
				dynStr_load(&dynStr, (char)currentSign);
			} else {
				error(LEXICAL_ERROR, "Unknown escape sequence in string");
			}
			bufferAdvance();
			state = STATE_QUOTES_BODY;
			break;

		case STATE_STRING_END:
			return makeString(helpToken);

		case STATE_STRING_EMPTY:
			if (currentSign == '"') {
				bufferAdvance();
				state = STATE_MUL_STRING_START;
			} else {
				return makeString(helpToken);
			}
			break;

		case STATE_MUL_STRING_START:
			if (currentSign == EOF) {
				error(LEXICAL_ERROR, "Unterminated multiline string");
			} else if (currentSign == '"') {
				bufferAdvance();
				state = STATE_MUL_STRING_1;
			} else {
				storeSign();
			}
			break;

		case STATE_MUL_STRING_1:
			if (currentSign == '"') {
				bufferAdvance();
				state = STATE_MUL_STRING_2;
			} else {
				dynStr_load(&dynStr, '"');
				state = STATE_MUL_STRING_START;
			}
			break;

		case STATE_MUL_STRING_2:
			if (currentSign == '"') {
				bufferAdvance();
				state = STATE_MUL_STRING_END;
			} else {
				dynStr_load(&dynStr, '"');
				dynStr_load(&dynStr, '"');
				state = STATE_MUL_STRING_START;
			}
			break;

		case STATE_MUL_STRING_END:
			return makeString(helpToken);

		// interpolated strings
		case STATE_INTERP_BODY: {
			int currMulti = (modeCurr() == MODE_INTERP_MULTI);
			int multiEnd;
			if (currentSign == EOF || (!currMulti && currentSign == '\n')) {
				error(LEXICAL_ERROR, "Unterminated interpolated string");
			} else if (currentSign == '"') {
				if (!currMulti ||
					(bufferPeeker(1) == '"' && bufferPeeker(2) == '"')) {
					multiEnd = 1;
				} else {
					multiEnd = 0;
				}
				if (!multiEnd) {
					storeSign();
					intpLen++;
				} else if (intpLen > 0) {
					helpToken.absType = abs_interp;
					helpToken.specType = intp_part;
					helpToken.value.word = dynStr_output(&dynStr);
					return makeToken(helpToken);
				} else {
					bufferAdvance();
					if (currMulti) {
						bufferAdvance();
						bufferAdvance();
					}
					modePop();
					helpToken.absType = abs_interp;
					helpToken.specType = intp_end;
					return makeToken(helpToken);
				}
			} else if (currentSign == '$') {
				if (bufferPeeker(1) == '$') {
					bufferAdvance();
					bufferAdvance();
					dynStr_load(&dynStr, '$');
					intpLen++;
				} else if (intpLen > 0) {
					helpToken.absType = abs_interp;
					helpToken.specType = intp_part;
					helpToken.value.word = dynStr_output(&dynStr);
					return makeToken(helpToken);
				} else {
					bufferAdvance();
					helpToken.line = input.line;
					helpToken.col = input.col;
					state = STATE_INTERP_DOLLAR;
				}
			} else if (currentSign == '\\' && !currMulti) {
				bufferAdvance();
				state = STATE_INTERP_ESCAPE;
			} else {
				storeSign();
				intpLen++;
			}
			break;
		}

		case STATE_INTERP_ESCAPE:
			if (currentSign == 'n') {
				dynStr_load(&dynStr, '\n');
			} else if (currentSign == 'r') {
				dynStr_load(&dynStr, '\r');
			} else if (currentSign == 't') {
				dynStr_load(&dynStr, '\t');
			} else if (currentSign == '"' || currentSign == '\\') {
				dynStr_load(&dynStr, (char)currentSign);
			} else {
				error(LEXICAL_ERROR, "Unknown escape sequence in string");
			}
			bufferAdvance();
			intpLen++;
			state = STATE_INTERP_BODY;
			break;

		case STATE_INTERP_DOLLAR:
			if (currentSign == '{') {
				bufferAdvance();
				modePush(MODE_EXPR);
				helpToken.absType = abs_interp;
				helpToken.specType = intp_exprBegin;
				return makeToken(helpToken);
			} else if (idType(currentSign) == 1 || idType(currentSign) == 3) {
				storeSign();
				state = STATE_INTERP_ID;
			} else {
				error(LEXICAL_ERROR,
					  "Expected '$', '{' or identifier after '$'");
			}
			break;

		case STATE_INTERP_ID:
			if (idType(currentSign) != 0) {
				storeSign();
			} else {
				char *word = dynStr_output(&dynStr);
				if (strcmp(word, "_") == 0 || findKeyword(word) != NULL) {
					free(word);
					error(LEXICAL_ERROR, "Invalid identifier after '$'");
				}
				helpToken.absType = abs_identifier;
				helpToken.specType = id_idtName;
				helpToken.value.word = word;
				return makeToken(helpToken);
			}
			break;

		default:
			error(INTERNAL_ERROR, "Scanner state not implemented");
			break;
		}
	}
}

/**
 * @brief Helper function that returns the next token - stored in peeked until
 * getToken() is called
 *
 * @return the next token from the input
 */
token peekToken(void) {
	if (!hasPeeked) {
		peeked = getTokenChecked();
		hasPeeked = 1;
	}
	return peeked;
}

/**
 * @brief Main function for getting tokens if it has peeked token, it returns it
 * and clears hasPeeked, else it calls getTokenChecked().
 *
 * @return the next token from the input
 */
token getToken(void) {
	if (hasPeeked) {
		hasPeeked = 0;
		return peeked;
	}
	return getTokenChecked();
}

/**
 * @brief Helper function that creates a final token from helpToken properties
 *
 * @param helpToken struct that holds properties of current token
 * @return new token
 */
token makeToken(token helpToken) {
	token newToken;
	newToken.absType = helpToken.absType;
	newToken.specType = helpToken.specType;
	newToken.value = helpToken.value;
	newToken.line = helpToken.line;
	newToken.col = helpToken.col;

	if (newToken.specType == ps_newLine || newToken.specType == ps_indent ||
		newToken.specType == ps_dedent) {
		lastWasNL = 1;
	} else if (newToken.specType != ps_eof) {
		lastWasNL = 0;
	}
	return newToken;
}

/**
 * @brief Helper function that stores current character into dynStr
 */
void storeSign(void) { dynStr_load(&dynStr, (char)bufferAdvance()); }

/**
 * @brief Stack algorithm indentPush - pushes indent level on top of indentStack
 * and handles realocation of memory
 *
 * @param spaces number of spaces of the new indentation level
 */
void indentPush(int spaces) {
	if (indentTop == indentCapacity) {
		size_t reallocHelperCapacity;
		if (indentCapacity == 0) {
			reallocHelperCapacity = 16;
		} else {
			reallocHelperCapacity = indentCapacity * 2;
		}
		int *reallocHelper =
			realloc(indentStack, reallocHelperCapacity * sizeof(int));
		if (reallocHelper == NULL) {
			error(INTERNAL_ERROR,
				  "Scanners indent stack realloc memory failed");
		}
		indentStack = reallocHelper;
		indentCapacity = reallocHelperCapacity;
	}
	indentStack[indentTop] = spaces;
	indentTop++;
}

/**
 * @brief Stack algorithm indentResolve - compares indent of a new line with the
 * top of indentStack and schedules INDENT/DEDENT tokens
 *
 * If the indent is bigger, it is pushed on the stack and set(pendingIndent)
 * If it is smaller, levels are popped and set(pendingDedent)
 * If the indent then does not match with spaces, it is a lexical error
 *
 * @param spaces number of spaces at current measured line
 */
void indentResolve(int spaces) {
	if (spaces > indentStack[indentTop - 1]) {
		indentPush(spaces);
		pendingIndent++;
		return;
	}
	while (spaces < indentStack[indentTop - 1]) {
		indentTop--;
		pendingDedent++;
	}
	if (spaces != indentStack[indentTop - 1]) {
		error(LEXICAL_ERROR, "Indentation does not match any outer block");
	}
}

/**
 * @brief Helper function that creates number token by assigning it correct
 * absType, specType and value
 *
 * @param helpToken struct that holds properties of current token
 * @param numType type of number literal (lit_int or lit_double) that decides
 * how the stored lexeme is converted
 * @return calls a makeToken() that uses the helpToken properties to create a
 * token
 */
token makeNumber(token helpToken, specTokenType numType) {
	char *word = dynStr_output(&dynStr);
	helpToken.absType = abs_literal;
	helpToken.specType = numType;
	if (numType == lit_int) {
		helpToken.value.num = strtoll(word, NULL, 10);
	} else {
		helpToken.value.decNum = strtod(word, NULL);
	}
	free(word);
	return makeToken(helpToken);
}

/**
 * @brief Helper function that creates string token by assinging it correct
 * absType, specType and value
 *
 * @param helpToken struct that holds properties of current token
 * @return calls a makeToken() that uses the helpToken properties to create a
 * token
 */
token makeString(token helpToken) {
	helpToken.absType = abs_literal;
	helpToken.specType = lit_string;
	helpToken.value.word = dynStr_output(&dynStr);
	return makeToken(helpToken);
}

/**
 * @brief Helper function that searches the keywords array for a given word
 *
 * @param word string that is compared with each keyword in the array
 * @return pointer to the matching keyword entry, else NULL
 */
const keyword *findKeyword(const char *word) {
	for (int keyWordHelper = 0; keywords[keyWordHelper].word != NULL;
		 keyWordHelper++) {
		if (strcmp(word, keywords[keyWordHelper].word) == 0) {
			return &keywords[keyWordHelper];
		}
	}
	return NULL;
}

/**
 * @brief Memory helper function that free space of loadedInput value
 */
void scanner_dtor(void) {
	free(input.loadedInput);
	input.loadedInput = NULL;
}

/**
 * @brief Memory helper function that free all space of whole scanner itself
 */
void scannerFree(void) {
	if (scannerActive == 0) {
		return;
	}
	if (hasPeeked == 1) {
		tokenFree(&peeked);
		hasPeeked = 0;
	}

	scanner_dtor();
	dynStr_dtor(&dynStr);
	free(indentStack);
	indentStack = NULL;
	indentTop = 0;
	indentCapacity = 0;
	pendingIndent = 0;
	pendingDedent = 0;
	scannerActive = 0;
	lastWasNL = 1;
	parenDepth = 0;
}

/**
 * @brief Memory helper function that frees value.word of tokens that own it
 * (id_idtName, intp_part, lit_string)
 *
 * @param heapUsingToken Pointer to the token whose memory is freed (id_idtName,
 * intp_part, lit_string)
 */
void tokenFree(token *heapUsingToken) {
	if (heapUsingToken->specType == id_idtName ||
		heapUsingToken->specType == intp_part ||
		heapUsingToken->specType == lit_string) {
		free(heapUsingToken->value.word);
		heapUsingToken->value.word = NULL;
	}
}