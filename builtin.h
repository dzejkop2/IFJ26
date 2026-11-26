/**
 * @name ifj26
 * @authors Tomáš Zajac (xzajact00)
 * @file builtin.h
 *
 * AI usage:
 * "Create function stubs for built in functions"
 */

#ifndef BUILTIN_H
#define BUILTIN_H

void builtinGenPrint(void);
void builtinGenReadLine(void);
void builtinGenReadInt(void);
void builtinGenReadDouble(void);
void builtinGenToStr(void);
void builtinGenToInt(void);
void builtinGenStrLength(void);
void builtinGenSubStr(void);
void builtinGenOrd(void);
void builtinGenChr(void);

// Extensions built in functions

void builtinGenReadBoolean(void);
void builtinGenFloor(void);

#endif
