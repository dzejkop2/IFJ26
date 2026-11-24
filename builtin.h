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

/**
 * @brief Generates the "print" built in function
 */
void builtinGenPrint(void);
/**
 * @brief Generates the "readLine" built in function
 */
void builtinGenReadLine(void);
/**
 * @brief Generates the "readInt" built in function
 */
void builtinGenReadInt(void);
/**
 * @brief Generates the "readDouble" built in function
 */
void builtinGenReadDouble(void);
/**
 * @brief Generates the "toStr" built in function
 */
void builtinGenToStr(void);
/**
 * @brief Generates the "toInt" built in function
 */
void builtinGenToInt(void);
/**
 * @brief Generates the "strLength" built in function
 */
void builtinGenStrLength(void);
/**
 * @brief Generates the "subStr" built in function
 */
void builtinGenSubStr(void);
/**
 * @brief Generates the "ord" built in function
 */
void builtinGenOrd(void);
/**
 * @brief Generates the "chr" built in function
 */
void builtinGenChr(void);

// Extensions built in functions

/**
 * @brief Generates the "readBoolean" built in function
 */
void builtinGenReadBoolean(void);
/**
 * @brief Generates the "floor" built in member call
 */
void builtinGenFloor(void);

#endif
