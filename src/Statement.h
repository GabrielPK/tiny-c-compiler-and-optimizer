/*
 * File:	Statement.h
 *
 * Description:	This file contains the class definitions for three-address
 *		statements.  Unlike the AST, in which all nodes consist of
 *		a single class, subclassing is used here since statements
 *		look very different from one another.
 */

# ifndef STATEMENT_H
# define STATEMENT_H
# include <list>
# include <ostream>
# include <iostream>
# include "Symbol.h"
# include <set>
# include <unordered_set>
# include "literal.h"
# include <map>
//# include "lvn.h"
# include "dataflow.h"

struct Label;
typedef std::list<class Statement *> Statements;

typedef struct LVA_sets {
    std::unordered_set<Symbol *> gen;
	Symbol *kill;
	int isFunc = 0;
} LVA_sets;

typedef struct avail_expr {
	expr_set gen, kill;
	
} avail_expr;

typedef struct cse_uni {
	expr_set uni;
} cse_uni;

class Statement {
protected:
    typedef std::ostream ostream;
	
public:
    virtual ~Statement() {}
    virtual void write(ostream &ostr) const = 0;
    virtual void generate() = 0;
	
    virtual Label *asLabel() const;
    virtual Label *target() const;
    virtual void target(Label *tgt);
    virtual bool fallsThru() const;
	virtual LVA_sets make_lva_sets() const = 0;
	virtual Statement *simplify() = 0;
	virtual Statement *cfold() = 0;
	virtual Statement *valnum(int &val_num) = 0;
	//virtual copy_set gen_uni() = 0;
	// cse
	virtual avail_expr availExpr(cse_uni universe) { avail_expr cur; return cur;};
	virtual void gen_cse_uni(cse_uni &universe) {return;}
	virtual Statement *cse_prop() {return this;}
	virtual void cp_uni(copy_set &universe) { return;}
	virtual void cp_gen_kill(copy_set &gen, copy_set &kill, copy_set &universe, Symbols globals) {return;}
	virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {return false;}
}; 


struct Null : public Statement {
    Null();
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		return sets;
	}
	
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
};


struct Label : public Statement {
    static unsigned _count;
    unsigned _number;
    struct Block *_block;

    Label();
    Block *block() { return _block; }

    virtual void write(ostream &ostr) const;
    virtual void generate();

    virtual Label *asLabel() const;
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
};


struct Jump : public Statement {
    Label *_target;

    Jump(Label *target);
    virtual void write(ostream &ostr) const;
    virtual void generate();

    virtual Label *target() const;
    virtual void target(Label *tgt);
    virtual bool fallsThru() const;
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
};


struct Branch : public Statement {
    int _token;
    Symbol *_left, *_right;
    Label *_target;

    Branch(int token, Symbol *left, Symbol *right, Label *target);
    virtual void write(ostream &ostr) const;
    virtual void generate();

    virtual Label *target() const;
    virtual void target(Label *tgt);
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_left);
		sets.gen.insert(_right);
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		return sets;
	}
	virtual Statement *simplify(); //{ return this;}
	virtual Statement *cfold();// { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
	
    virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
        bool changed = false;
        for(auto &pair : gen) {
            if(_left == pair.first) {
                _left = pair.second;
                changed = true;
            }
            if(_right == pair.first) {
                _right = pair.second;
                changed = true;
            }
        }
        return changed;
    }

};


struct Call : public Statement {
    Symbol *_result, *_function;
    Symbols _arguments;

    Call(Symbol *result, Symbol *function, const Symbols &arguments);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		for (auto &i : _arguments) {
			if (!isNumber(i) && (i->kind() != STRLIT)) {
				sets.gen.insert(i);
			}
		}
		//sets.kill.insert(_result);
		//sets.gen = _arguments;
		sets.isFunc = 1;
		sets.kill = _result;
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num); //{ return this; }
	
	
    virtual void cp_gen_kill(copy_set &gen, copy_set &kill, copy_set &universe, Symbols globals) {

        for(auto &pair : universe) {
            if(pair.first == _result || pair.second == _result) {
                kill.insert(pair);
                if(gen.count(pair) > 0)
                    gen.erase(pair);
            }
        }

		for(auto &sym : globals) {
			if(sym->kind() == GLOBAL && !(sym->type().isFunction())) {
				for(auto &pair : universe) {
					if(sym == pair.first || sym == pair.second) {
						kill.insert(pair);
						gen.erase(pair);
					}
				}
			}
		}

        return;
    }

	
	virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
		bool changed = false;
		for(auto &arg : _arguments) {
			for(auto &pair : gen) {
				if(arg == pair.first){
					arg = pair.second;
					changed = true;
				}
			}
		}

		return changed;
	}
};


struct Return : public Statement {
    Symbol *_expr;

    Return(Symbol *expr);
    virtual void write(ostream &ostr) const;
    virtual void generate();

    virtual bool fallsThru() const;
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_expr);
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
	virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
		bool changed = false;
		for(auto &pair : gen) {
			if(_expr == pair.first) {
				_expr = pair.second;
				changed = true;
			}
		}
		return changed;
	}
};


