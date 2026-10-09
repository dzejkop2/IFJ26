/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.h
 */

#ifndef SCANNER_H
#define SCANNER_H

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
    abs_reserved
} absTokenType;

/**
 * @brief Identifies types of specTokenType
 */
typedef enum {
    //identifier
        id_idtName,
    
    //literal
        lit_int,
        lit_double,
        lit_string,
//BONUS
        lit_true,
        lit_false,
    
    //keyword
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
//BONUS
        kw_for,
        kw_extension,
    
    //op
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
//BONUS
        op_and,
        op_or,
        // op_neg, toto neviem ci tam vlastne v tom zadani je... Spytat sa Kuba
        op_not,
        
    //dataType
        dt_int,
        dt_double,
        dt_string,
        dt_unit,
//BONUS
        dt_bool,
    
    //bracet
        br_start,
        br_end,

    //sign
        sn_semiColon,
        sn_colon,
        sn_comma,
        sn_dot,
        sn_underscore,
        // sn_quote,

    //position
        ps_eof,
        ps_indent,
        ps_dedent,
        ps_newLine,
//BONUS
        // ps_paraStart,
        // ps_paraEnd,
    
    //control
//BONUS
        ct_arrowLeft,
        ct_to,
        ct_until,
        ct_by,
    //reserved
        res_case,
        res_class,
        res_list,
        res_match,
        res_type,
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
} Token;


#endif