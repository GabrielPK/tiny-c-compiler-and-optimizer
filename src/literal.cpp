/*
 * File:	literal.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable declarations for handling integer and string
 *		literals as symbols.
 */

# include <cassert>
# include <unordered_map>
# include "literal.h"
# include "string.h"

using namespace std;
static unordered_map<string, Symbol *> literals;


/*
 * Function:	makeLiteral
 *
 * Description:	Insert a literal with the given name.  For uniformity,
 *		string, character, and integer literals are all represented
 *		as symbols; however, we only need one copy of each.  The
 *		type of the literal is inferred from its name.  A string
 *		literal is first parsed and then escaped in order to create
 *		a canonical version.
 */

Symbol *makeLiteral(const string &name)
{
    Symbol *symbol;
    string parsed, escaped;


    if (name[0] == '"') {
	parsed = parseString(name);
	escaped = escapeString(parsed);
	symbol = literals[escaped];

	if (symbol == nullptr) {
	    symbol = new Symbol(escaped, Type(CHAR, parsed.size() + 1), STRLIT);
	    literals[escaped] = symbol;
	}

    } else {
	symbol = literals[name];

	if (symbol == nullptr) {
	    symbol = new Symbol(name, Type(INT), NUM);
	    literals[name] = symbol;
	}
    }

    return symbol;
}


/*
 * Function:	makeLiteral
 *
 * Description:	Insert an integer literal with the given value.
 */

Symbol *makeLiteral(int value)
{
    return makeLiteral(to_string(value));
}


/*
 * Function:	getLiterals
 *
 * Description:	Return a vector of all literals.
 */

const Symbols getLiterals()
{
    Symbols all;

    for (auto p : literals)
	all.push_back(p.second);

    return all;
}


/*
 * Function:	valueOf
 *
 * Description:	Return the value of a symbol, which must be a number.
 */

int valueOf(Symbol *sym)
{
    assert(sym->kind() == NUM);
    return stoi(sym->name());
}


/*
 * Function:	isNumber
 *
 * Description:	Check if the given symbol is a number.
 */

bool isNumber(Symbol *sym)
{
    return sym->kind() == NUM;
}
