/*
 * File:	Type.h
 *
 * Description:	This file contains the class definition for types in Tiny
 *		C.  A type is either a scalar, an array, or a function, as
 *		indicated by its declarator.  An array type also includes
 *		its length, and a function type includes its parameters.
 *
 *		By convention, a null parameter list represents an
 *		unspecified parameter list, and an empty parameter list is
 *		represented by an empty vector.
 */

# ifndef TYPE_H
# define TYPE_H
# include <vector>

typedef std::vector<class Type> Types;

class Type {
    short _declarator, _specifier;
    unsigned _length;
    Types *_parameters;

public:
    Type(int specifier);
    Type(int specifier, unsigned length);
    Type(int specifier, Types *parameters);

    bool isArray() const;
    bool isFunction() const;
    bool isScalar() const;
    bool isPointer() const;

    int specifier() const;
    unsigned length() const;
    Types *parameters() const;

    unsigned size() const;
};

# endif /* TYPE_H */
