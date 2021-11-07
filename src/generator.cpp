/*
 * File:	generator.cpp
 *
 * Description:	This file contains the public and private function and
 *		variable definitions for the code generator for Tiny C.
 *
 *		The register allocator is very simple.
 *
 *		It assumes that a source operand is either never used again
 *		or can be reread from memory, and therefore its register
 *		can be used for the result or deallocated after use.
 *
 *		It does not take into account commutativity or duality of
 *		binary operators.
 */

# include <cassert>
# include <iostream>
# include <algorithm>
# include <unordered_map>
# include "literal.h"
# include "machine.h"
# include "Register.h"
# include "flowgraph.h"
# include "generator.h"

using namespace std;

# define isVariable(s) ((s)->kind() == LOCAL || (s)->kind() == GLOBAL)

# define nextuse(s) false
# define nextdef(s) false
# define liveonexit(s) (isVariable(s))

# define shrink_pool(pool,reg) \
    pool.erase(remove(pool.begin(), pool.end(), reg), pool.end())

# define isParamArray(s) \
    ((s)->kind() == LOCAL && (s)->type().isPointer())

# define isLocalArray(s) \
    ((s)->kind() == LOCAL && (s)->type().isArray() && !(s)->type().isPointer())

# define isGlobalArray(s) \
    ((s)->kind() == GLOBAL && (s)->type().isArray())

# define isImmediate(s) \
    ((s)->kind() == NUM || (s)->kind() == STRLIT || isGlobalArray(s))

# define isByteObject(s) \
    ((s)->type().isScalar() && (s)->type().size() == 1)

//TODO: LRA

bool nextuse1(Symbol *s) {
	
	return false;
}

bool nextdef1(Symbol *s) {
	
	return false;
}

bool liveonexit1(Symbol *s) {
	
	return false;
}


/* For code generation */

static unsigned max_args;
static int offset, param_offset;
static unordered_map<Symbol *, int> strings;
static Label *return_label;


/* The registers */

static Register *eax = new Register("%eax", "%al");
static Register *ecx = new Register("%ecx", "%cl");
static Register *edx = new Register("%edx", "%dl");

typedef vector <Register *> Registers;
static Registers caller_saved = {eax, ecx, edx};
static Registers registers = caller_saved;


/* Data transfer primitives */

static void move(const Register *src, const Register *dst);
static void load(Symbol *sym, const Register *reg);
static void store(const Register *reg, Symbol *sym);
static void store(int imm, Symbol *sym);

static void iload(const Register *src, const Register *dst, unsigned size);
static void istore(const Register *src, const Register *dst, unsigned size);
static void istore(int imm, const Register *dst, unsigned size);


/* Convenience functions */

static void getreg(Symbol *dst, Symbol *src);
static void load(Symbol *sym, Registers &pool = registers);
static void spill(Register *reg);
static void release(Symbol *sym);
static void save(Symbol *sym);


/* The register allocator */

static void deallocate(Register *reg);
static Register *allocate(Registers &pool = registers);


/*
 * Function:	align (private)
 *
 * Description:	Return the number of bytes necessary to align the given
 *		offset on the stack.
 */

static int align(int offset)
{
    if (offset % STACK_ALIGNMENT == 0)
	return 0;

    return STACK_ALIGNMENT - (abs(offset) % STACK_ALIGNMENT);
}


/*
 * Function:	basesize (private)
 *
 * Description:	Return the base size in bytes of an array or function.
 */

static unsigned basesize(const Symbol *sym)
{
    return Type(sym->type().specifier()).size();
}


/*
 * Function:	operand (private)
 *
 * Description:	Return a string operand designating the memory location of
 *		the given operand.
 */

static string operand(Symbol *sym)
{
    if (sym->kind() == NUM)
	return "$" + sym->name();

    if (sym->kind() == GLOBAL && sym->type().isArray())
	return "$" + string(global_prefix) + sym->name();

    if (sym->kind() == GLOBAL)
	return global_prefix + sym->name();

    if (sym->kind() == STRLIT) {
	if (strings.count(sym) == 0)
	    strings[sym] = strings.size();

	return "$" + string(string_prefix) + to_string(strings[sym]);
    }

    assert(sym->kind() == LOCAL || sym->kind() == TEMP);

    if (sym->_offset == 0)
	sym->_offset = offset -= sym->type().size();

    return to_string(sym->_offset) + "(%ebp)";
}


