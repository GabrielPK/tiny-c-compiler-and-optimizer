/*
 * File:	parser.c
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for the recursive-descent parser for
 *		Tiny C.
 */

# include <string>
# include <cstdio>
# include <cstdlib>
# include <iostream>
# include "unistd.h"
# include "Node.h"
# include "lexer.h"
# include "tokens.h"
# include "string.h"
# include "checker.h"
# include "literal.h"
# include "generator.h"
# include "optimizer.h"
# include "translator.h"
# include <getopt.h>
# include "opflgs.h"
using namespace std;

int dce_on=0;
int cprop_on=0;
int lvn_on=0;
int asimp_on=0;
int cfold_on = 0;



static int word, peeked;
static string lexeme;
static Node *expression(), *statement();

static enum {
    OUTPUT_ASM, OUTPUT_AST, OUTPUT_TAC,
} output_format;


/*
 * Function:	peek
 *
 * Description:	Return the next word from the lexer, but do not fetch it.
 */

static int peek()
{
    if (peeked == 0)
	peeked = yylex();

    return peeked;
}


/*
 * Function:	nextWord
 *
 * Description:	Return the next word from the lexer.  The textbook calls
 *		such a function 'nextWord' and calls the text that was
 *		matched 'lexeme' so we do as well.
 */

static int nextWord()
{
    int token;
    
    
    if (peeked != 0) {
	token = peeked;
	peeked = 0;
    } else
	token = yylex();

    lexeme = yytext;
    return token;
}


/*
 * Function:	error
 *
 * Description:	Report a syntax error to standard error.
 */

static void error()
{
    cerr << "line " << yylineno;
    cerr << ": syntax error at '" << yytext << "'" << endl;
    exit(EXIT_FAILURE);
}


/*
 * Function:	match
 *
 * Description:	Match the next token against the specified token.  A
 *		failure indicates a syntax error and will terminate the
 *		program since our parser does not do error recovery.
 */

static void match(int token)
{
    if (word != token)
	error();

    word = nextWord();
}


/*
 * Function:	argument
 *
 * Description:	Parse an argument to a function call.  The only place
 *		string literals are allowed in Tiny C is here, to enable
 *		calling printf(), scanf(), and the like.
 *
 *		Argument:
 *		  strlit
 *		  name
 *		  Expression
 */

static Node *argument()
{
    Node *expr;
    Symbol *symbol;


    if (word == STRLIT) {
	expr = new Node(STRLIT, makeLiteral(lexeme));
	match(STRLIT);

    } else if (word == NAME && (peek() == ',' || peek() == ')')) {
	symbol = lookupName(lexeme);

	if (symbol == nullptr)
	    symbol = lookupScalar(lexeme);

	expr = new Node(NAME, symbol);
	match(NAME);

	if (expr->type().isScalar() && expr->type().specifier() == CHAR)
	    expr = new Node(INT, expr);

    } else
	expr = expression();

    return expr;
}


/*
 * Function:	argumentList
 *
 * Description:	Parse the argument list of a function call.
 *
 *		ArgumentList:
 *		  Argument
 *		  Argument , ArgumentList
 */

static void argumentList(Node *expr)
{
    expr->append(argument());

    while (word == ',') {
	match(',');
	expr->append(argument());
    }
}


/*
 * Function:	primaryExpression
 *
 * Description:	Parse a primary expression, which will always have a
 *		scalar type.
 *
 *		PrimaryExpression:
 *		  ( Expression )
 *		  charlit
 *		  num
 *		  name
 *		  name ( )
 *		  name ( ExpressionList )
 *		  name [ Expression ]
 */

