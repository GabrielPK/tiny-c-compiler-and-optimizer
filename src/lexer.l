%{
/*
 * File:	lexer.l
 *
 * Description:	This file contains the flex description for the lexical
 *		analyzer for Tiny C.
 */

# include <cerrno>
# include <cstdlib>
# include <iostream>
# include "lexer.h"
# include "tokens.h"
# include "string.h"

using namespace std;

int numerrors = 0;
static void checkNumber();
static void checkString(bool limited, const string &msg);
%}

%option nounput noyywrap yylineno
%%

"auto"				{return AUTO;}
"break"				{return BREAK;}
"case"				{return CASE;}
"char"				{return CHAR;}
"const"				{return CONST;}
"continue"			{return CONTINUE;}
"default"			{return DEFAULT;}
"do"				{return DO;}
"double"			{return DOUBLE;}
"else"				{return ELSE;}
"enum"				{return ENUM;}
"extern"			{return EXTERN;}
"float"				{return FLOAT;}
"for"				{return FOR;}
"goto"				{return GOTO;}
"if"				{return IF;}
"int"				{return INT;}
"long"				{return LONG;}
"register"			{return REGISTER;}
"return"			{return RETURN;}
"short"				{return SHORT;}
"signed"			{return SIGNED;}
"sizeof"			{return SIZEOF;}
"static"			{return STATIC;}
"struct"			{return STRUCT;}
"switch"			{return SWITCH;}
"typedef"			{return TYPEDEF;}
"union"				{return UNION;}
"unsigned"			{return UNSIGNED;}
"void"				{return VOID;}
"volatile"			{return VOLATILE;}
"while"				{return WHILE;}

"||"				{return OR;}
"&&"				{return AND;}
"=="				{return EQL;}
"!="				{return NEQ;}
"<="				{return LEQ;}
">="				{return GEQ;}
"++"				{return INC;}
"--"				{return DEC;}
[-=<>+*/%!()\[\]{};,]		{return *yytext;}

[0-9]+				{checkNumber(); return NUM;}
[_a-zA-Z][_a-zA-Z0-9]*		{return NAME;}

\"(\\.|[^\\\n\"])*\"		{checkString(false, "string"); return STRLIT;}
\'(\\.|[^\\\n\'])+\'		{checkString(true, "character"); return CHARLIT;}

[ \f\n\r\t\v]+			{/* ignored */}
\/\*([^*]|\*+[^/])*\*+\/	{/* ignored */}

.				{return ERROR;}

%%

/*
 * Function:	checkNumber (private)
 *
 * Description:	Check if an integer constant is valid.
 */

void checkNumber()
{
    long val;


    errno = 0;
    val = strtol(yytext, NULL, 0);

    if (errno != 0 || val != (int) val)
	report("integer constant too large");
}


/*
 * Function:	checkString (private)
 *
 * Description:	Check if a string or character literal is valid.
 */

void checkString(bool limited, const string &msg)
{
    bool invalid, overflow;
    string s(yytext + 1, yyleng - 2);


    s = parseString(s, invalid, overflow);

    if (invalid)
        report("unknown escape sequence in %s constant", msg);
    else if (overflow)
        report("escape sequence out of range in %s constant", msg);
    else if (limited && s.size() > 1)
	report("multi-character character constant");
}


/*
 * Function:	report
 *
 * Description:	Report an error to the standard error prefixed with the
 *		line number.  We'll be using this a lot later with an
 *		optional string argument, but C++'s stupid streams don't do
 *		positional arguments, so we actually resort to snprintf.
 *		You just can't beat C for doing things down and dirty.
 */

void report(const string &str, const string &arg)
{
    char buf[1000];

    snprintf(buf, sizeof(buf), str.c_str(), arg.c_str());
    cerr << "line " << yylineno << ": " << buf << endl;
    numerrors ++;
}