/*
 * Function:	operator << (private)
 *
 * Description:	Write a symbol to the specified output stream.  If the
 *		symbol is in a register, it is preferred.  Otherwise,
 *		memory is referenced.
 */

static ostream &operator <<(ostream &ostr, Symbol *symbol)
{
    if (symbol->_register != nullptr)
	return ostr << symbol->_register;

    return ostr << operand(symbol);
}


/*
 * Function:	assign (private)
 *
 * Description:	Assign the given symbol to the given register.  No assembly
 *		code is generated here as only the pointers are updated.
 */

static void assign(Symbol *symbol, Register *reg)
{
    if (symbol != nullptr) {
	if (symbol->_register != nullptr)
	    symbol->_register->_symbol = nullptr;

	symbol->_register = reg;
    }

    if (reg != nullptr) {
	if (reg->_symbol != nullptr)
	    reg->_symbol->_register = nullptr;

	reg->_symbol = symbol;
    }
}


/*
 * Function:	move (private)
 *
 * Description:	Move one register to another.  This function does NOT
 *		check if the destination register is occupied, and The
 *		register mappings are NOT updated.
 */

static void move(const Register *src, const Register *dst)
{
    assert(src != nullptr && dst != nullptr);
    cout << "\tmovl\t" << src << ", " << dst << endl;
}


/*
 * Function:	load (private)
 *
 * Description:	Load the symbol from memory into the given register.  This
 *		function does NOT check if the register is occupied, and
 *		the register mappings are NOT updated.
 */

static void load(Symbol *sym, const Register *reg)
{
    assert(sym != nullptr && reg != nullptr);

    if (isLocalArray(sym))
	cout << "\tleal\t" << operand(sym) << ", " << reg << endl;
    else if (isByteObject(sym))
	cout << "\tmovsbl\t" << operand(sym) << ", " << reg << endl;
    else
	cout << "\tmovl\t" << operand(sym) << ", " << reg << endl;
}


/*
 * Function:	store (private)
 *
 * Description:	Store the register to the memory location designated by the
 *		given symbol.  The register mappings are NOT updated.
 */

static void store(const Register *reg, Symbol *sym)
{
    assert(reg != nullptr && sym != nullptr);

    if (isByteObject(sym))
	cout << "\tmovb\t" << reg->byte() << ", " << operand(sym) << endl;
    else
	cout << "\tmovl\t" << reg->name() << ", " << operand(sym) << endl;
}


/*
 * Function:	store (private)
 *
 * Description:	Store an immediate value into the memory location
 *		designated by the given symbol.  The register mappings are
 *		NOT updated.
 */

static void store(int imm, Symbol *sym)
{
    assert(sym != nullptr);

    if (sym->type().size() == 1)
	cout << "\tmovb\t$" << (char) imm << ", " << operand(sym) << endl;
    else
	cout << "\tmovl\t$" << imm << ", " << operand(sym) << endl;
}


/*
 * Function:	iload (private)
 *
 * Description:	Load the destination register with the value in the memory
 *		location specified by the source register.  This function
 *		does NOT check if the register is occupied, and does NOT
 *		update the register mappings.
 */

static void iload(const Register *src, const Register *dst, unsigned size)
{
    assert(src != nullptr && dst != nullptr && (size == 1 || size == 4));

    if (size == 1)
	cout << "\tmovsbl\t(" << src << "), " << dst << endl;
    else
	cout << "\tmovl\t(" << src << "), " << dst << endl;
}


/*
 * Function:	istore (private)
 *
 * Description:	Store the source register into the memory location
 *		specified by the destination register.  The register
 *		mappings are NOT updated.
 */

static void istore(const Register *src, const Register *dst, unsigned size)
{
    assert(src != nullptr && dst != nullptr && (size == 1 || size == 4));

    if (size == 1)
	cout << "\tmovb\t" << src->byte() << ", (" << dst << ")" << endl;
    else
	cout << "\tmovl\t" << src->name() << ", (" << dst << ")" << endl;
}


/*
 * Function:	istore (private)
 *
 * Description:	Store an immediate value into the memory location
 *		specified by the destination register.  The register
 *		mappings are NOT updated.
 */

static void istore(int imm, const Register *dst, unsigned size)
{
    assert(dst != nullptr && (size == 1 || size == 4));

    if (size == 1)
	cout << "\tmovb\t$" << (int) (char) imm << ", (" << dst << ")" << endl;
    else
	cout << "\tmovl\t$" << imm << ", (" << dst << ")" << endl;
}