static Node *primaryExpression()
{
    string name;
    unsigned size;
    Node *expr, *left, *right;
    Symbol *symbol;


    if (word == '(') {
	match('(');
	expr = expression();
	match(')');

    } else if (word == CHARLIT) {
	expr = new Node(NUM, makeLiteral(parseString(lexeme)[1]));
	match(CHARLIT);

    } else if (word == NUM) {
	expr = new Node(NUM, makeLiteral(lexeme));
	match(NUM);

    } else {
	name = lexeme;
	match(NAME);

	if (word == '(') {
	    expr = new Node(FUNC, new Node(NAME, lookupFunction(name)));
	    match('(');

	    if (word != ')')
		argumentList(expr);

	    checkCall(expr);
	    match(')');

	} else if (word == '[') {
	    left = new Node(NAME, lookupArray(name));
	    match('[');
	    right = expression();
	    match(']');

	    size = Type(left->symbol()->type().specifier()).size();

	    if (size != 1) {
		symbol = makeLiteral(size);
		right = new Node('*', right, new Node(NUM, symbol));
	    }

	    expr = new Node(INDEX, left, right);

	} else {
	    expr = new Node(NAME, lookupScalar(name));

	    if (expr->type().specifier() == CHAR)
		expr = new Node(INT, expr);
	}
    }

    return expr;
}


/*
 * Function:	unaryExpression
 *
 * Description:	Parse a unary expression.
 *
 *		UnaryExpression:
 *		  - UnaryExpression
 *		  ! UnaryExpression
 *		  PrimaryExpression
 */

static Node *unaryExpression()
{
    Node *expr;


    if (word == '-') {
	match('-');
	expr = unaryExpression();
	expr = new Node(NEGATE, expr);

    } else if (word == '!') {
	match('!');
	expr = unaryExpression();
	expr = new Node('!', expr);

    } else
	expr = primaryExpression();

    return expr;
}


/*
 * Function:	multiplicativeExpression
 *
 * Description:	Parse a multiplicative expression.
 *
 *		MultiplicativeExpression:
 *		  UnaryExpression
 *		  MultiplicativeExpression * UnaryExpression
 *		  MultiplicativeExpression / UnaryExpression
 *		  MultiplicativeExpression % UnaryExpression
 */

static Node *multiplicativeExpression()
{
    Node *left, *right;


    left = unaryExpression();

    while (1) {
	if (word == '*') {
	    match('*');
	    right = unaryExpression();
	    left = new Node('*', left, right);

	} else if (word == '/') {
	    match('/');
	    right = unaryExpression();
	    left = new Node('/', left, right);

	} else if (word == '%') {
	    match('%');
	    right = unaryExpression();
	    left = new Node('%', left, right);

	} else
	    break;
    }

    return left;
}


/*
 * Function:	additiveExpression
 *
 * Description:	Parse an additive expression.
 *
 *		AdditiveExpression:
 *		  MultiplicativeExpression
 *		  AdditiveExpression + MultiplicativeExpression
 *		  AdditiveExpression - MultiplicativeExpression
 */

static Node *additiveExpression()
{
    Node *left, *right;


    left = multiplicativeExpression();

    while (1) {
	if (word == '+') {
	    match('+');
	    right = multiplicativeExpression();
	    left = new Node('+', left, right);

	} else if (word == '-') {
	    match('-');
	    right = multiplicativeExpression();
	    left = new Node('-', left, right);

	} else
	    break;
    }

    return left;
}


/*
 * Function:	relationalExpression
 *
 * Description:	Parse a relational expression.
 *
 *		RelationalExpression:
 *		  AdditiveExpression
 *		  RelationalExpression < AdditiveExpression
 *		  RelationalExpression > AdditiveExpression
 *		  RelationalExpression <= AdditiveExpression
 *		  RelationalExpression >= AdditiveExpression
 */

static Node *relationalExpression()
{
    Node *left, *right;


    left = additiveExpression();

    while (1) {
	if (word == '<') {
	    match('<');
	    right = additiveExpression();
	    left = new Node('<', left, right);

	} else if (word == '>') {
	    match('>');
	    right = additiveExpression();
	    left = new Node('>', left, right);

	} else if (word == LEQ) {
	    match(LEQ);
	    right = additiveExpression();
	    left = new Node(LEQ, left, right);

	} else if (word == GEQ) {
	    match(GEQ);
	    right = additiveExpression();
	    left = new Node(GEQ, left, right);

	} else
	    break;
    }

    return left;
}


/*
 * Function:	equalityExpression
 *
 * Description:	Parse an equality expression.
 *
 *		EqualityExpression:
 *		  RelationalExpression
 *		  EqualityExpression == RelationalExpression
 *		  EqualityExpression != RelationalExpression
 */

