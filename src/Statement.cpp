/*
 * File:	Statement.cpp
 *
 * Description:	This file contains the member function definitions for
 *		three-address statements.
 */

# include "tokens.h"
# include "Statement.h"
# include "lvn.h"

using namespace std;

std::map<Symbol*, int> sym_int;
std::map<struct LVN_expr*, int> expr_int;
unsigned Label::_count = 0;


/*
 * Function:	operator << (private)
 *
 * Description:	Write the name of the given symbol to the specified stream.
 */

static ostream &operator <<(ostream &ostr, Symbol *symbol)
{
    return ostr << (symbol != nullptr ? symbol->name() : "(null)");
}


/*
 * Function:	Statement::asLabel
 *
 * Description:	Default implementation that fails to convert this statement
 *		to a label statement.
 */

Label *Statement::asLabel() const
{
    return nullptr;
}


/*
 * Function:	Statement::target (accessor)
 *
 * Description:	Default implementation to return the target label of this
 *		statement.
 */

Label *Statement::target() const
{
    return nullptr;
}


/*
 * Function:	Statement::target (mutator)
 *
 * Description:	Default implementation to update the target label of this
 *		statement.
 */

void Statement::target(Label *tgt)
{
}


/*
 * Function:	Statement::fallsThru (predicate)
 *
 * Description:	Default implementation to return whether control falls
 *		through from this statement.
 */

bool Statement::fallsThru() const
{
    return true;
}


/*
 * Function:	Null::Null (constructor)
 *
 * Description:	Initialize a null statement.  There is nothing to do here
 *		as null statements merely exist as a placeholder to prevent
 *		a basic block from being collapsed and removed.
 */

Null::Null()
{
}


/*
 * Function:	Null::write
 *
 * Description:	Write a null statement to the specified stream.
 */

void Null::write(ostream &ostr) const
{
}


/*
 * Function:	Label::Label (constructor)
 *
 * Description:	Initialize a label statement.
 */

Label::Label()
    : _number(_count ++), _block(nullptr)
{
}


/*
 * Function:	Label::write
 *
 * Description:	Write a label statement to the specified stream.
 */

void Label::write(ostream &ostr) const
{
    ostr << "L" << _number << ":" << endl;
}


/*
 * Function:	Label::asLabel
 *
 * Description:	Return this statement converted to a label statement.
 */

Label *Label::asLabel() const
{
    return const_cast<Label *>(this);
}


/*
 * Function:	Jump:Jump (constructor)
 *
 * Description:	Initialize a jump statement.
 */

Jump::Jump(Label *target)
    : _target(target)
{
}


/*
 * Function:	Jump::write
 *
 * Description:	Write a jump statement to the specified stream.
 */

void Jump::write(ostream &ostr) const
{
    ostr << "\tgoto L" << _target->_number << endl;
}


/*
 * Function:	Jump::target (accessor)
 *
 * Description:	Return the target label of this statement.
 */

Label *Jump::target() const
{
    return _target;
}


/*
 * Function:	Jump::target (mutator)
 *
 * Description:	Update the target label of this statement.
 */

void Jump::target(Label *tgt)
{
    _target = tgt;
}


/*
 * Function:	Jump::fallsThru (predicate)
 *
 * Description:	Indicate that control does not fall through from this
 *		statement.
 */

bool Jump::fallsThru() const
{
    return false;
}


/*
 * Function:	Branch::Branch (constructor)
 *
 * Description:	Initialize a branch statement.
 */

Branch::Branch(int token, Symbol *left, Symbol *right, Label *target)
    : _token(token), _left(left), _right(right), _target(target)
{
}


/*
 * Function:	Branch::write
 *
 * Description:	Write a branch statement to the specified stream.
 */

void Branch::write(ostream &ostr) const
{
    ostr << "\tif " << _left << " " << lexemes[_token] << " ";
    ostr << _right << " goto L" << _target->_number << endl;
}


/*
 * Function:	Branch::target (accessor)
 *
 * Description:	Return the target label of this statement.
 */

Label *Branch::target() const
{
    return _target;
}


/*
 * Function:	Branch::target (mutator)
 *
 * Description:	Update the target label of this statement.
 */

void Branch::target(Label *tgt)
{
    _target = tgt;
}


/*
 * Function:	Call::Call (constructor)
 *
 * Description:	Initialize a call statement.
 */

Call::Call(Symbol *result, Symbol *function, const Symbols &arguments)
    : _result(result), _function(function), _arguments(arguments)
{
}


/*
 * Function:	Call:write
 *
 * Description:	Write a call statement to the specified stream.
 */