/*
 * Function:	getreg (private)
 *
 * Description:	Convenience function to get a register for a destination,
 *		using the source operand if it is available, and allocating
 *		a new register otherwise.
 */

static void getreg(Symbol *dst, Symbol *src)
{
    if (src != dst) {
	if (src->_register != nullptr && !nextuse(src))
	    assign(dst, src->_register);
	else {
	    auto reg = src->_register;
	    assign(dst, allocate());

	    if (src->_register != nullptr)
		move(src->_register, dst->_register);
	    else if (reg != dst->_register)
		load(src, dst->_register);
	}

    } else if (dst->_register == nullptr) {
	assign(dst, allocate());
	load(src, dst->_register);
    }
}


/*
 * Function:	load (private)
 *
 * Description:	Convenience function to load a symbol into any available
 *		register from the given pool.  The symbol is only loaded if
 *		necessary, and the register mappings ARE updated.
 */

static void load(Symbol *sym, Registers &pool)
{
    assert(sym != nullptr);

    if (sym->_register == nullptr) {
	assign(sym, allocate(pool));
	load(sym, sym->_register);
    }
}


/*
 * Function:	spill (private)
 *
 * Description:	Convenience function to store a register to memory and then
 *		deallocate the register.  The register mappings ARE
 *		updated.
 */

static void spill(Register *reg)
{
    if (reg->_symbol != nullptr) {
	store(reg, reg->_symbol);
	deallocate(reg);
    }
}


/*
 * Function:	release (private)
 *
 * Description:	Convenience function to deallocate the register of a symbol
 *		if the symbol is not used again.
 */

void release(Symbol *sym)
{
    if (!nextuse(sym))
	deallocate(sym->_register);
}


/*
 * Function:	save (private)
 *
 * Description:	Convenience function to store a symbol by writing it back
 *		to memory if necessary.
 */

void save(Symbol *sym)
{
    sym->_register->_dirty = true;

    if (!nextdef(sym) && liveonexit(sym)) {
	store(sym->_register, sym);
	sym->_register->_dirty = false;
	release(sym);
    }
}


/*
 * Function:	deallocate (private)
 *
 * Description:	Deallocate the given register, which may be null.
 */

static void deallocate(Register *reg)
{
    assign(nullptr, reg);

    if (reg != nullptr)
	reg->_dirty = false;
}


/*
 * Function:	allocate (private)
 *
 * Description:	Allocate a register from the given register pool.  If no
 *		register is available, then the first one is spilled.
 */

static Register *allocate(Registers &pool)
{
    assert(!pool.empty());

    for (auto reg : pool)
	if (reg->_symbol == nullptr)
	    return reg;

    spill(pool[0]);
    assign(nullptr, pool[0]);

    return pool[0];
}


/*
 * Function:	Null::generate
 *
 * Description:	Generate code for a null statement.
 */

void Null::generate()
{
}


/*
 * Function:	Label::generate
 *
 * Description:	Generate code for a label statement.
 */

void Label::generate()
{
    cout << label_prefix << _number << ":" << endl;
}


/*
 * Function:	Jump::generate
 *
 * Description:	Generate code for a jump statement.
 */

void Jump::generate()
{
    cout << "\tjmp\t" << label_prefix << _target->_number << endl;
}


/*
 * Function:	Branch::generate
 *
 * Description:	Generate code for a branch statement.
 */

void Branch::generate()
{
    static unordered_map<int, string> jump_ops = {
	{EQL, "je"}, {NEQ, "jne"}, {LEQ, "jle"},
	{GEQ, "jge"}, {'<', "jl"}, {'>', "jg"},
    };


    load(_left);

    cout << "\tcmpl\t" << _right << ", " << _left->_register << endl;
    cout << "\t" << jump_ops[_token] << "\t";
    cout << label_prefix << _target->_number << endl;

    release(_left);
    release(_right);
}


/*
 * Function:	loadArgument (private)
 *
 * Description:	Load an argument onto the stack.  This function is more
 *		complicated than it should be because an argument can be an
 *		array or a string literal, in which case we have to
 *		effectively compute its address here.
 */

static void loadArgument(Symbol *arg, int offset, const string &base)
{
    if (isLocalArray(arg)) {
	assign(arg, allocate());
	load(arg, arg->_register);
	cout << "\tmovl\t" << arg;

    } else {
	if (!isImmediate(arg))
	    load(arg);

	cout << "\tmovl\t" << arg;
    }

    cout << ", " << offset << "(" << base << ")" << endl;
    release(arg);
}


