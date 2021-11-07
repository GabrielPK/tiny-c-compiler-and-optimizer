/*
 * File:	translator.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for translating the AST into
 *		three-address code.
 */

# include <cassert>
# include <unordered_map>
# include "translator.h"
# include "literal.h"

# define emit(x)	(stmts.push_back(x))

using namespace std;

static unsigned numtemps;
static Symbol *zero, *one;
static void generate(Statements &stmts, Node *node);

static unordered_map<int,int> inverses = {
    {EQL, NEQ}, {NEQ, EQL}, {'<', GEQ}, {'>', LEQ}, {LEQ, '>'}, {GEQ, '<'},
};


/*
 * Function:	newTemp (private)
 *
 * Description:	Create a new temporary and assign it to the given node.
 */

static Symbol *newTemp(Node *node)
{
    string name = "t" + to_string(numtemps ++);
    node->_symbol = new Symbol(name, Type(INT), TEMP);
    return node->_symbol;
}


/*
 * Function:	test (private)
 *
 * Description:	Generate code for a node and test its value.  For the
 *		traditional boolean operators that will require a compare
 *		instruction, it is faster and cleaner to generate a branch
 *		statement to the specified target statement.  For the
 *		traditional arithmetic operators, we simply compare the
 *		result with zero and branch.
 */

static void test(Statements &stmts, Node *node, Label *target, bool ifTrue)
{
    int token;
    Label *skip;
    Symbol *left, *right;


    switch(node->_token) {
    case '!':
	test(stmts, node->_kids[0], target, !ifTrue);
	break;


    case EQL: case NEQ: case '<': case '>': case LEQ: case GEQ:
	generate(stmts, node->_kids[0]);
	generate(stmts, node->_kids[1]);

	left = node->_kids[0]->_symbol;
	right = node->_kids[1]->_symbol;

	token = (ifTrue ? node->_token : inverses[node->_token]);
	emit(new Branch(token, left, right, target));
	break;


    case AND:
	skip = (ifTrue ? new Label() : target);

	test(stmts, node->_kids[0], skip, false);
	test(stmts, node->_kids[1], target, ifTrue);

	if (ifTrue)
	    emit(skip);

	break;


    case OR:
	skip = (ifTrue ? target : new Label());

	test(stmts, node->_kids[0], skip, true);
	test(stmts, node->_kids[1], target, ifTrue);

	if (!ifTrue)
	    emit(skip);

	break;


    default:
	generate(stmts, node);
	token = (ifTrue ? NEQ : EQL);
	emit(new Branch(token, node->_symbol, zero, target));
	break;
    }
}


/*
 * Function:	generate (private)
 *
 * Description:	Generate three-address code for the specified node,
 *		appending the resulting statements to the given list.
 */

