#include <stdio.h>
#include <stdlib.h>
#include "../scanner.h"

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

int dumpTokens(void) {
    scannerInit();

    printf("%-7s %-10s %-8s %s\n", "POS", "ABS", "TOKEN", "VALUE");
    printf("---------------------------------------------\n");

    for (;;) {
        token t = getTokenChecked();

        printTableRow(t);

        if (t.specType == id_idtName || t.specType == lit_string || t.specType == intp_part) {
            free(t.value.word);
        }

        if (t.specType == ps_eof) {
            break;
        }
    }

    scannerFree();
    return 0;
}
