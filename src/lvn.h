# ifndef LVN_H
# define LVN_H

struct LVN_expr {
    //Symbol *left, *right;
	int left, right, op;
};
extern std::map<Symbol*, int> sym_int;
extern std::map<struct LVN_expr*, int> expr_int;
extern Symbols lvn_globals;
//std::map<Symbol*, int> sym_int;
//std::map<struct LVN_expr*, int> expr_int;

# endif /* LVN_H */
