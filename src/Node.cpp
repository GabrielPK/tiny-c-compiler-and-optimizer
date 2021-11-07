/*
 * File:	Node.cpp
 *
 * Description:	This file contains the member function definitions for AST
 *		nodes in Tiny C.  All nodes include a token and a vector of
 *		children, which may be empty.
 */

# include "Node.h"

using namespace std;


/*
 * Function:	Node::Node (constructor)
 *
 * Description:	Initialize this node with the specified token.  The node
 *		has no symbol and no children.
 */

Node::Node(int token)
    : _token(token), _symbol(nullptr)
{
}


/*
 * Function:	Node::Node (constructor)
 *
 * Description:	Initialize this node with the specified token and symbol.
 *		The node has no children.
 */

Node::Node(int token, Symbol *symbol)
    : _token(token), _symbol(symbol)
{
}


/*
 * Function:	Node::Node (constructor)
 *
 * Description:	Initialize this node with the specified token and children.
 *		The node has no symbol.
 */

Node::Node(int token, Node *left, Node *right)
    : _token(token), _symbol(nullptr)
{
    _kids.push_back(left);

    if (right != nullptr)
	_kids.push_back(right);
}


/*
 * Function:	Node::append
 *
 * Description:	Appends the given node to the vector of children.
 */

void Node::append(Node *child)
{
    _kids.push_back(child);
}


/*
 * Function:	Node::type
 *
 * Description:	Return the type of this node.  If the node represents an
 *		array index or function call, then the type is a scalar
 *		with the same specifier as that of the symbol.  Otherwise,
 *		if the node has a symbol, then its type is returned.
 *		Otherwise, int is simply returned.
 */

Type Node::type() const
{
    if (_token == INDEX || _token == FUNC || _token == PROC)
	return Type(_kids[0]->_symbol->type().specifier());

    return _symbol != nullptr ? _symbol->type() : Type(INT);
}


/*
 * Function:	operator <<
 *
 * Description:	Write this node to the specified output stream.
 */

ostream &operator <<(ostream &ostr, const Node *node)
{
    if (node->_symbol == nullptr) {
	ostr << "(" << lexemes[node->_token];

	for (auto kid : node->_kids)
	    ostr << " " << kid;

	ostr << ")";

    } else
	ostr << node->_symbol->name();

    return ostr;
}