void Call::write(ostream &ostr) const
{
    if (_result != nullptr)
	ostr << "\t" << _result << " := call " << _function << "(";
    else
	ostr << "\tcall " << _function << "(";

    for (unsigned i = 0; i < _arguments.size(); i ++)
	ostr << (i > 0 ? ", " : "") << _arguments[i];

    ostr << ")" << endl;
}


/*
 * Function:	Return::Return (constructor)
 *
 * Description:	Initialize a return statement.
 */

Return::Return(Symbol *expr)
    : _expr(expr)
{
}


/*
 * Function:	Return::write
 *
 * Description:	Write a return statement to the specified stream.
 */

void Return::write(ostream &ostr) const
{
    ostr << "\treturn " << _expr << endl;
}


/*
 * Function:	Return::fallsThru (predicate)
 *
 * Description:	Indicate that control does not fall through from this
 *		statement.
 */

bool Return::fallsThru() const
{
    return false;
}


/*
 * Function:	Binary::Binary (constructor)
 *
 * Description:	Initialize a binary statement.
 */

Binary::Binary(int token, Symbol *result, Symbol *left, Symbol *right)
    : _token(token), _result(result), _left(left), _right(right)
{
}


/*
 * Function:	Binary::write
 *
 * Description:	Write a binary statement to the specified stream.
 */

void Binary::write(ostream &ostr) const
{
    ostr << "\t" << _result << " := " << _left << " ";
    ostr << lexemes[_token] << " " << _right << endl;
}


/*
 * Function:	Unary::Unary (constructor)
 *
 * Description:	Initialize a unary statement.
 */

Unary::Unary(int token, Symbol *result, Symbol *expr)
    : _token(token), _result(result), _expr(expr)
{
}


/*
 * Function:	Unary::write
 *
 * Description:	Write a unary statement to the specified stream.
 */

void Unary::write(ostream &ostr) const
{
    ostr << "\t" << _result << " := " << lexemes[_token] << " ";
    ostr << _expr << endl;
}


/*
 * Function:	Copy::Copy (constructor)
 *
 * Description:	Initialize a copy statement.
 */

Copy::Copy(Symbol *result, Symbol *expr)
    : _result(result), _expr(expr)
{
}


/*
 * Function:	Copy::write
 *
 * Description:	Write a copy statement to the specified stream.
 */

void Copy::write(ostream &ostr) const
{
    ostr << "\t" << _result << " := " << _expr << endl;
}


/*
 * Function:	Index::Index (constructor)
 *
 * Description:	Initialize an array index (i.e., read) statement.
 */

Index::Index(Symbol *result, Symbol *array, Symbol *index)
    : _result(result), _array(array), _index(index)
{
}


/*
 * Function:	Index::write
 *
 * Description:	Write an array index statement to the specified stream.
 */

void Index::write(ostream &ostr) const
{
    ostr << "\t" << _result << " := " << _array << "[" << _index << "]" << endl;
}


/*
 * Function:	Update::Update (constructor)
 *
 * Description:	Initialize an array update (i.e., write) statement.
 */

Update::Update(Symbol *array, Symbol *index, Symbol *expr)
    : _array(array), _index(index), _expr(expr)
{
}


/*
 * Function:	Update::write
 *
 * Description:	Write an array update statement to the specified stream.
 */

void Update::write(ostream &ostr) const
{
    ostr << "\t" << _array << "[" << _index << "] := " << _expr << endl;
}


/*
 * Function:	operator <<
 *
 * Description:	Write a statement to the specified stream.
 */

ostream &operator <<(ostream &ostr, Statement *stmt)
{
    stmt->write(ostr);
    return ostr;
}


/*
 * Function:	operator <<
 *
 * Description:	Write a vector of statements to the specified stream.
 */

ostream &operator <<(ostream &ostr, const Statements &stmts)
{
    for (auto it = stmts.begin(); it != stmts.end(); it ++)
	ostr << *it;

    return ostr;
}

Statement *Binary::simplify() { 
	// x + 0, 0 + x
	if(_token == '+') {
		if(_right->name() == "0") {
			return new Copy(_result, _left);
		} else if (_left->name() == "0") {
			return new Copy(_result, _right);
		}
	// x * 1, 1 * x, x * 0, 0 * x
	} else if (_token == '*') {
		if(_right->name() == "1") {
			return new Copy(_result, _left);
		} else if (_left->name() == "1") {
			return new Copy(_result, _right);
		} else if (_right->name() == "0") {
			return new Copy(_result, new Symbol("0", Type(INT), NUM));
		} else if (_left->name() == "0") {
			return new Copy(_result, new Symbol("0", Type(INT), NUM));
		}
	// x - 0, 0 - x, x - x
	} else if (_token == '-') {
		if(_right->name() == "0") {
			return new Copy(_result, _left);
		} else if(_left->name() == "0") {
			return new Unary(NEGATE,_result,_right);
		} else if(_right->name() == _left->name()) {
			return new Copy(_result, new Symbol("0", Type(INT), NUM));
		} 
	// x / 1, 0 / x
	} else if (_token == '/') {
		if(_right->name() == "1") {
			return new Copy(_result, _left);
		} else if(_left->name() == "0") {
			return new Copy(_result, new Symbol("0", Type(INT), NUM));
		}
	} else if ((_token == EQL) || (_token == LEQ) || (_token == GEQ)) {
		if(_right->name() == _left->name()) {
			return new Copy(_result, new Symbol("1", Type(INT), NUM));
		} 
	} else if ((_token == NEQ) || (_token == GTN) || (_token == LTN)) {
		if(_right->name() == _left->name()) {
			return new Copy(_result, new Symbol("0", Type(INT), NUM));
		}
	}
	return this;
}