/*
 * Function:	Call::generate
 *
 * Description:	Generate code for a call statement.
 */

void Call::generate()
{
    if (_arguments.size() > max_args)
	max_args = _arguments.size();

    for (int i = _arguments.size() - 1; i >= 0; i --)
	loadArgument(_arguments[i], SIZEOF_ARG * i, "%esp");

    for (auto reg : caller_saved)
	spill(reg);

    cout << "\tcall\t" << global_prefix << _function->name() << endl;

    if (_result != nullptr) {
	if (basesize(_function) == 1)
	    cout << "\tmovsbl\t%al, %eax" << endl;

	assign(_result, eax);
	save(_result);
    }
}


/*
 * Function:	Return::generate
 *
 * Description:	Generate code for a return statement.
 */

void Return::generate()
{
    if (_expr->_register == nullptr)
	load(_expr, eax);
    else if (_expr->_register != eax)
	move(_expr->_register, eax);

    cout << "\tjmp\t" << label_prefix << return_label->_number << endl;

    release(_expr);
    deallocate(eax);
}


/*
 * Function:	Binary::generate
 *
 * Description:	Generate code for a binary statement.
 */

void Binary::generate()
{
    Register *reg;

    static unordered_map<int, string> set_ops = {
	{EQL, "sete"}, {NEQ, "setne"}, {LEQ, "setle"},
	{GEQ, "setge"}, {'<', "setl"}, {'>', "setg"},
    };


    switch(_token) {
    case EQL: case NEQ: case LEQ: case GEQ: case '<': case '>':
	load(_left);
	cout << "\tcmpl\t" << _right << ", " << _left << endl;
	release(_left);
	release(_right);

	reg = allocate();
	assign(_result, reg);
	cout << "\t" << set_ops[_token] << "\t" << reg->byte() << endl;
	cout << "\tmovzbl\t" << reg->byte() << ", " << reg << endl;
	break;


    case '+':
	getreg(_result, _left);
	cout << "\taddl\t" << _right << ", " << _result << endl;
	release(_right);
	break;


    case '-':
	getreg(_result, _left);
	cout << "\tsubl\t" << _right << ", " << _result << endl;
	release(_right);
	break;


    case '*':
	getreg(_result, _left);
	cout << "\timull\t" << _right << ", " << _result << endl;
	release(_right);
	break;


    case '/':
    case '%':
	if (isNumber(_right) && _right->_register != ecx) {
	    spill(ecx);
	    assign(_right, ecx);
	    load(_right, ecx);
	}

	if (_left->_register == nullptr) {
	    spill(eax);
	    load(_left, eax);
	} else if (_left->_register != eax) {
	    spill(eax);
	    move(_left->_register, eax);
	}

	spill(edx);

	cout << "\tcltd" << endl;
	cout << "\tidivl\t" << _right << endl;

	release(_right);
	deallocate(_left->_register);
	assign(_result, _token == '/' ? eax : edx);
	break;


    default:
	assert(0);
    }

    save(_result);
}


/*
 * Function:	Unary::generate
 *
 * Description:	Generate code for a unary statement.
 */

void Unary::generate()
{
    switch(_token) {
    case NEGATE:
	getreg(_result, _expr);
	cout << "\tnegl\t" << _result << endl;
	break;


    case INT:
	assign(_result, allocate());
	cout << "\tmovsbl\t" << _expr << ", " << _result << endl;
	break;
    }

    save(_result);
}


/*
 * Function:	Copy::generate
 *
 * Description:	Generate code for a copy statement.
 */

void Copy::generate()
{
    if (isNumber(_expr) && isVariable(_result))
	store(valueOf(_expr), _result);

    else {
	getreg(_result, _expr);
	save(_result);
    }
}


/*
 * Function:	Index::generate
 *
 * Description:	Generate code for an array index statement.
 */

