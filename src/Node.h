/*
 * File:	Node.h
 *
 * Description:	This file contains the class definition for AST nodes in
 *		Tiny C.  All nodes contain a token and a vector of
 *		children, which may be empty.
 */

# ifndef NODE_H
# define NODE_H
# include <vector>
# include <ostream>
# include "tokens.h"
# include "Symbol.h"

typedef std::vector<struct Node *> Nodes;

struct Node {
    int _token;
    Nodes _kids;
    Symbol *_symbol;

    Node(int token);
    Node(int token, Symbol *symbol);
    Node(int token, Node *left, Node *right = nullptr);

    int token() const { return _token; }
    Symbol *symbol() const { return _symbol; }

    Node *kids(int i) const { return _kids[i]; }
    const Nodes &kids() const { return _kids; }

    void append(Node *child);
    Type type() const;
};

std::ostream &operator <<(std::ostream &ostr, const Node *node);

# endif /* NODE_H */