struct Binary : public Statement {
    int _token;
    Symbol *_result, *_left, *_right;
	struct LVN_expr *lvn_expr;
	
	
    Binary(int token, Symbol *result, Symbol *left, Symbol *right);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_left);
		sets.gen.insert(_right);
		//sets.kill.insert(_result);
		sets.kill = _result;
		return sets;
	}
	virtual Statement *simplify();
	virtual Statement *cfold();
	virtual Statement *valnum(int &val_num); //{return this;}
	virtual avail_expr availExpr(cse_uni universe) {
		avail_expr cur;
		struct expr_tuple gen;
		gen._left = _left;
		gen._right = _right;
		gen._token = _token;
		cur.gen.insert(gen);
		for(auto &i : universe.uni) {
			if(_result == i._left || _result == i._right) {
				//TODO this might have errors
				cur.kill.insert(i);
			}
		}
		
		return cur;
	}
	virtual void gen_cse_uni(cse_uni &universe) {
		struct expr_tuple *temp = new struct expr_tuple;
		temp->_left = _left;
		temp->_right = _right;
		temp->_token = _token;
		universe.uni.insert(*temp);
	}
	virtual Statement *cse_prop() {
	
		return this;
	}

	virtual void cp_gen_kill(copy_set &gen, copy_set &kill, copy_set &universe, Symbols globals) {
		
		for(auto &pair : universe) {
			if(pair.first == _result || pair.second == _result) {
				kill.insert(pair);
				if(gen.count(pair) > 0)
					gen.erase(pair);
			}
		}

		return;
	}
	virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
		bool changed = false;
		for(auto &pair : gen) {
			if(_left == pair.first) {
				_left = pair.second;
				changed = true;
			}
			if(_right == pair.first) {
				_right = pair.second;
				changed = true;
			}
		}
		return changed;
	}
};


struct Unary : public Statement {
    int _token;
    Symbol *_result, *_expr;
	struct LVN_expr *lvn_expr;
    
	Unary(int token, Symbol *result, Symbol *expr);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual struct LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_expr);
		sets.kill = _result;
		//sets.kill.insert(nullptr);
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num); //{ return this; }
	virtual avail_expr availExpr(cse_uni universe) {
		avail_expr cur;
		for(auto &i : universe.uni) {
			if(_result == i._left || _result == i._right) {
				cur.kill.insert(i);
			}
		}
		return cur;
	}

	
    virtual void cp_gen_kill(copy_set &gen, copy_set &kill, copy_set &universe, Symbols globals) {

        for(auto &pair : universe) {
            if(pair.first == _result || pair.second == _result) {
                kill.insert(pair);
                if(gen.count(pair) > 0)
                    gen.erase(pair);
            }
        }

        return;
    }

	
    virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
        bool changed = false;
        for(auto &pair : gen) {
            if(_expr == pair.first) {
                _expr = pair.second;
                changed = true;
            }
        }
        return changed;
    }

};


struct Copy : public Statement {
    Symbol *_result, *_expr;

    Copy(Symbol *result, Symbol *expr);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual struct LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_expr);
		//sets.kill.insert(_result);
		sets.kill = _result;
		return sets;
	}
	virtual Statement *simplify();
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num); //{ return this;}
	virtual avail_expr availExpr(cse_uni universe) {
		avail_expr cur;
		for(auto &i : universe.uni) {
			if(_result == i._left || _result == i._right) {
				cur.kill.insert(i);
			}
		}
		return cur;
	}
	virtual void cp_uni(copy_set &universe) { 
		
		universe.insert(copy_pair(_result, _expr));
		return;
	}
	virtual void cp_gen_kill(copy_set &gen, copy_set &kill, copy_set &universe, Symbols globals) {
		
		kill.insert(copy_pair(_result, _expr));
		for(auto &pair : universe) {
			if(pair.first == _result || pair.second == _result) {
				kill.insert(pair);
				gen.erase(pair);
			}
		}
		gen.insert(copy_pair(_result, _expr));

		return;
	}
	virtual bool cprop(copy_set &gen, copy_set &kill, copy_set &in, copy_set &universe) {
		bool changed = false;
		
		for(auto &pair : gen) {
			if(_expr == pair.first){
				_expr = pair.second;
				changed = true;
			}
		}

		return changed;
	}
};


struct Index : public Statement {
    Symbol *_result, *_array, *_index;

    Index(Symbol *result, Symbol *array, Symbol *index);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual struct LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_index);
		//sets.kill.insert(_result);
		sets.kill = _result;
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
};


struct Update : public Statement {
    Symbol *_array, *_index, *_expr;

    Update(Symbol *array, Symbol *index, Symbol *expr);
    virtual void write(ostream &ostr) const;
    virtual void generate();
	virtual struct LVA_sets make_lva_sets() const {
		LVA_sets sets;
		sets.gen.insert(_index);
		sets.gen.insert(_expr);
		//sets.kill.insert(nullptr);
		sets.kill = nullptr;
		return sets;
	}
	virtual Statement *simplify() { return this;}
	virtual Statement *cfold() { return this;}
	virtual Statement *valnum(int &val_num) { return this; }
};

std::ostream &operator <<(std::ostream &ostr, Statement *stmt);
std::ostream &operator <<(std::ostream &ostr, const Statements &stmts);

# endif /* STATEMENT_H */
