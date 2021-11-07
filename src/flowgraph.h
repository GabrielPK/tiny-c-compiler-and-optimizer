/*
 * File:	flowgraph.h
 *
 * Description:	This file contains the function definitions for
 *		constructing and optimizing the control-flow graph.
 */

# ifndef FLOWGRAPH_H
# define FLOWGRAPH_H
# include "Function.h"

void rebuildFlowgraph(Function &function);
Blocks getBlocks(Function &function);

# endif /* FLOWGRAPH_H */