Statement *Copy::simplify(){
	if(_result->name() == _expr->name()) {
		return nullptr;
	} else 
		return this;
}

Statement *Branch::simplify(){
	if((_token == EQL) || (_token == LEQ) || (_token == GEQ)) {
		if(_left->name() == _right->name()) {
			return new Branch(EQL, new Symbol("0", Type(INT), NUM), new Symbol("0", Type(INT), NUM), _target);
		} 
	} else if ((_token == NEQ) || (_token == GTN) || (_token == LTN)) {
		if(_left->name() == _right->name()) {
			return new Branch(EQL, new Symbol("1", Type(INT), NUM), new Symbol("0", Type(INT), NUM), _target);
		}
	}
	return this;
}

Statement *Branch::cfold() {
	
	if(isNumber(_left) && isNumber(_right)) {
		if(_token == EQL) 
			if(_left->name() == _right->name()) 
				return new Jump(_target);
			else 
				return nullptr;

		else if(_token == NEQ) 
			if(_left->name() != _right->name()) 
				return new Jump(_target);
			else 
				return nullptr;

		else if(_token == GEQ) 
			if(stoi(_left->name()) >= stoi(_right->name())) 
				return new Jump(_target);
			else
				return nullptr;

		else if(_token == GTN) 
			if(stoi(_left->name()) > stoi(_right->name()))
				return new Jump(_target);
			else
				return nullptr;

		else if(_token == LEQ)
			if(stoi(_left->name()) <= stoi(_right->name()))
				return new Jump(_target);
			else
				return nullptr;

		else if(_token == LTN)
			if(stoi(_left->name()) < stoi(_right->name()))
				return new Jump(_target);
			else
				return nullptr;
			
	}
	
	return this;
}

