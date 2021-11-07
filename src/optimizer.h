/*
 * File:	optimizer.h
 *
 * Description:	This file contains the public function definitions for
 *		optimizing the input program.
 */

# ifndef OPTIMIZER_H
# define OPTIMIZER_H
# include "Function.h"
# include "flowgraph.h"

void optimizeTree(Function &function);
void optimizeStatements(Function &function);

# endif /* OPTIMIZER_H */
