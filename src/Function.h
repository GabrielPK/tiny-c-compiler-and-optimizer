/*
 * File:	Function.h
 *
 * Description:	This file contains the class and variable declarations for
 *		a function, which is simply a container for related
 *		information about a given function.
 */

# ifndef FUNCTION_H
# define FUNCTION_H
# include "Node.h"
# include "Block.h"
# include "Scope.h"
# include <unordered_map>

struct Function {
    Node *body;
    Block *exit;
    Block *entry;
    Scope *locals;
    Symbol *symbol;
    Statements stmts;

    Function();
};

extern std::unordered_map<Symbol *, Function> functions;

# endif /* FUNCTION_H */