static Node *equalityExpression()
{
    Node *left, *right;


    left = relationalExpression();

    while (1) {
	if (word == EQL) {
	    match(EQL);
	    right = relationalExpression();
	    left = new Node(EQL, left, right);

	} else if (word == NEQ) {
	    match(NEQ);
	    right = relationalExpression();
	    left = new Node(NEQ, left, right);

	} else
	    break;
    }

    return left;
}


/*
 * Function:	andExpression
 *
 * Description:	Parse a logical-and expression.
 *
 *		AndExpression:
 *		  EqualityExpression
 *		  AndExpression && EqualityExpression
 */

static Node *andExpression()
{
    Node *left, *right;


    left = equalityExpression();

    while (word == AND) {
	match(AND);
	right = equalityExpression();
	left = new Node(AND, left, right);
    }

    return left;
}


/*
 * Function:	expression
 *
 * Description:	Parse an expression, or more specifically a logical-or
 *		expression, since Tiny C does not allow comma or
 *		assignment as an expression operator.
 *
 *		Expression:
 *		  AndExpression
 *		  Expression || AndExpression
 */

static Node *expression()
{
    Node *left, *right;


    left = andExpression();

    while (word == OR) {
	match(OR);
	right = andExpression();
	left = new Node(OR, left, right);
    }

    return left;
}


/*
 * Function:	assignment
 *
 * Description:	Parse an assignment statement, or more specifically a
 *		simple expression statement that has a side effect,
 *		which includes assignments and function calls.
 *
 *		Assignment:
 *		  name = Expression
 *		  name [ Expression ] = Expression
 *		  name ( ExpressionList )
 *		  name ( )
 */

static Node *assignment()
{
    string name;
    unsigned size;
    Node *expr, *left, *right;
    Symbol *symbol;


    name = lexeme;
    match(NAME);

    if (word == '=') {
	left = new Node(NAME, lookupScalar(name));
	match('=');
	right = expression();
	expr = new Node('=', left, right);

    } else if (word == '[') {
	left = new Node(NAME, lookupArray(name));
	match('[');
	right = expression();
	match(']');

	size = Type(left->symbol()->type().specifier()).size();

	if (size != 1) {
	    symbol = makeLiteral(size);
	    right = new Node('*', right, new Node(NUM, symbol));
	}

	left = new Node(INDEX, left, right);

	match('=');
	right = expression();
	expr = new Node('=', left, right);

    } else if (word == '(') {
	expr = new Node(PROC, new Node(NAME, lookupFunction(name)));
	match('(');

	if (word != ')')
	    argumentList(expr);

	checkCall(expr);
	match(')');

    } else {
	expr = nullptr;
	error();
    }

    return expr;
}


/*
 * Function:	statements
 *
 * Description:	Parse a possibly empty sequence of statements.  Rather than
 *		checking if the next token starts a statement, we check if
 *		the next token ends the sequence, since a sequence of
 *		statements is always terminated by a closing brace.
 *
 *		Statements:
 *		  empty
 *		  Statement Statements
 */

static Node *statements()
{
    Node *stmt;


    stmt = new Node(BLOCK);

    while (word != '}')
	stmt->append(statement());

    return stmt;
}


/*
 * Function:	statement
 *
 * Description:	Parse a statement.  Note that Tiny C has so few
 *		statements that we handle them all in this one function.
 *
 *		Statement:
 *		  { Statements }
 *		  if ( Expression ) Statement
 *		  if ( Expression ) Statement else Statement
 *		  for ( Assignment ; Expression ; Assignment ) Statement
 *		  while ( Expression ) Statement
 *		  do Statement while ( Expression ) ;
 *		  return Expression ;
 *		  Assignment ;
 */

