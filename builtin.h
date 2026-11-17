/**
 * @name ifj26
 * @authors TODO
 * @file builtin.h
 */

#ifndef BUILTIN_H
#define BUILTIN_H

/** print(value: Any): Unit */
void builtinGenPrint(void);
/** readLine(): String */
void builtinGenReadLine(void);
/** readInt(): Int */
void builtinGenReadInt(void);
/** readDouble(): Double */
void builtinGenReadDouble(void);
/** toStr(value: Any): String */
void builtinGenToStr(void);
/** toInt(value: Double): Int */
void builtinGenToInt(void);
/** strLength(value: String): Int */
void builtinGenStrLength(void);
/** subStr(value: String, start: Int, end: Int): String */
void builtinGenSubStr(void);
/** ord(value: String, index: Int): Int */
void builtinGenOrd(void);
/** chr(code: Int): String */
void builtinGenChr(void);
/** readBoolean(): Boolean */
void builtinGenReadBoolean(void);
/** x.floor: Double */
void builtinGenFloor(void);

#endif
