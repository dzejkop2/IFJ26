/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner_test.c
 */
/*
RunCommand: 
clang -std=c11 -Wall -Wextra -pedantic -g -fsanitize=address,undefined \
      -o scanner_test test/scanner_test.c scanner.c dynamicString.c error.c
./test/scanner_test < test.ifj26
*/

/*
AI generated scanner test file
Prompt: Write me a scanner_test.c with main that calls scannerInit(), then calls getToken()
in a loop until ps_eof and prints each token from my scanner.h enums + value for lit_int, lit_double and lit_string / id_idtName.
At the end use scannerFree(). 
*/


#include <stdio.h>
#include <stdlib.h>
#include "../scanner.h"

/*
 * Console: detailed table (position, abstract type, token, value).
 * File (default tokens.txt, overwritten on every run): compact form,
 * one logical line per row, INDENT/DEDENT at the start, nested blocks shifted:
 *
 *   def factorial ( n : Int ) : Int = NL
 *   INDENT var result = 1 NL
 *          while i > 1 do NL
 *   INDENT   result = result * i NL
 *   DEDENT result NL
 */

#define MARKER_WIDTH 7      // width of "INDENT " / "DEDENT "
#define LEVEL_SHIFT  2      // extra spaces per nested block level

static FILE *outFile = NULL;

static const char *absName(absTokenType t) {
    switch (t) {
        case abs_identifier: return "identifier";
        case abs_literal:    return "literal";
        case abs_keyword:    return "keyword";
        case abs_op:         return "op";
        case abs_dataType:   return "dataType";
        case abs_bracet:     return "bracet";
        case abs_sign:       return "sign";
        case abs_position:   return "position";
        case abs_control:    return "control";
        case abs_reserved:   return "reserved";
        case abs_interp:     return "interp";
        default:             return "???";
    }
}

static const char *specName(specTokenType t) {
    switch (t) {
        case id_idtName:     return "ID";
        case lit_int:        return "INT";
        case lit_double:     return "DOUBLE";
        case lit_string:     return "STRING";
        case lit_true:       return "true";
        case lit_false:      return "false";
        case kw_import:      return "import";
        case kw_return:      return "return";
        case kw_do:          return "do";
        case kw_while:       return "while";
        case kw_else:        return "else";
        case kw_if:          return "if";
        case kw_then:        return "then";
        case kw_var:         return "var";
        case kw_val:         return "val";
        case kw_def:         return "def";
        case kw_main:        return "@main";
        case kw_for:         return "for";
        case kw_extension:   return "extension";
        case op_biggerThen:  return ">";
        case op_lowerThen:   return "<";
        case op_equal:       return "==";
        case op_assign:      return "=";
        case op_notEqual:    return "!=";
        case op_lowerEqual:  return "<=";
        case op_biggerEqual: return ">=";
        case op_add:         return "+";
        case op_sub:         return "-";
        case op_mul:         return "*";
        case op_div:         return "/";
        case op_and:         return "&&";
        case op_or:          return "||";
        case op_not:         return "!";
        case dt_int:         return "Int";
        case dt_double:      return "Double";
        case dt_string:      return "String";
        case dt_unit:        return "Unit";
        case dt_bool:        return "Boolean";
        case br_start:       return "(";
        case br_end:         return ")";
        case sn_semiColon:   return ";";
        case sn_colon:       return ":";
        case sn_comma:       return ",";
        case sn_dot:         return ".";
        case sn_underscore:  return "_";
        case ps_eof:         return "EOF";
        case ps_indent:      return "INDENT";
        case ps_dedent:      return "DEDENT";
        case ps_newLine:     return "NL";
        case ct_arrowLeft:   return "<-";
        case res_case:       return "case";
        case res_class:      return "class";
        case res_list:       return "List";
        case res_match:      return "match";
        case res_type:       return "type";
        case intp_begin:     return "s\"";
        case intp_part:      return "PART";
        case intp_exprBegin: return "${";
        case intp_exprEnd:   return "}";
        case intp_end:       return "\"";
        default:             return "???";
    }
}