static Node *statement()
{
    Node *stmt;


    if (word == '{') {
	match('{');
	stmt = statements();
	match('}');

    } else if (word == IF) {
	stmt = new Node(IF);

	match(IF);
	match('(');
	stmt->append(expression());
	match(')');
	stmt->append(statement());

	if (word == ELSE) {
	    match(ELSE);
	    stmt->append(statement());
	}

    } else if (word == FOR) {
	stmt = new Node(FOR);

	match(FOR);
	match('(');
	stmt->append(assignment());
	match(';');
	stmt->append(expression());
	match(';');
	stmt->append(assignment());
	match(')');
	stmt->append(statement());

    } else if (word == WHILE) {
	stmt = new Node(WHILE);

	match(WHILE);
	match('(');
	stmt->append(expression());
	match(')');
	stmt->append(statement());

    } else if (word == DO) {
	stmt = new Node(DO);

	match(DO);
	stmt->append(statement());
	match(WHILE);
	match('(');
	stmt->append(expression());
	match(')');
	match(';');

    } else if (word == RETURN) {
	stmt = new Node(RETURN);

	match(RETURN);
	stmt->append(expression());
	match(';');

    } else {
	stmt = assignment();
	match(';');
    }

    return stmt;
}


/*
 * Function:	specifier
 *
 * Description:	Parse a type specifier, which in Tiny C is char or int.
 */

static int specifier()
{
    if (word == INT) {
	match(INT);
	return INT;
    }

    match(CHAR);
    return CHAR;
}


/*
 * Function:	parameter
 *
 * Description:	Parse a parameter, which in Tiny C is either a scalar
 *		or an array in which the size is not specified.
 *
 *		Parameter:
 *		  Specifier name
 *		  Specifier name [ ]
 */

static void parameter(Types *formals)
{
    int typespec;
    string name;

    typespec = specifier();
    name = lexeme;
    match(NAME);

    if (word == '[') {
	insertName(name, Type(typespec, 0U));
	formals->push_back(Type(typespec, 0U));
	match('[');
	match(']');

    } else {
	insertName(name, Type(typespec));
	formals->push_back(Type(typespec));
    }
}


/*
 * Function:	parameterList
 *
 * Description:	Parse a non-empty parameter list.
 *
 *		ParameterList:
 *		   Parameter
 *		   Parameter , ParameterList
 */

static void parameterList(Types *formals)
{
    parameter(formals);

    while (word == ',') {
	match(',');
	parameter(formals);
    }
}


/*
 * Function:	parameters
 *
 * Description:	Parse the parameters of a function, but not the
 *		opening or closing parentheses.
 *
 *		Parameters:
 *		  void
 *		  ParameterList
 */

static void parameters(Types *formals)
{
    if (word == VOID)
	match(VOID);
    else
	parameterList(formals);
}


/*
 * Function:	declarator
 *
 * Description:	Parse a declarator, which in Tiny C is either a scalar
 *		or a single-dimensional array.
 *
 *		Declarator:
 *		  name
 *		  name [ num ]
 */

static void declarator(int typespec)
{
    string name;
    unsigned length;


    name = lexeme;
    match(NAME);

    if (word == '[') {
	match('[');
	length = (word == NUM ? stoul(lexeme) : 1);
	checkArray(insertName(name, Type(typespec, length)));
	match(NUM);
	match(']');

    } else
	insertName(name, Type(typespec));
}


/*
 * Function:	moreDeclarators
 *
 * Description:	Parse any remaining declarators in a declaration after
 *		the first declarator.
 *
 *		MoreDeclarators:
 *		  empty
 *		  , Declararor MoreDeclarators
 */

static void moreDeclarators(int typespec)
{
    while (word == ',') {
	match(',');
	declarator(typespec);
    }
}


/*
 * Function:	declaration
 *
 * Description:	Parse a single declaration.
 *
 *		Declaration:
 *		  Specifier Declarator MoreDeclarators ;
 */

static void declaration()
{
    int typespec;


    typespec = specifier();
    declarator(typespec);
    moreDeclarators(typespec);
    match(';');
}


/*
 * Function:	declarations
 *
 * Description:	Parse a possible empty sequence of declarations.
 *
 *		Declarations:
 *		  empty
 *		  Declaration Declarations
 */

static void declarations()
{
    while (word == CHAR || word == INT)
	declaration();
}


/*
 * Function:	globalDeclaration
 *
 * Description:	Parse a global (i.e., top-level) declaration, which is
 *		either a variable declaration or a function definition.
 *
 *		GlobalDeclaration:
 *		  Specifier name MoreDeclarators ;
 *		  Specifier name [ num ] MoreDeclarators ;
 *		  Specifier name ( Parameters ) { Declarations Statements }
 */

