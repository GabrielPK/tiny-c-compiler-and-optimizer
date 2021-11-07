/*
 * File:	optimizer.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for optimizing the input program.
 *
 *		Currently, these functions are empty.  However, they are
 *		called by the parser to optimize the AST and the
 *		three-address code.
 */


# include "optimizer.h"
# include <unordered_set>
# include <iostream>
# include "literal.h"
# include <algorithm>
# include <map>
# include "Type.h"
# include "lvn.h"
# include "dataflow.h"
# include "opflgs.h"

# define DCE     dce_on
# define ALGSIMP asimp_on
# define CF      cfold_on
# define LVN     lvn_on
# define CPROP	 cprop_on
# define CSE     0
/*
typedef struct LVA_sets {
    std::unordered_set<Symbol *> gen;
    std::unordered_set<Symbol *> kill;
} LVA_sets;
*/

using namespace std;



void initLVA(Block *b, Function &function);
void doLVA(Function &function);
bool doDCE(Function &function);
map<Block*, int> dce_marks;
int marked = 1;
void doDFS(Block *block);
bool doConstantFolding(Function &function);
bool doAlgSimp(Function &function);
bool doLVN(Function &function);
bool doCprop(Function &function);
Symbols lvn_globals;
bool doCSE(Function &function);
//std::map<Symbol*, int> sym_int;
//std::map<struct LVN_expr*, int> expr_int;
/*
struct LVN_expr {
	Symbol *left, *right;
	int op;
};
*/

template< class Iterator >
reverse_iterator<Iterator> make_reverse_iterator(Iterator i)
{
    return reverse_iterator<Iterator>(i);
}

void optimizeTree(Function &function)
{
	
}

void optimizeStatements(Function &function)
{
	rebuildFlowgraph(function);

	bool changed = true;
	while(changed) {
		changed = false;
		//cout << "# optimize\n";
		doLVA(function);
		//cout << "# did lva\n";
		if(DCE)
			if(doDCE(function)) {
				changed = true;
				//cout << "# dce changed\n";
				rebuildFlowgraph(function);
				//cout << "# dce rebuilt\n";
			}
		if(ALGSIMP)	
			if(doAlgSimp(function)) {
				changed = true;
				rebuildFlowgraph(function);
			}
		if(CF)
			if(doConstantFolding(function)) {
				changed = true;
				rebuildFlowgraph(function);
			}
		if(LVN)
			if(doLVN(function)) {
				changed = true;
				rebuildFlowgraph(function);
			}
		if(CPROP)
			if(doCprop(function)) {
				changed = true;
				rebuildFlowgraph(function);
			}
		if(CSE)
			if(doCSE(function)) {
				changed = true;
				rebuildFlowgraph(function);
			}
	}
}
expr_set uni;
struct cse_uni cse_universe;

void AvailExprsinit(Block *block, Function &function) {
	block->ExprKill.clear();
	block->DEExprs.clear();
	struct avail_expr gen_kill;
	struct avail_expr temp;
	for(auto it = function.stmts.begin(); it != function.stmts.end(); it++) {
		(*it)->gen_cse_uni(cse_universe);
	}
	for(auto it = block->begin(); it != block->end(); it++) {
		temp = (*it)->availExpr(cse_universe);
		for(auto &killed : temp.kill) {
			block->ExprKill.insert(killed);
			block->DEExprs.erase(killed);
		}
		for(auto &genned : temp.gen) {
			block->DEExprs.insert(genned);
		}
	}
}

void doAvailExprs(Function &function) {
	
	Blocks blocks = getBlocks(function);
	bool changed = true;
	while(changed) {
		changed = false;
		for(auto &block : blocks) {
			block->ExprKill.clear();
			block->DEExprs.clear();
			AvailExprsinit(block, function);
			block->AvailIn = cse_universe.uni;
			auto atemp = block->AvailIn;
			auto temp = cse_universe.uni;
			for(auto &pred : block->predecessors()) {
				auto avail_inp = pred->AvailIn;
				auto killp = pred->ExprKill;
				auto genp = pred->DEExprs;
			
				remove(avail_inp, killp);
				insert(genp, avail_inp);
				filter(temp, genp);
				block->AvailIn = temp;
			}
			changed = changed || atemp != block->AvailIn;
		}
	}
}


bool doCSE(Function &function) {
	cout << "# doCSE\n";
	bool changed = false;
	Blocks blocks = getBlocks(function);
	
	doAvailExprs(function);
	
	for(auto &block : blocks) {
		block->DEExprs.clear();
		block->ExprKill.clear();
		block->DEExprs = block->AvailIn;
	}
	
	avail_expr temp;
	
	
	return changed;
}

map<Block*,copy_set> DECopies;
map<Block*,copy_set> CopyKill;
map<Block*,copy_set> AvailIn;
copy_set universe;
	
void availCopiesInit(Block *block, Symbols &globals) {
	for(auto it = block->begin(); it != block->end(); it++) {
		(*it)->cp_gen_kill(DECopies[block], CopyKill[block], universe, globals);
	}	
}

