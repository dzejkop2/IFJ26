/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file builtin.c
 */

#include <stdio.h>
#include "builtin.h"

/**
 * @brief Generates the "print" built in function
 */
void builtinGenPrint(void) {
	printf("LABEL $$print\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("JUMPIFEQ $$print$unit LF@value nil@nil\n");
	printf("WRITE LF@value\n");
	printf("JUMP $$print$end\n");
	printf("LABEL $$print$unit\n");
	printf("WRITE string@()\n");
	printf("LABEL $$print$end\n");
	printf("PUSHS nil@nil\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "readLine" built in function
 */
void builtinGenReadLine(void) {
	printf("LABEL $$readLine\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@result\n");
	printf("READ LF@result string\n");
	printf("DEFVAR LF@t\n");
	printf("TYPE LF@t LF@result\n");
	printf("JUMPIFEQ $$readLine$ok LF@t string@string\n");
	printf("MOVE LF@result string@\n");
	printf("LABEL $$readLine$ok\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "readInt" built in function
 */
void builtinGenReadInt(void) {
	printf("LABEL $$readInt\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@result\n");
	printf("READ LF@result int\n");
	printf("DEFVAR LF@t\n");
	printf("TYPE LF@t LF@result\n");
	printf("JUMPIFEQ $$readInt$ok LF@t string@int\n");
	printf("MOVE LF@result int@0\n");
	printf("LABEL $$readInt$ok\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "readDouble" built in function
 */
void builtinGenReadDouble(void) {
	printf("LABEL $$readDouble\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@result\n");
	printf("READ LF@result float\n");
	printf("DEFVAR LF@t\n");
	printf("TYPE LF@t LF@result\n");
	printf("JUMPIFEQ $$readDouble$ok LF@t string@float\n");
	printf("MOVE LF@result float@0x0p+0\n");
	printf("LABEL $$readDouble$ok\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "toStr" built in function
 */
void builtinGenToStr(void) {
	printf("LABEL $$toStr\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@result\n");
	printf("DEFVAR LF@type\n");
	printf("TYPE LF@type LF@value\n");
	printf("JUMPIFEQ $$toStr$int LF@type string@int\n");
	printf("JUMPIFEQ $$toStr$float LF@type string@float\n");
	printf("JUMPIFEQ $$toStr$bool LF@type string@bool\n");
	printf("JUMPIFEQ $$toStr$nil LF@type string@nil\n");
	printf("MOVE LF@result LF@value\n");
	printf("JUMP $$toStr$end\n");
	printf("LABEL $$toStr$int\n");
	printf("INT2STR LF@result LF@value\n");
	printf("JUMP $$toStr$end\n");
	printf("LABEL $$toStr$float\n");
	printf("FLOAT2STR LF@result LF@value\n");
	printf("JUMP $$toStr$end\n");
	printf("LABEL $$toStr$bool\n");
	printf("MOVE LF@result string@false\n");
	printf("JUMPIFEQ $$toStr$end LF@value bool@false\n");
	printf("MOVE LF@result string@true\n");
	printf("JUMP $$toStr$end\n");
	printf("LABEL $$toStr$nil\n");
	printf("MOVE LF@result string@()\n");
	printf("LABEL $$toStr$end\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "toInt" built in function
 */
void builtinGenToInt(void) {
	printf("LABEL $$toInt\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@result\n");
	printf("FLOAT2INT LF@result LF@value\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "strLength" built in function
 */
void builtinGenStrLength(void) {
	printf("LABEL $$strLength\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@result\n");
	printf("STRLEN LF@result LF@value\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "subStr" built in function
 */
void builtinGenSubStr(void) {
	printf("LABEL $$subStr\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@end\n");
	printf("POPS LF@end\n");
	printf("DEFVAR LF@start\n");
	printf("POPS LF@start\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@len\n");
	printf("STRLEN LF@len LF@value\n");
	printf("DEFVAR LF@tmp\n");
	printf("LT LF@tmp LF@start int@0\n");
	printf("JUMPIFEQ $$subStr$error LF@tmp bool@true\n");
	printf("LT LF@tmp LF@end int@0\n");
	printf("JUMPIFEQ $$subStr$error LF@tmp bool@true\n");
	printf("GT LF@tmp LF@start LF@end\n");
	printf("JUMPIFEQ $$subStr$error LF@tmp bool@true\n");
	printf("GT LF@tmp LF@end LF@len\n");
	printf("JUMPIFEQ $$subStr$error LF@tmp bool@true\n");
	printf("LT LF@tmp LF@start LF@len\n");
	printf("JUMPIFEQ $$subStr$error LF@tmp bool@false\n");
	printf("DEFVAR LF@result\n");
	printf("DEFVAR LF@counter\n");
	printf("DEFVAR LF@curr_char\n");
	printf("MOVE LF@result string@\n");
	printf("MOVE LF@counter LF@start\n");
	printf("LABEL $$subStr$loop\n");
	printf("JUMPIFEQ $$subStr$end LF@end LF@counter\n");
	printf("GETCHAR LF@curr_char LF@value LF@counter\n");
	printf("CONCAT LF@result LF@result LF@curr_char\n");
	printf("ADD LF@counter LF@counter int@1\n");
	printf("JUMP $$subStr$loop\n");
	printf("LABEL $$subStr$error\n");
	printf("EXIT int@40\n");
	printf("LABEL $$subStr$end\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "ord" built in function
 */
void builtinGenOrd(void) {
	printf("LABEL $$ord\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@index\n");
	printf("POPS LF@index\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@len\n");
	printf("DEFVAR LF@tmp\n");
	printf("DEFVAR LF@result\n");
	printf("STRLEN LF@len LF@value\n");
	printf("LT LF@tmp LF@index int@0\n");
	printf("JUMPIFEQ $$ord$error LF@tmp bool@true\n");
	printf("LT LF@tmp LF@index LF@len\n");
	printf("JUMPIFEQ $$ord$error LF@tmp bool@false\n");
	printf("STRI2INT LF@result LF@value LF@index\n");
	printf("JUMP $$ord$end\n");
	printf("LABEL $$ord$error\n");
	printf("MOVE LF@result int@-1\n");
	printf("LABEL $$ord$end\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "chr" built in function
 */
void builtinGenChr(void) {
	printf("LABEL $$chr\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@value\n");
	printf("POPS LF@value\n");
	printf("DEFVAR LF@result\n");
	printf("INT2CHAR LF@result LF@value\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "readBoolean" built in function
 */
void builtinGenReadBoolean(void) {
	printf("LABEL $$readBoolean\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@result\n");
	printf("READ LF@result bool\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}

/**
 * @brief Generates the "floor" built in member call
 */
void builtinGenFloor(void) {
	printf("LABEL $$floor\n");
	printf("CREATEFRAME\n");
	printf("PUSHFRAME\n");
	printf("DEFVAR LF@x\n");
	printf("POPS LF@x\n");
	printf("DEFVAR LF@int_part\n");
	printf("DEFVAR LF@back\n");
	printf("DEFVAR LF@tmp\n");
	printf("DEFVAR LF@result\n");
	printf("FLOAT2INT LF@int_part LF@x\n");
	printf("INT2FLOAT LF@back LF@int_part\n");
	printf("GT LF@tmp LF@back LF@x\n");
	printf("JUMPIFEQ $$floor$neg LF@tmp bool@true\n");
	printf("MOVE LF@result LF@back\n");
	printf("JUMP $$floor$end\n");
	printf("LABEL $$floor$neg\n");
	printf("SUB LF@result LF@back float@0x1p+0\n");
	printf("LABEL $$floor$end\n");
	printf("PUSHS LF@result\n");
	printf("POPFRAME\n");
	printf("RETURN\n");
}
