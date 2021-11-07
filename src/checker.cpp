/*
 * File:	checker.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for the semantic checker for Tiny C.
 */

# include <vector>
# include <cassert>
# include "lexer.h"
# include "tokens.h"
# include "checker.h"

# define SYM_TOKEN (current == globals ? GLOBAL : LOCAL)


using namespace std;

static Scope *globals, *current;


/*
 * Function:	initializeScope
 *
 * Description:	Create a new scope and make it the new top-level scope.
 *		The new scope is returned for convenience.
 */

Scope *initializeScope()
{
    current = new Scope(current);

    if (globals == nullptr)
	globals = current;

    return current;
}


/*
 * Function:	finalizeScope
 *
 * Description:	Remove the top-level scope, and make its enclosing scope
 *		the new top-level scope.  The old top-level scope is
 *		returned for convenience.
 */

Scope *finalizeScope()
{
    Scope *old;


    assert(current != nullptr);

    old = current;
    current = current->enclosing();

    return old;
}


/*
 * Function:	insertName
 *
 * Description:	Insert (i.e., declare) in the current scope a symbol with
 *		the given name and type.  If a symbol with that name
 *		already exists, report an error and discard the
 *		redeclaration.
 */

Symbol *insertName(const string &name, const Type &type)
{
    Symbol *symbol;


    assert(current != nullptr);
    symbol = current->find(name);

    if (symbol != nullptr) {
	report("'%s' redeclared", name);
	return symbol;
    }

    symbol = new Symbol(name, type, SYM_TOKEN);
    current->insert(symbol);

    return symbol;
}


/*
 * Function:	lookupName
 *
 * Description:	Lookup and return the nearest symbol with the given name.
 *		No error is reported if no such symbol is found.
 */

Symbol *lookupName(const string &name)
{
    assert(current != nullptr);
    return current->lookup(name);
}


/*
 * Function:	lookupArray
 *
 * Description:	Lookup and return the symbol with the given name and ensure
 *		that it has an array type.  If no such symbol exists,
 *		report an error but insert a new symbol to prevent future
 *		errors.
 */

Symbol *lookupArray(const string &name)
{
    Symbol *symbol;


    assert(current != nullptr);
    symbol = current->lookup(name);

    if (symbol == nullptr) {
	report("'%s' undeclared", name);
	symbol = new Symbol(name, Type(INT, 1), SYM_TOKEN);
	current->insert(symbol);

    } else if (!symbol->type().isArray())
	report("array type required for '%s'", name);

    return symbol;
}


/*
 * Function:	lookupFunction
 *
 * Description:	Lookup and return the symbol with the given name and ensure
 *		that it has a function type.  If no such symbol exists,
 *		do not report an error but instead insert an implicit
 *		function declaration in the global scope.
 */

Symbol *lookupFunction(const string &name)
{
    Symbol *symbol;


    assert(current != nullptr);
    symbol = current->lookup(name);

    if (symbol == nullptr) {
	symbol = new Symbol(name, Type(INT, nullptr), SYM_TOKEN);
	globals->insert(symbol);

    } else if (!symbol->type().isFunction())
	report("function type required for '%s'", name);

    return symbol;
}


/*
 * Function:	lookupScalar
 *
 * Description:	Lookup and return the symbol with the given name and ensure
 *		that it has a scalar type.  If no such symbol exists,
 *		report an error but insert a new symbol to prevent future
 *		errors.
 */

Symbol *lookupScalar(const string &name)
{
    Symbol *symbol;


    assert(current != nullptr);
    symbol = current->lookup(name);

    if (symbol == nullptr) {
	report("'%s' undeclared", name);
	symbol = new Symbol(name, Type(INT), SYM_TOKEN);
	current->insert(symbol);

    } else if (!symbol->type().isScalar())
	report("scalar type required for '%s'", name);

    return symbol;
}


/*
 * Function:	checkArray
 */

void checkArray(Symbol *symbol)
{
    if (symbol->type().isArray() && symbol->type().length() == 0)
	report("'%s' has zero length", symbol->name());
}


/*
 * Function:	checkCall
 *
 * Description:	Check the arguments in a function call expression.  If the
 *		symbol does not have a function type, no error is reported.
 *		If the function is implicitly declared, then no checking
 *		can be performed.  Otherwise, the number of formals and
 *		actuals must agree, the individual declarators must agree,
 *		the actual cannot have a function type, and in the case of
 *		arrays, the specifiers must agree.
 */

Node *checkCall(Node *expr)
{
    Symbol *symbol;
    Types *formals;
    unsigned i;


    assert(expr->token() == FUNC || expr->token() == PROC);
    symbol = expr->kids(0)->symbol();

    if (!symbol->type().isFunction())
	return expr;

    formals = symbol->type().parameters();

    if (formals == nullptr)
	return expr;

    if (formals->size() != expr->kids().size() - 1) {
	report("invalid arguments to '%s'", symbol->name());
	return expr;
    }

    for (i = 0; i < formals->size(); i ++) {
	Type actual = expr->kids(i + 1)->type();
	Type formal = formals->at(i);

	if (actual.isFunction() || (actual.isScalar() && !formal.isScalar()))
	    break;

	if (actual.isArray()) {
	    if (!formal.isArray())
		break;
	    else if (actual.specifier() != formal.specifier())
		break;
	}
    }

    if (i != formals->size())
	report("invalid arguments to '%s'", symbol->name());

    return expr;
}
