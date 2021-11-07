/*
 * File:	checker.h
 *
 * Description:	This file contains the public function declarations for the
 *		semantic checker for Tiny C.
 */

# ifndef CHECKER_H
# define CHECKER_H
# include "Node.h"
# include "Scope.h"

Scope *initializeScope();
Scope *finalizeScope();

Symbol *insertName(const std::string &name, const Type &type);

Symbol *lookupName(const std::string &name);
Symbol *lookupArray(const std::string &name);
Symbol *lookupFunction(const std::string &name);
Symbol *lookupScalar(const std::string &name);

void checkArray(Symbol *symbol);
Node *checkCall(Node *expr);

# endif /* CHECKER_H */