// writes a string literal with escapes, so "\n" inside a string stays on one line
static void writeEscaped(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s != '\0'; s++) {
        if (*s == '\n') {
            fputs("\\n", f);
        } else if (*s == '\t') {
            fputs("\\t", f);
        } else if (*s == '\r') {
            fputs("\\r", f);
        } else if (*s == '"') {
            fputs("\\\"", f);
        } else if (*s == '\\') {
            fputs("\\\\", f);
        } else {
            fputc(*s, f);
        }
    }
    fputc('"', f);
}

// writes the token as it appears in the source (ID name, literal value, operator...)
static void writeTokenText(FILE *f, token t) {
    if (t.specType == id_idtName) {
        fputs(t.value.word, f);
    } else if (t.specType == lit_int) {
        fprintf(f, "%lld", t.value.num);
    } else if (t.specType == lit_double) {
        fprintf(f, "%g", t.value.decNum);
    } else if (t.specType == lit_string) {
        writeEscaped(f, t.value.word);
    } else {
        fputs(specName(t.specType), f);
    }
}

// console: one detailed row per token
static void printTableRow(token t) {
    char pos[16];
    snprintf(pos, sizeof(pos), "%d:%d", t.line, t.col);
    printf("%-7s %-10s %-8s", pos, absName(t.absType), specName(t.specType));
    if (t.specType == lit_int || t.specType == lit_double ||
        t.specType == lit_string || t.specType == id_idtName) {
        putchar(' ');
        writeTokenText(stdout, t);
    }
    putchar('\n');
}

/* ---------- compact file output ---------- */

static int level = 0;          // current block nesting (INDENT +1, DEDENT -1)
static int atLineStart = 1;    // nothing printed yet on the current row
static int markerLen = 0;      // characters of INDENT/DEDENT markers already printed on this row

// called before the first real token of a row: pad markers and shift by block level
static void writeLinePrefix(void) {
    if (level > 0 || markerLen > 0) {
        for (int i = markerLen; i < MARKER_WIDTH; i++) {
            fputc(' ', outFile);
        }
        for (int i = 0; i < (level - 1) * LEVEL_SHIFT; i++) {
            fputc(' ', outFile);
        }
    }
    atLineStart = 0;
}

static void writeCompact(token t) {
    if (t.specType == ps_indent || t.specType == ps_dedent) {
        fputs(t.specType == ps_indent ? "INDENT " : "DEDENT ", outFile);
        markerLen += MARKER_WIDTH;
        level += (t.specType == ps_indent) ? 1 : -1;
    } else {
        if (atLineStart) {
            writeLinePrefix();
        }
        writeTokenText(outFile, t);
        if (t.specType == ps_newLine || t.specType == ps_eof) {
            fputc('\n', outFile);
            atLineStart = 1;
            markerLen = 0;
        } else {
            fputc(' ', outFile);
        }
    }
    fflush(outFile);   // keep what was written even if the scanner exits on error
}

int main(int argc, char **argv) {
    const char *outName = (argc > 1) ? argv[1] : "tokens.txt";
    outFile = fopen(outName, "w");   // "w" = always a new (overwritten) file
    if (outFile == NULL) {
        fprintf(stderr, "Cannot create output file '%s'\n", outName);
        return 99;
    }

    scannerInit();

    printf("%-7s %-10s %-8s %s\n", "POS", "ABS", "TOKEN", "VALUE");
    printf("---------------------------------------------\n");

    for (;;) {
        token t = getTokenChecked();

        printTableRow(t);
        writeCompact(t);

        // the token owns value.word for ID and STRING -> the receiver must free it
        if (t.specType == id_idtName || t.specType == lit_string || t.specType == intp_part) {
            free(t.value.word);
        }
        
        if (t.specType == ps_eof) {
            break;
        }
    }

    scannerFree();
    fclose(outFile);
    fprintf(stderr, "Tokens written to %s\n", outName);
    return 0;
}
