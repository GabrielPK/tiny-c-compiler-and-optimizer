/*
 * File:	Scope.cpp
 *
 * Description:	This file contains the member function definitions for
 *		scopes in Tiny C.
 *
 *		We didn't allocate the symbols we're given, so we don't
 *		deallocate them.  That's the rule.  You have to do that
 *		yourself.  Besides, it's possible that they're hanging
 *		around other places, like abstract syntax trees.
 */

# include <cassert>
# include "Scope.h"

using namespace std;


/*
 * Function:	Scope::Scope (constructor)
 *
 * Description:	Initialize this scope object.
 */

Scope::Scope(Scope *enclosing)
    : _enclosing(enclosing)
{
}


/*
 * Function:	Scope::enclosing
 *
 * Description:	Return the enclosing scope of this scope.
 */

Scope *Scope::enclosing() const
{
    return _enclosing;
}


/*
 * Function:	Scope::symbols (accessor)
 *
 * Description:	Return the vector of symbols in this scope.
 */

const Symbols &Scope::symbols() const
{
    return _symbols;
}


/*
 * Function:	Scope::insert
 *
 * Description:	Insert the given symbol into this scope.  We fail if
 *		another symbol with the same name is already present.
 */

void Scope::insert(Symbol *symbol)
{
    assert(find(symbol->name()) == nullptr);
    _symbols.push_back(symbol);
}


/*
 * Function:	Scope::find
 *
 * Description:	Find and return the symbol with the given name in this
 *		scope.  If no such symbol is found, return a null pointer.
 */

Symbol *Scope::find(const string &name) const
{
    for (auto sym : _symbols)
	if (sym->name() == name)
	    return sym;

    return nullptr;
}


/*
 * Function:	Scope::lookup
 *
 * Description:	Find and return the nearest symbol with the given name,
 *		starting the search in this scope and moving into the
 *		enclosing scopes.  If no such symbol is found, return a
 *		null pointer.
 */

Symbol *Scope::lookup(const string &name) const
{
    Symbol *symbol;


    if ((symbol = find(name)) != nullptr)
	return symbol;

    return _enclosing != nullptr ? _enclosing->find(name) : nullptr;
}
