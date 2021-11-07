/*
 * File:	Type.cpp
 *
 * Description:	This file contains the member function definitions for
 *		types in Tiny C.  A type is either a scalar, an array, or a
 *		function, as indicated by its declarator.
 *
 *		Types are immutable, as only accessors are provided.  The
 *		declarator is inferred from the constructor.
 */

# include <cassert>
# include "machine.h"
# include "tokens.h"
# include "Type.h"

using namespace std;

enum { ARRAY, FUNCTION, SCALAR };


/*
 * Function:	Type::Type (constructor)
 *
 * Description:	Initialize this type as a scalar type.
 */

Type::Type(int specifier)
    : _declarator(SCALAR), _specifier(specifier)
{
}


/*
 * Function:	Type::Type (constructor)
 *
 * Description:	Initialize this type as an array type.
 */

Type::Type(int specifier, unsigned length)
    : _declarator(ARRAY), _specifier(specifier), _length(length)
{
}


/*
 * Function:	Type::Type (constructor)
 *
 * Description:	Initialize this type as a function type.
 */

Type::Type(int specifier, Types *parameters)
    : _declarator(FUNCTION), _specifier(specifier), _parameters(parameters)
{
}


/*
 * Function:	Type::isArray (predicate)
 *
 * Description:	Return whether this type is an array type.
 */

bool Type::isArray() const
{
    return _declarator == ARRAY;
}


/*
 * Function:	Type::isFunction (predicate)
 *
 * Description:	Return whether this type is a function type.
 */

bool Type::isFunction() const
{
    return _declarator == FUNCTION;
}


/*
 * Function:	Type::isScalar (predicate)
 *
 * Description:	Return whether this type is a scalar type.
 */

bool Type::isScalar() const
{
    return _declarator == SCALAR;
}


/*
 * Function:	Type::isPointer (predicate)
 *
 * Description:	Return whether this type represents a pointer type.  A
 *		pointer type is only possible in Tiny C through an array
 *		parameter, which is denoted by a zero-length array.
 */

bool Type::isPointer() const
{
    return _declarator == ARRAY && _length == 0;
}


/*
 * Function:	Type::specifier (accessor)
 *
 * Description:	Return the specifier of this type.
 */

int Type::specifier() const
{
    return _specifier;
}


/*
 * Function:	Type::length (accessor)
 *
 * Description:	Return the length of this type, which must be an array.
 */

unsigned Type::length() const
{
    assert(_declarator == ARRAY);
    return _length;
}


/*
 * Function:	Type::parameters (accessor)
 *
 * Description:	Return the parameters of this type, which must be a
 *		function.
 */

Types *Type::parameters() const
{
    assert(_declarator == FUNCTION);
    return _parameters;
}


/*
 * Function:	Type::size
 *
 * Description:	Return the size of this type in bytes.  The size of an
 *		array is the length of the array multiplied by the size of
 *		a single element in the array.
 */

unsigned Type::size() const
{
    unsigned count;


    if (isPointer())
	return SIZEOF_PTR;

    count = (_declarator == ARRAY ? _length : 1);
    return count * (_specifier == INT ? SIZEOF_INT : SIZEOF_CHAR);
}
