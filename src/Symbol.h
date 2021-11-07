/*
 * File:	Symbol.h
 *
 * Description:	This file contains the class definition for symbols in Tiny
 *		C.  A symbol consists of a name and a type, along with an
 *		indicator of its kind (local, global, literal, etc.).
 */

# ifndef SYMBOL_H
# define SYMBOL_H
# include <string>
# include <vector>
# include "tokens.h"
# include "Type.h"

typedef std::vector<class Symbol *> Symbols;

class Symbol {
    typedef std::string string;
    string _name;
    Type _type;
    int _kind;

public:
    Symbol(const string &name, const Type &type, int kind);
    const string &name() const;
    const Type &type() const;
    int kind() const;
    void kind(int k);

    int _offset;
    class Register *_register;
};

# endif /* SYMBOL_H */
