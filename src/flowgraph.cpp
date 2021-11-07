/*
 * File:	flowgraph.cpp
 *
 * Description:	This file contains the function definitions for
 *		constructing and optimizing the control-flow graph.
 */

# include <cassert>
# include <unordered_map>
# include <unordered_set>
# include "flowgraph.h"

using namespace std;


/*
 * Function:	deleteBasicBlocks (private)
 *
 * Description:	Delete all the basic blocks in the flowgraph.
 */

static void deleteBasicBlocks(Statements &stmts)
{
    Label *label;


    for (auto stmt : stmts)
	if ((label = stmt->asLabel()) != nullptr)
	    if (label->_block != nullptr) {
		delete label->_block;
		label->_block = nullptr;
	    }
}


/*
 * Function:	mergeAdjacentLabels (private)
 *
 * Description:	Merge a sequence of adjacent labels into a single label,
 *		and rewrite any references to deleted labels to the first
 *		label in the sequence.  We first insert a label after any
 *		branch or jump statement.  The idea is that each basic
 *		block starts and ends with a label.  If statements inside
 *		the block are added or deleted, the labels, and thus the
 *		block, always remain.
 */

static void mergeAdjacentLabels(Statements &stmts)
{
    Label *first, *label;
    unordered_map<Label *, Label *> merged;


    /* Insert a label after each branch, jump, or return statement. */

    for (auto it = stmts.begin(); it != stmts.end(); it ++)
	if ((*it)->target() != nullptr || !(*it)->fallsThru())
	    if (!(*next(it))->asLabel())
		stmts.insert(next(it), new Label());


    /* Upon finding a label, delete all other labels in the sequence. */

    for (auto it = stmts.begin(); it != stmts.end(); it ++)
	if ((first = (*it)->asLabel()) != nullptr) {
	    it ++;

	    while (it != stmts.end() && (label = (*it)->asLabel())) {
		delete label;
		merged[label] = first;
		it = stmts.erase(it);
	    }

	    it --;
	}


    /* Rewrite each branch or jump statement with its new label. */

    for (auto stmt : stmts)
	if (merged.count(stmt->target()) > 0)
	    stmt->target(merged[stmt->target()]);
}


/*
 * Function:	eliminateMultipleJumps (private)
 *
 * Description:	Eliminate jumps to labeled statements that then jump to
 *		another label by replacing the first jump with a jump to
 *		the final label.
 */

static bool eliminateMultipleJumps(Statements &stmts)
{
    bool changed;
    Statement *nextstmt, *prevstmt;
    unordered_map<Label *, Label *> forward;


    changed = false;

    for (auto it = stmts.begin(); it != stmts.end(); it ++)
	if ((*it)->asLabel() && next(it) != stmts.end()) {
	    nextstmt = *next(it);

	    if (!nextstmt->fallsThru() && nextstmt->target()) {
		if (nextstmt->target() != (*it)->asLabel()) {
		    forward[(*it)->asLabel()] = nextstmt->target();

		    if (it != stmts.begin()) {
			prevstmt = *prev(it);

			if (!prevstmt->fallsThru()) {
			    delete *it;
			    delete nextstmt;
			    changed = true;
			    stmts.erase(next(it));
			    it = stmts.erase(it);
			    it --;
			}
		    }
		}
	    }
	}

    for (auto stmt : stmts)
	while (forward.count(stmt->target()) > 0)
	    stmt->target(forward[stmt->target()]);


    /* Eliminate jumps or branches to the very next statement. */

    auto it = stmts.begin();

    while (it != stmts.end())
	if ((*it)->target() == *next(it)) {
	    it = stmts.erase(it);
	    changed = true;
	} else
	    it ++;

    return changed;
}


/*
 * Function:	removeUneededLabels (private)
 *
 * Description:	Removes any unneeded labels from the list of statements.  A
 *		label is needed if it is immediately after a branch
 *		statement, a jump statement, or a null statement, or is the
 *		target of a branch or jump statement.
 */

bool removeUnneededLabels(Statements &stmts)
{
    bool changed;
    Label *this_label;
    Statement *prev_stmt;
    unordered_set<Label *> needed;


    if (stmts.size() < 2)
	return false;

    for (auto it = next(stmts.begin()); it != prev(stmts.end()); it ++)
	if ((this_label = (*it)->asLabel()) != nullptr) {
	    prev_stmt = *prev(it);

	    if (dynamic_cast<Null *>(prev_stmt))
		needed.insert(this_label);
	    else if (prev_stmt->target() || !prev_stmt->fallsThru())
		needed.insert(this_label);

	} else if ((*it)->target() != nullptr)
	    needed.insert((*it)->target());

    changed = false;
    auto it = next(stmts.begin());

    while (it != prev(stmts.end())) {
	this_label = (*it)->asLabel();

	if (this_label != nullptr && needed.count(this_label) == 0) {
	    it = stmts.erase(it);
	    changed = true;
	} else
	    it ++;
    }

    return changed;
}


/*
 * Function:	buildBasicBlocks (private)
 *
 * Description:	Construct basic blocks from the list of three-address
 *		statements.  A basic block is delimited by a label at each
 *		end (a leader and a trailer).  The leader has a reference
 *		to its block.  The basic blocks form the vertices of the
 *		control-flow graph.
 *
 *		label:        <----+ leader of block 1
 *		  statement        |
 *		  statement        | block 1
 *		  statement        |
 *		label:        <----+ trailer of block 1 / leader of block 2
 *		  statement        |
 *		  statement        | block
 *		  statement        |
 *		label:        <----+ trailer of block 2
 */

static void buildBasicBlocks(Function &function)
{
    Block *current, *next;
    Label *label, *target, *exit;
    Statement *prevstmt;


    current = nullptr;

    assert(function.stmts.size() > 0);
    function.exit = new Block();

    exit = function.stmts.back()->asLabel();
    exit->_block = function.exit;

    for (auto it = function.stmts.begin(); it != function.stmts.end(); it ++)
	if ((label = (*it)->asLabel()) != nullptr) {
	    if (label->_block == nullptr)
		label->_block = new Block();

	    next = label->_block;
	    next->_first = it;

	    if (it != function.stmts.begin()) {
		current->_last = it;
		prevstmt = *prev(it);
		target = prevstmt->target();

		if (prevstmt->fallsThru())
		    current->link(next);

		if (target != nullptr) {
		    if (target->_block == nullptr)
			target->_block = new Block();

		    current->link(target->_block);

		} else if (!prevstmt->fallsThru())
		    current->link(exit->_block);

	    } else
		function.entry = next;

	    if (current != nullptr)
		current->_next = next;

	    current = next;
	}

    current->_last = current->_first;
}


/*
 * Function:	rebuildFlowgraph
 *
 * Description:	Rebuild the control-flow graph.
 */

void rebuildFlowgraph(Function &function)
{
    bool changed;

    deleteBasicBlocks(function.stmts);

    do {
	changed = false;
	mergeAdjacentLabels(function.stmts);
	changed = changed | eliminateMultipleJumps(function.stmts);
	changed = changed | removeUnneededLabels(function.stmts);
    } while (changed);

    buildBasicBlocks(function);
}


/*
 * Function:	getBlocks
 *
 * Description:	Convenience function for returning a vector of blocks in
 *		the control-flow graph.
 */

Blocks getBlocks(Function &function)
{
    Block *block;
    Blocks blocks;


    assert(function.stmts.size() > 0);

    if (function.entry == nullptr)
	rebuildFlowgraph(function);

    for (block = function.entry; block != nullptr; block = block->next())
	blocks.push_back(block);

    return blocks;
}
