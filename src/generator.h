/*
 * File:	generator.h
 *
 * Description:	This file contains the public function declarations for the
 *		code generator for Tiny C.
 */

# ifndef GENERATOR_H
# define GENERATOR_H
# include "Function.h"

void generateFunction(Function &function);
void generateGlobals(Scope *globals);

# endif /* GENERATOR_H */
