/*
 * File:	tokens.cpp
 *
 * Description:	This file contains a single variable definition for mapping
 *		token values back to their original lexemes.  Not all token
 *		values are included, just those that we will store in the
 *		AST and three-address statements.
 */

# include "tokens.h"

using namespace std;

unordered_map<int, string> lexemes = {
    {'!', "!"}, {'*', "*"}, {'/', "/"}, {'%', "%"}, {'+', "+"}, {'-', "-"},
    {'<', "<"}, {'>', ">"}, {LEQ, "<="}, {GEQ, ">="}, {EQL, "=="}, {NEQ, "!="},
    {AND, "&&"}, {OR, "||"}, {'=', "="}, {INT, "int"}, {NEGATE, "-"},
    {FUNC, "call"}, {PROC, "call"}, {INDEX, "index"}, {BLOCK, "begin"},
    {WHILE, "while"}, {DO, "do"}, {RETURN, "return"}, {FOR, "for"}, {IF, "if"},
};

unordered_map<int,int> duals {
    {'+', '+'}, {'*', '*'}, {'<', '>'}, {'>', '<'},
    {LEQ, GEQ}, {GEQ, LEQ}, {EQL, EQL}, {NEQ, NEQ},
};
