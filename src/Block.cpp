/*
 * File:	Block.cpp
 *
 * Description:	This function contains the member function definitions for
 *		basic blocks.
 *
 *		Forward iterators in C++ work as follows:
 *
 *		  begin() returns the first element
 *		  end() returns the element after the last element
 *
 *		Reverse iterators in C++ work as follows:
 *		
 *		  rbegin() returns the element after last element
 *		  rend() returns the first element
 *
 *		Since we want to iterate over the non-label statements in
 *		the block, we have the following:
 *
 *		  begin() returns next(block's first statement)
 *		  end() returns the block's last statement
 *
 *		  rbegin() returns the block's last statement
 *		  rend() returns next(block's first statement)
 *
 *		Remember that the first and last statements of a block are
 *		always label statements.
 */

# include "Block.h"
# include <algorithm>

using namespace std;


/*
 * Function:	Block::Block (constructor)
 *
 * Description:	Initialize this block.
 */

Block::Block()
    : _next(nullptr)
{/*
	_UEVar = unordered_set<Symbol *>();
	_VarKill = unordered_set<Symbol *>();
	_LiveOut = unordered_set<Symbol *>();
*/
}


/*
 * Function:	Block::link
 *
 * Description:	Link this block to the given successor.
 */

void Block::link(Block *successor)
{
    _successors.push_back(successor);
    successor->_predecessors.push_back(this);
}


/*
 * Function:	Block::unlink
 *
 * Description:	Unlink this block from the given successor.
 */

void Block::unlink(Block *successor)
{
    Blocks &preds = successor->_predecessors;
    _successors.erase(remove(_successors.begin(), _successors.end(), successor));
    preds.erase(remove(preds.begin(), preds.end(), this));
}


/*
 * Function:	Block::begin (forward iterator)
 *
 * Description:	Return a forward iterator to the first actual statement in
 *		this block.
 */

Statements::iterator Block::begin() const
{
    if (_first == _last)
	return end();

    return std::next(_first);
}


/*
 * Function:	Block::end (forward iterator)
 *
 * Description:	Return a forward iterator to the statement immediately
 *		after the last actual statement in this block.
 */

Statements::iterator Block::end() const
{
    return _last;
}


/*
 * Function:	Block::rbegin (reverse iterator)
 *
 * Description:	Return a reverse iterator to the last actual statement in
 *		this block.
 */

Statements::reverse_iterator Block::rbegin() const
{
    if (_first == _last)
	return rend();

    return reverse_iterator<Statements::iterator>(_last);
}


/*
 * Function:	Block::rend (reverse iterator)
 *
 * Description:	Return a reverse iterator to the statement immediately
 *		before the first actual statement in this block.
 */

Statements::reverse_iterator Block::rend() const
{
    return reverse_iterator<Statements::iterator>(std::next(_first));
}


/*
 * Function:	operator <<
 *
 * Description:	Write the block to the specified output stream.
 */

ostream &operator <<(ostream &ostr, const Block *block)
{
    return ostr << "B" << (*block->_first)->asLabel()->_number;
}
