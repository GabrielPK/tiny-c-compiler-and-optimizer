/*
 * File:	Block.h
 *
 * Description:	This file contains the class definition for basic blocks.
 *		Each basic block has links to its predecessors and
 *		successors in the control-flow graph, as well as iterators
 *		to its first and last statements.  Each block also has a
 *		link to the next block sequentially, for easily traversing
 *		all the blocks in a sequence.
 *
 *		For iterating through the block, forward and reverse
 *		iterators are supported.  These iterators traverse the
 *		non-label statements in the block, which allows us to
 *		process the actual statements without worrying about the
 *		labels.
 */

# ifndef BLOCK_H
# define BLOCK_H
# include <vector>
# include <ostream>
# include "Statement.h"
# include <unordered_set>

typedef std::vector<Block *> Blocks;

struct Block {
    Block *_next;
    Blocks _predecessors, _successors;
    Statements::iterator _first, _last;

	std::unordered_set<Symbol *> _UEVar;
	std::unordered_set<Symbol *> _VarKill;
	std::unordered_set<Symbol *> _LiveOut;
	expr_set ExprKill;
	expr_set DEExprs;
	expr_set AvailIn;
    Block();
    void link(Block *successor);
    void unlink(Block *successor);
	
    Block *next() const { return _next; }
    const Blocks &predecessors() const { return _predecessors; }
    const Blocks &successors() const { return _successors; }

    Statements::iterator first() const { return _first; }
    Statements::iterator last() const { return _last; }

    Statements::iterator begin() const, end() const;
    Statements::reverse_iterator rbegin() const, rend() const;
	
};

std::ostream &operator <<(std::ostream &ostr, const Block *block);

# endif /* BLOCK_H */
