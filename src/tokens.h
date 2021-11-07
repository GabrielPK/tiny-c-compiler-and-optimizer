/*
 * File:	tokens.h
 *
 * Description:	This file contains the token definitions for use by the
 *		lexical analyzer and parser for Tiny C.  Single character
 *		tokens use their ASCII values, so we can refer to them
 *		either as character literals or as symbolic names.
 */

# ifndef TOKENS_H
# define TOKENS_H
# include <string>
# include <unordered_map>

enum {
    ASSIGN = '=', LTN = '<', GTN = '>', PLUS = '+', MINUS = '-',
    STAR = '*', DIV = '/', REM = '%', NOT = '!',
    LPAREN = '(', RPAREN = ')', LBRACK = '[', RBRACK = ']',
    LBRACE = '{', RBRACE = '}', SEMI = ';', COMMA = ',',

    AUTO = 256, BREAK, CASE, CHAR, CONST, CONTINUE, DEFAULT, DO, DOUBLE,
    ELSE, ENUM, EXTERN, FLOAT, FOR, GOTO, IF, INT, LONG, REGISTER,
    RETURN, SHORT, SIGNED, SIZEOF, STATIC, STRUCT, SWITCH, TYPEDEF,
    UNION, UNSIGNED, VOID, VOLATILE, WHILE,

    OR, AND, EQL, NEQ, LEQ, GEQ, INC, DEC, NEGATE, INDEX, FUNC, PROC, BLOCK,
    LOCAL, GLOBAL, TEMP, NAME, NUM, STRLIT, CHARLIT, DONE = 0, ERROR = -1
};

extern std::unordered_map<int, std::string> lexemes;
extern std::unordered_map<int, int> duals;

# endif /* TOKENS_H */
