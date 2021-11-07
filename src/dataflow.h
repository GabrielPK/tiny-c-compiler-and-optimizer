# ifndef DATAFLOW_H
# define DATAFLOW_H
# include <ostream>
# include <unordered_set>
# include "Symbol.h"

/* For live variable analysis */

typedef std::unordered_set<Symbol *> sym_set;


/* For available copies */

struct pair_hash {
    template<class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
	return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

typedef std::pair<class Symbol *, class Symbol *> copy_pair;
typedef std::unordered_set<copy_pair, pair_hash> copy_set;


/* For available expressions */

struct expr_tuple {
    class Symbol *_left, *_right;
    int _token;
    bool operator==(const expr_tuple &e) const {
	return _left == e._left && _token == e._token && _right == e._right;
    }
};

struct expr_hash {
    std::size_t operator()(const expr_tuple &expr) const {
	return std::hash<Symbol *>()(expr._left) ^
	    std::hash<Symbol *>()(expr._right) ^ std::hash<int>()(expr._token);
    }
};

typedef std::unordered_set<expr_tuple, expr_hash> expr_set;


/* Destructive set union: s1 = s1 + s2 */

template<class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    class Alloc = std::allocator<Key> >
void insert(std::unordered_set<Key, Hash, Pred, Alloc> &s1,
	const std::unordered_set<Key, Hash, Pred, Alloc> &s2)
{
    s1.insert(s2.begin(), s2.end());
}


/* Destructive set difference: s1 = s1 - s2 */

template<class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    class Alloc = std::allocator<Key> >
void remove(std::unordered_set<Key, Hash, Pred, Alloc> &s1,
	const std::unordered_set<Key, Hash, Pred, Alloc> &s2)
{
    for (auto it = s2.begin(); it != s2.end(); it ++)
	s1.erase(*it);
}


/* Destructive set intersection: s1 = s1 * s2 */

template<class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    class Alloc = std::allocator<Key> >
void filter(std::unordered_set<Key, Hash, Pred, Alloc> &s1,
	const std::unordered_set<Key, Hash, Pred, Alloc> &s2)
{
    auto it = s1.begin();

    while (it != s1.end())
	if (s2.count(*it))
	    it ++;
	else
	    it = s1.erase(it);
}


/* Write a set to a stream */

template<class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    class Alloc = std::allocator<Key> >
std::ostream &operator <<(std::ostream &ostr,
	const std::unordered_set<Key, Hash, Pred, Alloc> &s)
{
    ostr << "{";

    for (auto it = s.begin(); it != s.end(); it ++)
	ostr << (it != s.begin() ? "," : "") << *it;

    ostr << "}";
    return ostr;
}

# endif /* DATAFLOW_H */