void doAvailCopies(Function &function) {
	// initialize AvailIn for all to the universe, except for the entry block
	Blocks blocks = getBlocks(function);

	// create the universe
	for(auto stmt : function.stmts) {
		stmt->cp_uni(universe);
	}
	
	auto globals = 	function.locals->enclosing()->symbols();
	for(auto &block : blocks) {
		availCopiesInit(block, globals);
		AvailIn[block] = universe;
	}
	AvailIn[function.entry].clear();
	
				/*cout << "universe\n";
				for(auto &pair : universe) {
                    cout << pair.first->name() << ", " << pair.second->name() << endl;
	
				}  */
	bool changed = true;
	while(changed) {
		changed = false;
		for(auto &block : blocks) {
			auto temp = AvailIn[block];
			auto check = AvailIn[block];
			AvailIn[block].clear();
			copy_set outp; 
			for(auto &pred : block->predecessors()) {
				auto inp = AvailIn[pred];
				auto killp = CopyKill[pred];
				auto genp = DECopies[pred];
				remove(inp, killp);
				insert(genp, inp);
				outp = genp;
				/*cout << "outp\n";
				for(auto &pair : outp) {
                    cout << pair.first->name() << ", " << pair.second->name() << endl;
	
				}*/
				filter(temp, outp);
				/*cout << "temp\n";
				for(auto &pair : temp) {
                    cout << pair.first->name() << ", " << pair.second->name() << endl;
	
				}*/
			}
			AvailIn[block] = temp;
		/*cout << "# DECopies\n";
		for(auto &pair : DECopies[block]) {
			cout << pair.first->name() << ", " << pair.second->name() << endl;
		}
		cout << "# AvailIn\n";
		for(auto &pair : AvailIn[block]) {
			cout << pair.first->name() << ", " << pair.second->name() << endl;
		}*/
			changed = changed || check != AvailIn[block];
		}
	}
}

bool doCprop(Function &function) {
	//cout << "# doCprop\n";	
	bool changed = false;
	doAvailCopies(function);
	Blocks blocks = getBlocks(function);
	auto globals = 	function.locals->enclosing()->symbols();
	for(auto &block : blocks) {
		CopyKill[block].clear();
		DECopies[block].clear();
		insert(DECopies[block], AvailIn[block]);
		/*cout << "DECopies\n";
		for(auto &pair : DECopies[block]) {
			cout << pair.first->name() << ", " << pair.second->name() << endl;
		}
		cout << "AvailIn\n";
		for(auto &pair : AvailIn[block]) {
			cout << pair.first->name() << ", " << pair.second->name() << endl;
		} */
		for(auto it = block->begin(); it != block->end(); it++) {
			(*it)->cp_gen_kill(DECopies[block], CopyKill[block], universe, globals);
			changed = changed || (*it)->cprop(DECopies[block], CopyKill[block], AvailIn[block], universe);

			for(auto stmt : function.stmts) {
		        stmt->cp_uni(universe);
			}
		}

	}
	
	return changed;
}

bool doLVN(Function &function) {
	//cout << sym_int.size() << endl;
	//cout << expr_int.size() << endl;
	sym_int.clear();
	expr_int.clear();
	lvn_globals = function.locals->enclosing()->symbols();
	int val_num = 0;
	//cout << "# doLVN\n";
	bool changed = false;
	Statement *result;
	auto it = function.stmts.begin();
		
	while (it != function.stmts.end()) {
		//cout << "# while\n";
		result = (*it)->valnum(val_num);
		if (result == nullptr) {
			delete *it;
			//cout << "result == nullptr\n";
			it = function.stmts.erase(it);
			//changed = true;
		} else {
			if (result != *it) {
				delete *it;
				//cout << "result != *it\n";
				*it = result;
				changed = true;
			} 
			it++;
			//cout << "# it++\n";
		}
	}
	return changed;
}

void doLVA(Function &function) {
	//cout << "# doLVA\n";
	Blocks blocks = getBlocks(function);
	bool changed = true;

	for(auto &block : blocks) {
		initLVA(block, function);
		block->_LiveOut.clear(); 
	}	
	for(auto &glob : function.locals->enclosing()->symbols()) {
		if((glob->kind() == GLOBAL) && !((glob->type().isFunction()))){
			//cout << "#LiveOut.insert(glob) \n";
			function.exit->_LiveOut.insert(glob);
		}
	}

	//cout << "left for\n";
			
	// for each block in the function
	while(changed) {
		changed = false;
		blocks = getBlocks(function);
		//for(auto &block : blocks) {
		for(auto blocki = blocks.rbegin(); blocki != blocks.rend(); blocki++) {
			// for each succ
			auto block = *blocki;
			for(auto &succ : block->successors()) {
				// insert UEVar(succ)
				auto temp = block->_LiveOut;
				for(auto &uevar : succ->_UEVar) {
					block->_LiveOut.insert(uevar);
					//cout << "c1\n";
				}

				// insert LiveOut(succ) - VarKill(succ)
				for(auto &lk : succ->_LiveOut) {
					if(succ->_VarKill.count(lk) == 0) {
						block->_LiveOut.insert(lk);
						//cout << "c2\n";
					}
				}
				changed = temp != block->_LiveOut;
			}
		}
	} 
	/*
	int i = 0;
	for (auto &block : getBlocks(function)){
        cerr << "# block" << i << endl; i++;
        cerr << "# UEVAR" << endl;
        for(auto i : block->_UEVar)
            cerr << "\t" << i->name() << endl;
        cerr << "# Varkill" << endl;
        for(auto i : block->_VarKill)
            cerr << "\t" << i->name() << endl;
        cerr << "# LiveOut" << endl;
        for(auto i : block->_LiveOut)
            cerr << "\t" << i->name() << endl;
    } */ 
	

}

