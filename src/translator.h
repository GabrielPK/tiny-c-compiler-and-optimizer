/*
 * File:	translator.h
 *
 * Description:	This file contains the public function declaration for
 *		translating an AST into three-address statements.
 */

# ifndef TRANSLATOR_H
# include "Node.h"
# include "Statement.h"

Statements translate(Node *node);

# endif /* TRANSLATOR_H */