Statement *Binary::cfold() {
	int res;
	if(isNumber(_left) && isNumber(_right)) {
		if(_token == '+') {
			res = stoi(_left->name()) + stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == '-') {
			res = stoi(_left->name()) - stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == '*') {	
			res = stoi(_left->name()) * stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if ((_token == '/') && (_right->name() != "0")) {
			res = stoi(_left->name()) / stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if ((_token == '%') && (_right->name() != "0")) {
			res = stoi(_left->name()) % stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == NEQ) {
			res = stoi(_left->name()) != stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == EQL) {
			res = stoi(_left->name()) == stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == GTN) {
			res = stoi(_left->name()) > stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == LTN) {
			res = stoi(_left->name()) < stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == LEQ) {
			res = stoi(_left->name()) <= stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == GEQ) {
			res = stoi(_left->name()) >= stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == AND) {
			res = stoi(_left->name()) && stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} else if (_token == OR) {
			res = stoi(_left->name()) || stoi(_right->name());
			return new Copy(_result, new Symbol(to_string(res), Type(INT), NUM));
		} 
	} 
	return this;
}

Statement *Call::valnum(int &val_num) {
	for(auto &symbol: lvn_globals) {
		if(sym_int.count(symbol) > 0) {
			sym_int.erase(symbol);
		}
	}
	return this;	
}

Statement *Copy::valnum(int &val_num) {
	if(sym_int.count(_expr) == 0) {
		sym_int[_expr] = val_num;
		sym_int[_result] = val_num;
		val_num++;
	} else {
		sym_int[_result] = sym_int[_expr];
	}
	
	return this;
}

Statement *Binary::valnum(int &val_num) {
	if(_token == '+' || _token == '*' || _token == EQL || _token == NEQ || _token == AND || _token == OR) {
		if(sym_int.count(_left) == 0) {
			sym_int[_left] = val_num++;
		} 
		if(sym_int.count(_right) == 0) {
			sym_int[_right] = val_num++;
		}
		bool found = false;
		int tnum = 0;
		for(auto it = expr_int.begin(); (it != expr_int.end()) && (!found); it++) {
			if(sym_int[_left] <= sym_int[_right]) {
				if((it->first->left == sym_int[_left]) && (it->first->right == sym_int[_right]) && (it->first->op == _token)) {
				found = true;
				tnum = it->second;
				}
			} else {
				if((it->first->left == sym_int[_right]) && (it->first->right == sym_int[_left]) && (it->first->op == _token)) {
				found = true;
				tnum = it->second;
				}
			}
		}
		if(found) {
			for(auto it = sym_int.begin(); it != sym_int.end(); it++) {
				if(it->second == tnum) {
					sym_int[_result] = tnum;//val_num++;
					return new Copy(_result, it->first);
				}
			}
		} else {
			lvn_expr = new LVN_expr;
			if(sym_int[_left] <= sym_int[_right]) {
				lvn_expr->left = sym_int[_left];
				lvn_expr->right = sym_int[_right];
			} else {
				lvn_expr->left = sym_int[_right];
				lvn_expr->right = sym_int[_left];
			}
			lvn_expr->op = _token;
			expr_int[lvn_expr] = val_num;
			sym_int[_result] = val_num++;
			return this;
		}
	} else if(_token == GTN || _token == LTN || _token == GEQ || _token == LEQ) {
        if(sym_int.count(_left) == 0) {
            sym_int[_left] = val_num++;
        }
        if(sym_int.count(_right) == 0) {
            sym_int[_right] = val_num++;
        }
		bool found = false;
		int tnum = 0;
        for(auto it = expr_int.begin(); (it != expr_int.end()) && (!found); it++) {
			if(_token <= duals[_token]) {
        		if((it->first->left == sym_int[_left]) && (it->first->right == sym_int[_right]) && (it->first->op == _token)) {
            		found = true;
            		tnum = it->second;
            	} 
			} else if ((it->first->left == sym_int[_right]) && (it->first->right == sym_int[_left]) && (it->first->op == duals[_token])){
				found = true;
				tnum = it->second;
			}
        }
        if(found) {
            for(auto it = sym_int.begin(); it != sym_int.end(); it++) {
                if(it->second == tnum) {
                    sym_int[_result] = tnum;//val_num++;
                    return new Copy(_result, it->first);
                }
            }
        } else {
            lvn_expr = new LVN_expr;
            if(_token <= duals[_token]) {
                lvn_expr->left = sym_int[_left];
                lvn_expr->right = sym_int[_right];
				lvn_expr->op = _token;
            } else {
                lvn_expr->left = sym_int[_right];
                lvn_expr->right = sym_int[_left];
            	lvn_expr->op = duals[_token];
            }
            expr_int[lvn_expr] = val_num;
            sym_int[_result] = val_num++;
            return this;
        }	
	} else {
		if(sym_int.count(_left) == 0) {
			sym_int[_left] = val_num++;
		} 
		if(sym_int.count(_right) == 0) {
			sym_int[_right] = val_num++;
		}
		bool found = false;
		int tnum = 0;
		for(auto it = expr_int.begin(); (it != expr_int.end()) && (!found); it++) {
			if((it->first->left == sym_int[_left]) && (it->first->right == sym_int[_right]) && (it->first->op == _token)) {
			found = true;
			tnum = it->second;
			}
		}
		if(found) {
			for(auto it = sym_int.begin(); it != sym_int.end(); it++) {
				if(it->second == tnum) {
					sym_int[_result] = tnum;//val_num++;
					return new Copy(_result, it->first);
				}
			}
		} else {
			lvn_expr = new LVN_expr;
			lvn_expr->left = sym_int[_left];
			lvn_expr->right = sym_int[_right];
			lvn_expr->op = _token;
			expr_int[lvn_expr] = val_num;
			sym_int[_result] = val_num++;
			return this;
		}
	}

	return this;
}

Statement *Unary::valnum(int &val_num) {
	bool found = false;
	int tnum = 0;
	if(sym_int.count(_expr) == 0) {
		sym_int[_expr] = val_num++;
	} 
	for(auto it = expr_int.begin(); (it != expr_int.end() && !found); it++) {
		if((it->first->left == -1) && (it->first->right == sym_int[_expr])) {
			found = true;
			tnum = it->second;
		}
	}
	if(found) {
		for(auto it = sym_int.begin(); it != sym_int.end(); it++) {
            if(it->second == tnum) {
            	sym_int[_result] = val_num++;
                return new Copy(_result, it->first);
        	}
		}
	} else {
		lvn_expr->op = _token;
		lvn_expr->left = -1;
		lvn_expr->right = sym_int[_expr];
		expr_int[lvn_expr] = val_num;
		sym_int[_result] = val_num++;
	}
	return this;
}

/*
Statement *Branch::valnum(int &val_num) {
	
}*/
