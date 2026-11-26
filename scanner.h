/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.h
 */

#ifndef SCANNER_H
#define SCANNER_H

#include <stddef.h>

/**
 * @brief Identifies types of absTokenType
 */
typedef enum {
	abs_identifier,
	abs_literal,
	abs_keyword,
	abs_op,
	abs_dataType,
	abs_bracet,
	abs_sign,
	abs_position,
	abs_control,
	abs_reserved,
	abs_intp,
} absTokenType;

/**
 * @brief Identifies types of specTokenType
 */
typedef enum {
	// identifier
	id_idtName,

	// literal
	lit_int,
	lit_double,
	lit_string,
	// BONUS
	lit_true,
	lit_false,

	// keyword
	kw_import,
	kw_return,
	kw_do,
	kw_while,
	kw_else,
	kw_if,
	kw_then,
	kw_var,
	kw_val,
	kw_def,
	kw_main,
	// BONUS
	kw_for,
	kw_extension,

	// op
	op_biggerThen,
	op_lowerThen,
	op_equal,
	op_assign,
	op_notEqual,
	op_lowerEqual,
	op_biggerEqual,
	op_add,
	op_sub,
	op_mul,
	op_div,
	// BONUS
	op_and,
	op_or,
	op_not,

	// dataType
	dt_int,
	dt_double,
	dt_string,
	dt_unit,
	// BONUS
	dt_bool,

	// bracet
	br_start,
	br_end,

	// sign
	sn_semiColon,
	sn_colon,
	sn_comma,
	sn_dot,
	sn_underscore,
	// sn_quote,

	// position
	ps_eof,
	ps_indent,
	ps_dedent,
	ps_newLine,
	// BONUS
	// ps_paraStart,
	// ps_paraEnd,

	// control
	// BONUS
	ct_arrowLeft,
	// ct_to,
	// ct_until,
	// ct_by,
	// reserved
	res_case,
	res_class,
	res_list,
	res_match,
	res_type,
	// interpolate
	intp_begin,
	intp_part,
	intp_exprBegin,
	intp_exprEnd,
	intp_end,
} specTokenType;

/**
 * @brief Struct that wrapps whole token architecture
 * @param absType   defines abstract type of the token
 * @param specType  defines specific type of the token
 * @param value     defines abstract type of the token
 * @param line      defines the exact line position in codebase
 * @param col       defines the level of encasement
 */
typedef struct {
	absTokenType absType;
	specTokenType specType;
	union {
		long long num;
		double decNum;
		char *word;
	} value;
	int line;
	int col;
} token;

/**
 * @brief Struct that holds whole source and current reading position
 * @param loadedInput buffer with whole source code
 * @param lenght      number of loaded characters
 * @param pos         index of current character in loadedInput
 * @param col         current column position in source code
 * @param line        current line position in source code
 */
typedef struct {
	char *loadedInput;
	size_t lenght;
	size_t pos;
	int col;
	int line;
} stdInput;

/**
 * @brief Struct that represents one entry of keywords array
 * @param word     keyword string
 * @param absType  abstract type of the keyword token
 * @param specType specific type of the keyword token
 */
typedef struct {
	const char *word;
	absTokenType absType;
	specTokenType specType;
} keyword;

/**
 * @brief Identifies states of lexer finite state machine
 */
typedef enum {
	// start state
	STATE_START,

	// identifiers
	STATE_IDENTIFIER,
	STATE_UNDERSCORE,
	STATE_MAIN,

	// num literals
	STATE_ZERO,
	STATE_ZERO_DIGITS,
	STATE_INT,
	STATE_INT_UNDERSCORE,
	STATE_INT_DOT,
	STATE_DOUBLE_DOT,
	STATE_DOUBLE,
	STATE_MID_UNDERSCORE,
	STATE_DOUBLE_UNDERSCORE,
	STATE_NUM_EXP,
	STATE_SIGN_NUM_EXP,
	STATE_DOUBLE_EXP,
	STATE_EXP_UNDERSCORE,
	STATE_DOUBLE_SUFFIX,

	// strings
	STATE_QUOTES_START,
	STATE_QUOTES_BODY,
	STATE_QUOTES_ESCAPE,
	STATE_STRING_END,
	STATE_STRING_EMPTY,
	STATE_MUL_STRING_START,
	STATE_MUL_STRING_1,
	STATE_MUL_STRING_2,
	STATE_MUL_STRING_END,

	// comments
	STATE_LINECOMMENT,
	STATE_BLOCKCOMMENT_MID,
	STATE_BLOCKCOMMENT_END,
	STATE_BLOCKCOMMENT_SLASH,
	STATE_BLOCKCOMMENT_AFTER_NL,

	// operators
	STATE_ADD,
	STATE_SUB,
	STATE_MUL,
	STATE_DIV,
	STATE_ASSIGN,
	STATE_EQUAL,
	STATE_BIGGER_THEN,
	STATE_BIGGER_EQUAL,
	STATE_LOWER_THEN,
	STATE_LOWER_EQUAL,
	STATE_ARROW_LEFT,
	STATE_NOT,
	STATE_NOT_EQUAL,
	STATE_AMP,
	STATE_PIPE,

	// interpolates
	STATE_INTP_BODY,
	STATE_INTP_DOLLAR,
	STATE_INTP_ESCAPE,
	STATE_INTP_ID,

	// others
	STATE_SIGN,
	STATE_NEWLINE,
	STATE_SLASH,
	STATE_TAB,
	STATE_TAB_SLASH,
	STATE_TAB_NL_SLASH,
	STATE_EOF
} states;

/**
 * @brief Identifies modes that are stored on modeStack (normal code,
 * interpolated string, expression inside ${})
 */
typedef enum {
	MODE_INTERP,
	MODE_INTP_MULTI,
	MODE_EXPR,
	MODE_NORMAL,
} mode;

extern const keyword keywords[];

token peekToken(void);
token getTokenChecked(void);
token getToken(void);
token makeToken(token helpToken);
void tokenFree(token *heapUsingToken);
const keyword *findKeyword(const char *word);

int bufferAdvance(void);
void scannerFree(void);
void scannerInit(void);
void scanner_dtor(void);
token getToken(void);

#endif
