/*
 * File:	lexer.h
 *
 * Description:	This file contains the public function and variable
 *		declarations for the lexical analyzer for Tiny C.
 */

# ifndef LEXER_H
# define LEXER_H
# include <string>

extern char *yytext;
extern int yylineno, numerrors;

int yylex();
void report(const std::string &str, const std::string &arg = "");

# endif /* LEXER_H */