static void generate(Statements &stmts, Node *node)
{
    Symbol *left, *right, *result;
    Label *loop, *exit, *skip, *target;
    Symbols args;


    switch(node->_token) {
    case BLOCK:
	for (unsigned i = 0; i < node->_kids.size(); i ++)
	    generate(stmts, node->_kids[i]);

	break;


    case IF:
	skip = new Label();
	test(stmts, node->_kids[0], skip, false);
	generate(stmts, node->_kids[1]);

	if (node->_kids.size() == 3) {
	    Label *elseLabel = new Label();

	    emit(new Jump(elseLabel));
	    emit(skip);

	    generate(stmts, node->_kids[2]);
	    skip = elseLabel;
	}

	emit(skip);
	break;


    case FOR:
	loop = new Label();
	exit = new Label();

	generate(stmts, node->_kids[0]);
	emit(loop);
	test(stmts, node->_kids[1], exit, false);
	generate(stmts, node->_kids[3]);
	generate(stmts, node->_kids[2]);
	emit(new Jump(loop));
	emit(exit);

	/*
	generate(stmts, node->_kids[0]);
	test(stmts, node->_kids[1], exit, false);
	emit(loop);
	generate(stmts, node->_kids[3]);
	generate(stmts, node->_kids[2]);
	test(stmts, node->_kids[1], loop, true);
	emit(exit);
	*/
	break;


    case WHILE:
	loop = new Label();
	exit = new Label();

	emit(loop);
	test(stmts, node->_kids[0], exit, false);
	generate(stmts, node->_kids[1]);
	emit(new Jump(loop));
	emit(exit);

	/*
	test(stmts, node->_kids[0], exit, false);
	emit(loop);
	generate(stmts, node->_kids[1]);
	test(stmts, node->_kids[0], loop, true);
	emit(exit);
	*/
	break;


    case DO:
	loop = new Label();
	emit(loop);
	generate(stmts, node->_kids[0]);
	test(stmts, node->_kids[1], loop, true);
	break;


    case RETURN:
	generate(stmts, node->_kids[0]);
	emit(new Return(node->_kids[0]->_symbol));
	emit(new Label());
	break;


    case '=':
	generate(stmts, node->_kids[1]);

	if (node->_kids[0]->_token == INDEX) {
	    generate(stmts, node->_kids[0]->_kids[1]);

	    left = node->_kids[0]->_kids[0]->_symbol;
	    right = node->_kids[0]->_kids[1]->_symbol;
	    emit(new Update(left, right, node->_kids[1]->_symbol));

	} else {
	    left = node->_kids[0]->_symbol;
	    right = node->_kids[1]->_symbol;

# if 0
	    Binary *b = dynamic_cast<Binary *>(stmts->back());

	    if (b && b->_result == right) {
		b->_result = left;
		break;
	    }
# endif

	    emit(new Copy(left, right));
	}

	break;


    case INDEX:
	generate(stmts, node->_kids[0]);
	generate(stmts, node->_kids[1]);

	left = node->_kids[0]->_symbol;
	right = node->_kids[1]->_symbol;
	result = newTemp(node);

	emit(new Index(result, left, right));
	break;


    case FUNC:
    case PROC:
	for (unsigned i = 1; i < node->_kids.size(); i ++) {
	    generate(stmts, node->_kids[i]);
	    args.push_back(node->_kids[i]->_symbol);
	}

	result = (node->_token == FUNC ? newTemp(node) : nullptr);
	emit(new Call(result, node->_kids[0]->_symbol, args));
	break;


    case '!':
	generate(stmts, node->_kids[0]);
	left = node->_kids[0]->_symbol;
	result = newTemp(node);
	emit(new Binary(EQL, result, left, zero));
	break;


    case NEGATE: case INT:
	generate(stmts, node->_kids[0]);
	right = node->_kids[0]->_symbol;

	result = newTemp(node);
	emit(new Unary(node->_token, result, right));
	break;


    case EQL: case NEQ: case '<': case '>': case LEQ: case GEQ:
    case '+': case '-': case '*': case '/': case '%':
	generate(stmts, node->_kids[0]);
	generate(stmts, node->_kids[1]);

	left = node->_kids[0]->_symbol;
	right = node->_kids[1]->_symbol;
	result = newTemp(node);

	emit(new Binary(node->_token, result, left, right));
	break;


    case AND:
	target = new Label();
	skip = new Label();

	test(stmts, node->_kids[0], target, false);
	test(stmts, node->_kids[1], target, false);

	result = newTemp(node);
	result->kind(LOCAL);
	emit(new Copy(result, one));
	emit(new Jump(skip));

	emit(target);
	emit(new Copy(result, zero));
	emit(skip);
	break;


    case OR:
	target = new Label();
	skip = new Label();

	test(stmts, node->_kids[0], target, true);
	test(stmts, node->_kids[1], target, true);

	result = newTemp(node);
	result->kind(LOCAL);
	emit(new Copy(result, zero));
	emit(new Jump(skip));

	emit(target);
	emit(new Copy(result, one));
	emit(skip);
	break;


    case NAME: case NUM: case STRLIT: case CHARLIT:
	break;


    default:
	assert(0);
    }
}


/*
 * Function:	translate
 *
 * Description:	Translate the given node into three-address code and return
 *		the list of statements.
 */

Statements translate(Node *node)
{
    Statements stmts;


    numtemps = 0;
    zero = makeLiteral(0);
    one = makeLiteral(1);

    emit(new Label());
    emit(new Null());
    emit(new Label());
    generate(stmts, node);
    emit(new Label());

    return stmts;
}
