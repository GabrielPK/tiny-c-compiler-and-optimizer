# ifndef LIVEVARS_H
# define LIVEVARS_H
# include "Symbol.h"
# include "Block.h"
# include <unordered_set>

typedef struct LVA_sets {
	unordered_set<Symbol *> gen;
	unordered_set<Symbol *> Symbol *kill;
} LVA_sets;

LVA_sets create_sets(Block block);


# endif /* LIVEVARS_H */
