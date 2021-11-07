/*
 * File:	Symbol.cpp
 *
 * Description:	This file contains the member function definitions for
 *		symbols in Tiny C.  A symbol consists of a name and a type,
 *		along with an indicator of its kind (variable, literal,
 *		etc.)
 */

# include "Symbol.h"

using namespace std;


/*
 * Function:	Symbol::Symbol (constructor)
 *
 * Description:	Initialize this symbol with the specified name and type.
 */

Symbol::Symbol(const string &name, const Type &type, int kind)
    : _name(name), _type(type), _kind(kind), _offset(0), _register(nullptr)
{
}


/*
 * Function:	Symbol::name (accessor)
 *
 * Description:	Return the name of this symbol.
 */

const string &Symbol::name() const
{
    return _name;
}


/*
 * Function:	Symbol::type (accessor)
 *
 * Description:	Return the type of this symbol.
 */

const Type &Symbol::type() const
{
    return _type;
}


/*
 * Function:	Symbol::kind (accessor)
 *
 * Description:	Return the kind of this symbol.
 */

int Symbol::kind() const
{
    return _kind;
}


/*
 * Function:	Symbol::kind (mutator)
 *
 * Description:	Update the kind of this symbol.
 */

void Symbol::kind(int k)
{
    _kind = k;
}