void Index::generate()
{
    /* Global variable: static address */

    if (_array->kind() == GLOBAL) {
	getreg(_result, _index);
	cout << "\taddl\t" << operand(_array) << ", " << _result << endl;


    /* Parameter: address is already given as the parameter */

    } else if (_array->type().isPointer()) {
	getreg(_result, _index);
	cout << "\taddl\t" << _array << ", " << _result << endl;


    /* Local: need to compute address */

    } else {
	assign(_result, allocate());
	load(_array, _result->_register);
	//cout << "\tleal\t" << operand(_array) << ", " << _result << endl;
	cout << "\taddl\t" << _index << ", " << _result << endl;
	release(_index);
    }

    iload(_result->_register, _result->_register, basesize(_array));
    release(_array);
    save(_result);
}


/*
 * Function:	Update::generate
 *
 * Description:	Generate code for an array update statement.
 */

void Update::generate()
{
    /* Global variable: static address */

    if (_array->kind() == GLOBAL) {
	getreg(_array, _index);
	cout << "\taddl\t" << operand(_array) << ", " << _array << endl;


    /* Parameter: address is already given as the parameter */

    } else if (_array->type().isPointer()) {
	getreg(_array, _index);
	cout << "\taddl\t" << operand(_array) << ", " << _array << endl;


    /* Local: need to compute address */

    } else {
	assign(_array, allocate());
	load(_array, _array->_register);
	//cout << "\tleal\t" << operand(_array) << ", " << _array << endl;
	cout << "\taddl\t" << _index << ", " << _array << endl;
	release(_index);
    }


    /* Load the expression if necessary */

    if (isNumber(_expr))
	istore(valueOf(_expr), _array->_register, basesize(_array));

    else {
	Registers pool = registers;

	shrink_pool(pool, _array->_register);
	load(_expr, pool);

	assert(_expr->_register != _array->_register);
	istore(_expr->_register, _array->_register, basesize(_array));
	release(_expr);
    }

    deallocate(_array->_register);
}


/*
 * Function:	generateFunction
 *
 * Description:	Generate code for the given function.
 */

void generateFunction(Function &function)
{
    Blocks blocks;
    unsigned num_formals;
    const Symbols &symbols = function.locals->symbols();
    string name;


    /* Assign offsets to the parameters and local variables. */

    offset = 0;
    max_args = 0;
    param_offset = SIZEOF_REG * 2;
    num_formals = function.symbol->type().parameters()->size();

    for (unsigned i = 0; i < symbols.size(); i ++)
	if (i < num_formals)
	    symbols[i]->_offset = param_offset + SIZEOF_ARG * i;
	else {
	    offset -= symbols[i]->type().size();
	    symbols[i]->_offset = offset;
	}


    /* Emit our prologue. */

    name = function.symbol->name();
    cout << global_prefix << name << ":" << endl;

    cout << "\tpushl\t%ebp" << endl;
    cout << "\tmovl\t%esp, %ebp" << endl;
    cout << "\tsubl\t$" << name << ".size, %esp" << endl;


    /* Generate code for the function body. */

    rebuildFlowgraph(function);
    return_label = function.stmts.back()->asLabel();


    blocks = getBlocks(function);

    for (auto block : blocks) {
	for (auto it = block->first(); it != block->last(); it ++)
	    (*it)->generate();

	for (auto reg : registers)
	    assign(nullptr, reg);
    }


    /* Generate our epilogue. */

    return_label->generate();

    cout << "\tmovl\t%ebp, %esp" << endl;
    cout << "\tpopl\t%ebp" << endl;
    cout << "\tret" << endl << endl;

    offset -= max_args * SIZEOF_ARG;
    offset -= align(offset - param_offset);
    cout << "\t.set\t" << name << ".size, " << -offset << endl;
    cout << "\t.globl\t" << global_prefix << name << endl << endl;
}


/*
 * Function:	generateGlobals
 *
 * Description:	Generate code for all global variables and string literals.
 */

void generateGlobals(Scope *globals)
{
    bool inDataSegment = false;
    const Symbols &symbols = globals->symbols();

    for (unsigned i = 0; i < symbols.size(); i ++)
	if (symbols[i]->kind() == GLOBAL && !symbols[i]->type().isFunction()) {
	    cout << "\t.comm\t" << global_prefix << symbols[i]->name();
	    cout << ", " << symbols[i]->type().size() << endl;
	}

    Symbols literals = getLiterals();

    for (unsigned i = 0; i < literals.size(); i ++)
	if (literals[i]->kind() == STRLIT) {
	    if (!inDataSegment) {
		cout << "\t.data" << endl;
		inDataSegment = true;
	    }

	    cout << string_prefix << strings[literals[i]] << ":\t.asciz\t";
	    cout << literals[i]->name() << endl;
	}
}