static void globalDeclaration()
{
    unsigned length;
    int typespec;
    string name;
    Types *formals;
    Function function;
    
    
    typespec = specifier();
    name = lexeme;
    match(NAME);

    if (word == '[') {
	match('[');
	length = (word == NUM ? stoul(lexeme) : 1);
	checkArray(insertName(name, Type(typespec, length)));
	match(NUM);
	match(']');
	moreDeclarators(typespec);
	match(';');

    } else if (word == '(') {
	formals = new Types;
	function.symbol = insertName(name, Type(typespec, formals));
	function.locals = initializeScope();
	match('(');
	parameters(formals);
	match(')');
	match('{');
	declarations();
	function.body = statements();
	match('}');
	finalizeScope();

	if (numerrors == 0) {
	    optimizeTree(function);

	    if (output_format == OUTPUT_AST)
		cout << function.body << endl;
	    else {
		function.stmts = translate(function.body);
		optimizeStatements(function);

		if (output_format == OUTPUT_TAC)
		    cout << function.stmts << endl;
		else if (output_format == OUTPUT_ASM)
		    generateFunction(function);
	    }

	    functions[function.symbol] = function;
	}

    } else {
	insertName(name, Type(typespec));
	moreDeclarators(typespec);
	match(';');
    }
}


/*
 * Function:	translationUnit
 *
 * Description:	Parse the current translation unit (i.e., file), which
 *		consists of a possibly empty sequence of global
 *		declarations.
 *
 *		TranslationUnit:
 *		  empty
 *		  GlobalDeclaration TranslationUnit
 */

static void translationUnit()
{
    initializeScope();

    while (word != DONE)
	globalDeclaration();

    if (output_format == OUTPUT_ASM)
	generateGlobals(finalizeScope());
}


/*
 * Function:	usage
 *
 * Description:	Print a usage message to standard error and exit.
 */

static void usage()
{
    cerr << "usage: tcc [-A|-S|-T] [file]" << endl;
    exit(EXIT_FAILURE);
}


/*
 * Function:	main
 *
 * Description:	Analyze the standard input stream.
 */

int main(int argc, char *argv[])
{
    int c;
    output_format = OUTPUT_ASM;
	/*
	struct option {
    	const char *name;
    	int         has_arg;
    	int        *flag;
    	int         val;
	};  */
	//int optional_argument = 1;
	struct option long_opt[] = {
		{"dce", optional_argument, NULL, 'D'},
		{"cprop", optional_argument, NULL, 'C'},
		{"lvn", optional_argument, NULL, 'L'},
		{"asimp", optional_argument, NULL, 'X'},
		{"cfold", optional_argument, NULL, 'Z'},
		{NULL, 0, NULL, 0}
	};
	bool opta = false;
    while ((c = getopt_long(argc, argv, "AOSTDCLXZ", long_opt, NULL)) != -1)
    //while ((c = getopt(argc, argv, "AOSTDCLXZ")) != -1)
	switch (c) {
	    case 'A':
		output_format = OUTPUT_AST;
		break;


	    case 'S':
		output_format = OUTPUT_ASM;
		break;


	    case 'T':
		output_format = OUTPUT_TAC;
		break;


	    case 'O':
		/* ignored for now */
		break;
		
		case 'D':
		dce_on = 1;
		opta = 1;

		case 'C':
		cprop_on = 1;
		opta = 1;

		case 'L':
		lvn_on = 1;
		opta = 1;

		case 'X':
		asimp_on = 1;
		opta = 1;

		case 'Z':
		cfold_on = 1;
		opta = 1;
	    
		default:
		if(!opta)
		usage();
	}
	/*
	struct option long_opt[] = {
		{"dce", optional_argument, NULL, 'D'},
		{"cprop", optional_argument, NULL, 'C'},
		{"lvn", optional_argument, NULL, 'L'},
		{"asimp", optional_argument, NULL, 'X'},
		{"cfold", optional_argument, NULL, 'Z'},
		{NULL, 0, NULL, 0}
	};*/
    argc -= optind;
    argv += optind;

    if (argc > 1)
	usage();

    if (argc == 1)
	if (freopen(argv[0], "r", stdin) == NULL) {
	    perror(argv[0]);
	    exit(errno);
	}

    word = nextWord();
    translationUnit();
}