void initLVA(Block *block, Function &function) {
	//cout << "# initLVA\n";
	block->_UEVar.clear();
	block->_VarKill.clear();
	// for each statement in the block
	for(auto it = (block->rbegin()); it != (block->rend()); it++) {
		
		auto sets = (*it)->make_lva_sets();
		if((sets.kill != nullptr) && (1 || !isNumber(sets.kill))) {
			//cout << "# set kill: " << sets.kill->name() << endl;
			block->_VarKill.insert(sets.kill);
			block->_UEVar.erase(sets.kill);
		}

		// insert gen elements
		for(auto &genned : sets.gen) {
			if(!isNumber(genned))
				block->_UEVar.insert(genned);
		}

		if(sets.isFunc == 1){
			for(auto &glob : function.locals->enclosing()->symbols()) {
				if((glob->kind() == GLOBAL) && !((glob->type().isFunction()))){
					block->_UEVar.insert(glob);
				}
			}
		}
	}
}

bool doDCE(Function &function) {

	//cout << "# doDCE\n";
	Blocks blocks = getBlocks(function);
	bool changed = false;
	// eliminate useless code
	for(auto &block : blocks) {
		auto it = block->rbegin();
		block->_VarKill.clear();
		block->_UEVar = block->_LiveOut;
		// walk each block backwards
		while(it != block->rend()) {
			auto set = (*it)->make_lva_sets();
			
			/*
			if(set.isFunc == 1) {
				for(auto &glob : function.locals->enclosing()->symbols()) {
					if((glob->kind() == GLOBAL) && !((glob->type().isFunction()))){
						block->_UEVar.insert(glob);
					}
				}
			} */ 

			if((set.kill != nullptr) && (block->_UEVar.count(set.kill) == 0)) {
				delete *it;
				it = make_reverse_iterator(function.stmts.erase(find(function.stmts.begin(), function.stmts.end(),*it)));
				changed = true;
			} else {
				it++;
			}
	
			if((set.kill != nullptr) && (1 || !isNumber(set.kill))) {
				block->_VarKill.insert(set.kill);
				block->_UEVar.erase(set.kill);
			}

			// insert gen elements
			for(auto &genned : set.gen) {
				if(!isNumber(genned))
					block->_UEVar.insert(genned);
			}
		}
	}

	if(changed)
		rebuildFlowgraph(function);

	//changed = false;
	// eliminate unreachable code
	// mark all blocks as not found
	dce_marks.clear();
	blocks = getBlocks(function);
	for(auto &block : blocks) {
		//dce_marks.insert(std::pair<Block*,int>(block, 0));
		dce_marks[block] = 0;
	} 
	auto it = blocks.front();
	doDFS(it);
	//cout << "# DoDFS\n";
	for(auto &block : blocks) {
		if (dce_marks[block] == 0) {
			// delete block by deleting all stmts in block
			for(auto stit = block->begin(); stit != block->end();) {
				delete *stit;
				stit = function.stmts.erase(stit);
			}
			changed = true;
		//	cout << "#deleted stmt\n";
		}
	}
	//cout << "#ret changed\n";
	return changed;
}

// traverse each successor for this block and mark and marked
void doDFS(Block *node){
	if(dce_marks[node] == 1)
		return;
	dce_marks[node] = 1;
	for(auto &block : node->successors()) {
		if(block != nullptr) {
			doDFS(block);
		} else 
			return;
	}
}

bool doConstantFolding(Function &function) {
	//cout << "# doCF\n";
	bool changed = false;
    Statement *result;
    auto it = function.stmts.begin();
    while (it != function.stmts.end()) {
        result = (*it)->cfold();
        if (result == nullptr) {
            delete *it;
            it = function.stmts.erase(it);
            changed = true;
        } else {
            if (result != *it) {
                delete *it;
                //cout << "result != *it\n";
                *it = result;
                changed = true;
            }
            it++;
        }
    }


	return changed;
}

bool doAlgSimp(Function &function) {
	//cout << "# doAlgSimp\n";
	bool changed = false;
	Statement *result;
	auto it = function.stmts.begin();
	while (it != function.stmts.end()) {
		//changed = false;
		result = (*it)->simplify();
		if (result == nullptr) {
			delete *it;
			cout << "result == nullptr\n";
			it = function.stmts.erase(it);
			changed = true;
		} else {
			if (result != *it) {
				delete *it;
				//cout << "result != *it\n";
				*it = result;
				changed = true;
			} 
			it++;
		}
	}

	return changed;
}

