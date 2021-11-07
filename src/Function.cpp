/*
 * File:	Function.cpp
 *
 * Description:	This file contains the member function and variable definitions
 *		for a function, which is simply a container for related
 *		information about a given function.
 */

# include "Function.h"

std::unordered_map<Symbol *, Function> functions;


/*
 * Function:	Function::Function (constructor)
 *
 * Description:	Initialize an empty function object.
 */

Function::Function()
    : body(nullptr), exit(nullptr), entry(nullptr),
      locals(nullptr), symbol(nullptr)
{
}
