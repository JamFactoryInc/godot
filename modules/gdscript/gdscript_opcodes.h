/**************************************************************************/
/*  gdscript_opcodes.h                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef GDSCRIPT_OPCODES_H
#define GDSCRIPT_OPCODES_H

#include "modules/gdscript/gdscript_function.h"

const int POINTER_SIZE = sizeof(size_t);

/**
 * @brief A
 *
 * @tparam POINTER_TYPE
 */
template<typename POINTER_TYPE = void>
struct EncodedPointer {
    constexpr static const int PTR_SIZE = POINTER_SIZE / sizeof(int);

    // here we encode the pointer into 4-byte-aligned parts so that the enclosing Opcode struct is also 4-byte-aligned
    // if this is just a size_t or POINTER_TYPE, it will force the enclosing struct to be an additional 4 bytes in size,
    // causing crashes and/or buffer overflows
    int ptr_parts[EncodedPointer::PTR_SIZE];

public:
    inline EncodedPointer(POINTER_TYPE &ptr) {
        *this = *reinterpret_cast<EncodedPointer<POINTER_TYPE> *>(ptr);
    }

    /**
     * @brief retrieves an aligned copy of the encoded pointer as its concrete type
     */
    constexpr inline POINTER_TYPE get() {
        // it's very important we return a copy of the data after reinterpreting to a pointer type
        // because if we return the data itself, it very well might be misaligned, and cause notable performance issues
        return *reinterpret_cast<POINTER_TYPE>(&ptr_parts);
    }
};

struct TypeInvariant;

struct VariantAddress {
    friend struct TypeInvariant;

    VariantAddress(int _address_data): address_data(_address_data) {}

    int address_data;
public:
    /**
     * @brief Looks up the address of the variant pointer represented by this Address via its stored type and stack index
     */
    inline Variant *get_variant_ptr(Variant *variant_addresses[GDScriptFunction::Address::ADDR_TYPE_MAX]) {
        return &variant_addresses[this->get_address_type()][this->get_address_index()];
    }

    inline int get_address_index() const {
        return this->address_data & GDScriptFunction::Address::ADDR_MASK;
    }

    inline GDScriptFunction::Address get_address_type() const {
        return static_cast<GDScriptFunction::Address>((this->address_data & GDScriptFunction::Address::ADDR_TYPE_MASK)
                >> GDScriptFunction::Address::ADDR_BITS);
    }

    bool operator==(const VariantAddress& other) const
    {
        return address_data == other.address_data;
    }

    static std::size_t hash(const VariantAddress& address) { return address.address_data; }
};

/**
 * The signature of a binary operation represented by the left and right operand types
 */
struct Signature {
    int type_data;

public:
    /**
     * @brief An uninitialized Signature.
     * This is most likely present if the signature's function has not yet been called
     */
    static const Signature UNINIT;
    /**
     * @brief A signature value that exists only not to be UNINIT. A sort of 'processed' flag
     */
    static const Signature OPAQUE;

    constexpr inline bool operator==(Signature &rhs) {
        return this->type_data == rhs.type_data;
    }

    inline Signature(Variant *&left, Variant *&right) :
            type_data((left->get_type() << 8) | right->get_type()) {}

    inline Signature(Variant::Type left_type, Variant::Type right_type) :
            type_data((left_type << 8) | right_type) {}

    inline Variant::Type get_left_type() {
        return static_cast<Variant::Type>(type_data >> 8);
    }

    inline Variant::Type get_right_type() {
        return static_cast<Variant::Type>(type_data & 0xFF);
    }

    constexpr inline bool is_uninit() {
        return this->type_data == 0;
    }

private:
    Signature(int value) :
            type_data(value) {}
};

enum ArgType {
    _TYPE,
    _VARIANT,
    _OPERATOR,
    _SIGNATURE,
    _INT,
    _OPERATOR_EVAL,
};

#define RUNTIME_ARG_TYPE(ARG_TYPE, TYPE_AT_RUNTIME) \
template <> \
struct RuntimeTypeLookup<ARG_TYPE> { using type = TYPE_AT_RUNTIME; };

#define COMPTIME_ARG_TYPE(ARG_TYPE, TYPE_AT_COMPILETIME) \
template <> \
struct CompiletimeTypeLookup<ARG_TYPE> { using type = TYPE_AT_COMPILETIME; };


template<ArgType>
struct RuntimeTypeLookup {
    using type = void;
};

template<ArgType>
struct CompiletimeTypeLookup {
    using type = void;
};

RUNTIME_ARG_TYPE(ArgType::_TYPE, Variant::Type)
RUNTIME_ARG_TYPE(ArgType::_VARIANT, VariantAddress)
RUNTIME_ARG_TYPE(ArgType::_OPERATOR, Variant::Operator)
RUNTIME_ARG_TYPE(ArgType::_SIGNATURE, Signature)
RUNTIME_ARG_TYPE(ArgType::_INT, int)
RUNTIME_ARG_TYPE(ArgType::_OPERATOR_EVAL, EncodedPointer<Variant::ValidatedOperatorEvaluator>)


#define CASE_SIZE(OP)                  \
    case GDScriptFunction::Opcode::OP: \
        return Opcode<GDScriptFunction::Opcode::OP>::from_instr_ptr(opcode_ptr, 0).size()

template<GDScriptFunction::Opcode CODE>
class BaseOpcode;

template<GDScriptFunction::Opcode CODE>
class StaticOpcode;

template<GDScriptFunction::Opcode CODE>
class StaticVarargOpcode;

struct TypeInvariantStatus {


    enum Status {
        // indicates a variant is guaranteed to have a certain type at this point in execution
        KNOWN,
        // indicates a variant's type can no longer be guaranteed at this point in execution
        UNKNOWN,
        // indicates no changes were made to the type guarantees of a variant
        UNCHANGED,
        // indicates that a variant has been assigned to equal another, and thus must now have the same type
        DEPENDENT,
    } status = Status::UNKNOWN;

    VariantAddress dependency = VariantAddress(0);

    TypeInvariantStatus(): TypeInvariantStatus(Status::UNKNOWN) { }
    TypeInvariantStatus(TypeInvariantStatus::Status _status): status(_status) { }
    TypeInvariantStatus(TypeInvariantStatus::Status _status, VariantAddress address): status(_status) { }
};

class DynOpcode {
protected:
    DynOpcode(GDScriptFunction *_fn, int *_opcode_ptr) : fn(_fn), opcodes(_fn->_code_ptr), opcode_ptr(_opcode_ptr) {}

    GDScriptFunction *fn;
    int *opcodes;
    int *opcode_ptr;

public:
    static std::unique_ptr<DynOpcode> from_opcode_ptr(GDScriptFunction *_fn, int *opcode_ptr);

    inline GDScriptFunction::Opcode get_opcode() {
        return *reinterpret_cast<GDScriptFunction::Opcode *>(opcode_ptr);
    }

    inline int index() {
        return (*opcode_ptr - *opcodes) / sizeof(int);
    }

    /// returns the dynamic opcode representing the next sequential opcode in the function
    inline std::unique_ptr<DynOpcode> next() {
        if (get_opcode() == GDScriptFunction::Opcode::OPCODE_END) {
            return std::unique_ptr<DynOpcode>(nullptr);
        }
        int *next_ptr = &opcode_ptr[this->get_size()];
        return from_opcode_ptr(fn, next_ptr);
    }

    inline bool is_branching() {
        return get_jump_target_a() != -1 || get_jump_target_b() != -1;
    }

    /// returns the dynamic opcode representing the 'true' branch path, if one exists
    inline std::unique_ptr<DynOpcode> logical_next_truthy() {

        if (get_jump_target_a() != -1) {
            int *next_ptr = &opcodes[get_jump_target_a()];
            return from_opcode_ptr(fn, next_ptr);
        } else {
            return next();
        }
    }

    /// returns the dynamic opcode representing the 'false' branch path, if one exists
    inline std::unique_ptr<DynOpcode> logical_next_falsy() {
        if (get_jump_target_b() != -1) {
            int *next_ptr = &opcodes[get_jump_target_b()];
            return from_opcode_ptr(fn, next_ptr);
        } else {
            return next();
        }
    }

    virtual Vector<TypeInvariantStatus> type_invariant_statuses() {
        thread_local Vector<TypeInvariantStatus> unchanged = Vector<TypeInvariantStatus>({ TypeInvariantStatus() });
        unchanged.write[0] = TypeInvariantStatus();
        return unchanged;
    }

    virtual Vector<Variant::Type> type_invariants() {
        return Vector<Variant::Type>();
    }

    /// returns the dynamic opcode representing the next sequential opcode in the function
    virtual bool has_logical_next() {
        return true;
    }

    virtual inline int get_size() = 0;

    virtual inline const Vector<VariantAddress *> _variant_addresses() {
        static const auto dummy = Vector<VariantAddress *>();
        return dummy;
    }

    inline void _foreach_variant_idx(const GDScriptFunction::Address address_type, void (*idx_consumer)(int &)) {
        for (auto address: this->_variant_addresses()) {
            if (address_type == address->get_address_type()) {
                int type_bitmap = address->address_data & GDScriptFunction::Address::ADDR_TYPE_MASK;
                int updated_address_data = address->get_address_index();
                idx_consumer(updated_address_data);
                address->address_data = type_bitmap | updated_address_data;
            }
        }
    }

    inline void foreach_stack_idx(void (*idx_consumer)(int &)) {
        _foreach_variant_idx(GDScriptFunction::Address::ADDR_TYPE_STACK, idx_consumer);
    }

    inline void foreach_constant_idx(void (*idx_consumer)(int &)) {
        _foreach_variant_idx(GDScriptFunction::Address::ADDR_TYPE_CONSTANT, idx_consumer);
    }

    inline void foreach_member_idx(void (*idx_consumer)(int &)) {
        _foreach_variant_idx(GDScriptFunction::Address::ADDR_TYPE_MEMBER, idx_consumer);
    }

#define NEG_ONE_REF \
    thread_local int temp = -1; \
    temp = -1; \
    return temp;

    inline bool has_jump_target() const {
        return get_jump_target_a() != -1;
    }

    virtual inline int &get_jump_target_a() const { NEG_ONE_REF }
    
    virtual inline int &get_jump_target_b() const { NEG_ONE_REF }
    
    virtual inline int &get_lambda_idx() { NEG_ONE_REF }

    virtual inline int &get_global_name_idx() { NEG_ONE_REF }

    virtual inline int &get_keyed_setter_idx() { NEG_ONE_REF }

    virtual inline int &get_keyed_getter_idx() { NEG_ONE_REF }

    virtual inline int &get_method_idx() { NEG_ONE_REF }

    virtual inline int &get_operator_func_idx() { NEG_ONE_REF }

    virtual inline int &get_default_arg_idx() { NEG_ONE_REF }

    virtual inline int &get_builtin_method_idx() { NEG_ONE_REF }

    virtual inline int &get_setter_idx() { NEG_ONE_REF }

    virtual inline int &get_getter_idx() { NEG_ONE_REF }

    virtual inline int &get_constructor_idx() { NEG_ONE_REF }

    virtual inline int &get_utilities_idx() { NEG_ONE_REF }

    virtual inline int &get_gds_utilities_idx() { NEG_ONE_REF }

    virtual inline int &get_indexed_setter_idx() { NEG_ONE_REF }

    virtual inline int &get_indexed_getter_idx() { NEG_ONE_REF }

    virtual ~DynOpcode() {}
};

class DynVarargOpcode: public DynOpcode {
public:
    DynVarargOpcode(GDScriptFunction *_fn, int *_opcode_ptr) : DynOpcode(_fn, _opcode_ptr) {}

    virtual inline int get_arg_count() {
        // the value after the code is always the instr arg count for vararg ops
        return this->opcode_ptr[1];
    }

    virtual int get_size() {
        // opcode + instr arg count + 1 per arg
        return 1 + 1 + this->get_arg_count();
    }

    virtual VariantAddress *get_instr_args_ptr() {
        return &reinterpret_cast<VariantAddress *>(this->opcode_ptr)[1];
    }

    /// the pointer to the first value after the instruction args
    virtual int *get_static_arg_ptr() {
        return &this->opcode_ptr[get_size()];
    }

    virtual inline const Vector<VariantAddress*> _variant_addresses() final {
        const Vector<VariantAddress*>::Size size = get_arg_count();
        VariantAddress *instr_args_ptr = this->get_instr_args_ptr();
        thread_local Vector<VariantAddress*> static_addresses = Vector<VariantAddress*>();

        if (size < static_addresses.size()) {
            static_addresses.resize(size);
        }

        for (int i = 0; i < size; ++i) {
            static_addresses.write[i] = &instr_args_ptr[i];
        }
        return static_addresses;
    }
};


template<GDScriptFunction::Opcode CODE>
class BaseOpcode {
public:
    const GDScriptFunction::Opcode code = CODE;
    using THIS = StaticOpcode<CODE>;

    static inline THIS &from_instr_ptr(int *code_ptr, int instr_ptr) {
        int &opcode_addr = code_ptr[instr_ptr];
        return reinterpret_cast<THIS &>(opcode_addr);
    };

    static inline THIS *from_opcode_ptr(int *code_ptr) {
        return reinterpret_cast<THIS *>(code_ptr);
    };

    constexpr inline int size() const {
        return sizeof(THIS);
    };
};


template<GDScriptFunction::Opcode CODE>
class StaticVarargOpcode {
public:
    const GDScriptFunction::Opcode code = CODE;
    const int arg_count = 0;
    using THIS = StaticOpcode<CODE>;

    static inline THIS &from_instr_ptr(int *code_ptr, int instr_ptr) {
        int &opcode_addr = code_ptr[instr_ptr];
        return reinterpret_cast<THIS &>(opcode_addr);
    };

    static inline THIS *from_opcode_ptr(int *code_ptr) {
        return reinterpret_cast<THIS *>(code_ptr);
    };

    template<typename TYPE>
    inline TYPE &get_static_arg_by_offset(int offset) {
        return reinterpret_cast<TYPE*>(this)[1 + arg_count + 1 + offset];
    };

    /// the combines size of the opcode, arg count, and args
    constexpr inline int vararg_header_size() const {
        return sizeof(THIS) + arg_count;
    };
};

#define STATIC_OP(OP) \
    template <>       \
    class StaticOpcode<OP> : public BaseOpcode<OP>

#define VARARG_OP(OP) \
    template <>       \
    class StaticOpcode<OP> : public StaticVarargOpcode<OP>

#define ARGS(...) \
    virtual inline const Vector<VariantAddress*> _variant_addresses() final {              \
        VariantAddress *addresses[] = { __VA_ARGS__ };                                         \
        Vector<VariantAddress*>::Size size = std::size(addresses); \
        thread_local Vector<VariantAddress*> static_addresses = Vector<VariantAddress*>(); \
        if (static_addresses.size() < size) {                                      \
            static_addresses.resize(size); \
        } \
        for (int i = 0; i < size; ++i) { \
            static_addresses.write[i] = addresses[i]; \
        } \
        return static_addresses; \
    }

#define DYN_HEADER \
    public:        \
    Dyn(GDScriptFunction *_fn, int *_opcode_ptr) : DynOpcode(_fn, _opcode_ptr) {} \
    THIS *opcode = THIS::from_opcode_ptr(opcode_ptr);

#define DYN_VARARG_HEADER \
    public:                      \
    Dyn(GDScriptFunction *_fn, int *_opcode_ptr) : DynVarargOpcode(_fn, _opcode_ptr) {} \
    THIS *opcode = THIS::from_opcode_ptr(opcode_ptr);

STATIC_OP(GDScriptFunction::Opcode::OPCODE_OPERATOR)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type left;
    RuntimeTypeLookup<ArgType::_VARIANT>::type right;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_OPERATOR>::type operation;
    RuntimeTypeLookup<ArgType::_SIGNATURE>::type signature;
    RuntimeTypeLookup<ArgType::_TYPE>::type return_type;
    RuntimeTypeLookup<ArgType::_OPERATOR_EVAL>::type operator_fn;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(&opcode->left, &opcode->right, &opcode->dst)
    };
};


STATIC_OP(GDScriptFunction::Opcode::OPCODE_OPERATOR_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type left;
    RuntimeTypeLookup<ArgType::_VARIANT>::type right;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int operator_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual inline int &get_operator_func_idx() final { return opcode->operator_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->left,
                &opcode->right,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_TYPE_TEST_BUILTIN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    RuntimeTypeLookup<ArgType::_TYPE>::type type;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_TYPE_TEST_ARRAY)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    RuntimeTypeLookup<ArgType::_VARIANT>::type script_type;
    RuntimeTypeLookup<ArgType::_TYPE>::type builtin_type;
    int native_type_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual inline int &get_global_name_idx() final { return opcode->native_type_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->value,
                &opcode->script_type,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_TYPE_TEST_NATIVE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    int native_type_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_global_name_idx() final { return opcode->native_type_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_TYPE_TEST_SCRIPT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    RuntimeTypeLookup<ArgType::_VARIANT>::type type_object;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }


        ARGS(
                &opcode->dst,
                &opcode->value,
                &opcode->type_object,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_KEYED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type index;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->index,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_KEYED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type index;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    int setter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_setter_idx() final { return opcode->setter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->index,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_INDEXED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type index;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    int setter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_setter_idx() final { return opcode->setter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->index,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_KEYED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type index;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
                &opcode->index,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_KEYED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type key;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int getter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_getter_idx() final { return opcode->getter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
                &opcode->key,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_INDEXED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type index;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int getter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_getter_idx() final { return opcode->getter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
                &opcode->index,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_NAMED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    int name_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_global_name_idx() final { return opcode->name_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_NAMED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    int setter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_setter_idx() final { return opcode->setter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->value,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_NAMED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int name_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_global_name_idx() final { return opcode->name_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_NAMED_VALIDATED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int getter_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_getter_idx() final { return opcode->getter_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_MEMBER)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    int name_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_global_name_idx() final { return opcode->name_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->src,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_MEMBER)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int name_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        inline int &get_global_name_idx() final { return opcode->name_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_SET_STATIC_VARIABLE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type value;
    RuntimeTypeLookup<ArgType::_VARIANT>::type _class;
    int static_var_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->value,
                &opcode->_class,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_GET_STATIC_VARIABLE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type target;
    RuntimeTypeLookup<ArgType::_VARIANT>::type _class;
    int static_var_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->target,
                &opcode->_class,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_NULL)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_TYPE>::type var_type;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_ARRAY)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type script_type;
    RuntimeTypeLookup<ArgType::_TYPE>::type builtin_type;
    int native_type_idx;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual inline int &get_global_name_idx() final { return opcode->native_type_idx; }

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
                &opcode->script_type,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type type_object;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
                &opcode->type_object,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_TYPE>::type to_type;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type to_type_object;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
                &opcode->to_type_object,
        )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type to_type_object;

    class Dyn : public DynOpcode {
        DYN_HEADER
        
        virtual int get_size() override { return opcode->size(); }

        ARGS(
                &opcode->dst,
                &opcode->src,
                &opcode->to_type_object,
        )
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CONSTRUCT)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline RuntimeTypeLookup<ArgType::_TYPE>::type get_type() {
        return get_static_arg_by_offset<RuntimeTypeLookup<ArgType::_TYPE>::type>(1);
    }

    inline int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CONSTRUCT_VALIDATED)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_ctor_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    inline int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        virtual inline int &get_constructor_idx() override final {
            return opcode->get_ctor_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY)
{
public:
    int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    int size() const {
        return vararg_header_size() + 1;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CONSTRUCT_TYPED_ARRAY)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline RuntimeTypeLookup<ArgType::_TYPE>::type get_type() {
        return get_static_arg_by_offset<RuntimeTypeLookup<ArgType::_TYPE>::type>(1);
    }

    inline int &get_native_type_idx() {
        return get_static_arg_by_offset<int>(2);
    }

    int size() const {
        return vararg_header_size() + 3;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_native_type_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    int size() const {
        return vararg_header_size() + 1;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_ASYNC)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_method_name_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_RETURN)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_method_name_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_method_name_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_RET)
{
public:
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_STATIC)
{
public:
    
    inline RuntimeTypeLookup<ArgType::_TYPE>::type get_type() {
        return get_static_arg_by_offset<RuntimeTypeLookup<ArgType::_TYPE>::type>(0);
    }

    inline int &get_method_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }
    
    inline int &get_argc() {
        return get_static_arg_by_offset<int>(2);
    }

    int size() const {
        return vararg_header_size() + 3;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_method_name_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC)
{
public:

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_method_idx() override {
            return opcode->get_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_TYPE_VALIDATED)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_builtin_method_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_builtin_method_idx() override {
            return opcode->get_builtin_method_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_UTILITY)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_global_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_global_name_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_UTILITY_VALIDATED)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_utilities_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_utilities_idx() override {
            return opcode->get_utilities_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_GDSCRIPT_UTILITY)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_utilities_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_utilities_idx() override {
            return opcode->get_utilities_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE)
{
public:

    inline int &get_argc() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_global_name_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_global_name_idx() override {
            return opcode->get_global_name_idx();
        }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_AWAIT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type arg_obj;

    class Dyn : public DynOpcode {
        DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS( &opcode->arg_obj )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_AWAIT_RESUME)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type result;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS( &opcode->result )
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CREATE_LAMBDA)
{
public:

    inline int &get_captures_count() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_lambda_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_lambda_idx() override {
            return opcode->get_lambda_idx();
        }
    };
};

VARARG_OP(GDScriptFunction::Opcode::OPCODE_CREATE_SELF_LAMBDA)
{
public:

    inline int &get_captures_count() {
        return get_static_arg_by_offset<int>(0);
    }

    inline int &get_lambda_idx() {
        return get_static_arg_by_offset<int>(1);
    }

    int size() const {
        return vararg_header_size() + 2;
    }

    class Dyn : public DynVarargOpcode {
        DYN_VARARG_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_lambda_idx() override {
            return opcode->get_lambda_idx();
        }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_JUMP)
{
public:
    int to;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_jump_target_a() const override {
            return opcode->to;
        }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_JUMP_IF)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type test;
    int if_true;
    int if_false;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_jump_target_a() const override {
            return opcode->if_true;
        }

        int &get_jump_target_b() const override {
            return opcode->if_false;
        }
        
        ARGS( &opcode->test )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type test;
    int if_false;
    int if_true;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_jump_target_a() const override {
            return opcode->if_true;
        }

        int &get_jump_target_b() const override {
            return opcode->if_false;
        }

        ARGS( &opcode->test )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT)
{
public:

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        // TODO: figure out how to get the defargs value for the jump target here
//        int &get_jump_target_a() const override {
//            return opcode->if_true;
//        }

//        int &get_jump_target_b() const override {
//            return opcode->if_true;
//        }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_JUMP_IF_SHARED)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type val;
    int if_shared;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        int &get_jump_target_a() const override {
            return opcode->if_shared;
        }

        ARGS( &opcode->val )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_RETURN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type ret_val;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        bool has_logical_next() override { return false; }

        ARGS( &opcode->ret_val )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_BUILTIN)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type ret_val;
    RuntimeTypeLookup<ArgType::_TYPE>::type ret_type;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        bool has_logical_next() override { return false; }

        ARGS( &opcode->ret_val )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_ARRAY)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type ret_val;
    RuntimeTypeLookup<ArgType::_VARIANT>::type script_type;
    RuntimeTypeLookup<ArgType::_TYPE>::type builtin_type;
    int native_type_idx;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        bool has_logical_next() override { return false; }

        int &get_global_name_idx() override {
            return opcode->native_type_idx;
        }

        ARGS( &opcode->ret_val, &opcode->script_type )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_NATIVE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type ret_val;
    RuntimeTypeLookup<ArgType::_VARIANT>::type type;
    int native_type_idx;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        bool has_logical_next() override { return false; }

        ARGS( &opcode->ret_val, &opcode->type )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_SCRIPT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type ret_val;
    RuntimeTypeLookup<ArgType::_VARIANT>::type type;
    int native_type_idx;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        bool has_logical_next() override { return false; }

        ARGS( &opcode->ret_val, &opcode->type )
    };
};

#define STATIC_OP_ITERATE(OP)\
STATIC_OP(OP)\
{\
public:\
    RuntimeTypeLookup<ArgType::_VARIANT>::type counter;\
    RuntimeTypeLookup<ArgType::_VARIANT>::type container;\
    RuntimeTypeLookup<ArgType::_VARIANT>::type iterator;\
    int loop_end_target;\
\
    class Dyn : public DynOpcode {\
    DYN_HEADER\
\
        virtual int get_size() override { return opcode->size(); }\
\
        int & get_jump_target_b() const override {\
            return opcode->loop_end_target;\
        }\
\
        ARGS( &opcode->counter, &opcode->container, &opcode->iterator )\
    };\
};

STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_INT)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_FLOAT)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2I)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3I)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_STRING)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_DICTIONARY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_OBJECT)

STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_INT)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_FLOAT)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2I)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3I)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_STRING)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_DICTIONARY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_BYTE_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT32_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT64_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT32_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT64_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_STRING_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR2_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR3_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_COLOR_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR4_ARRAY)
STATIC_OP_ITERATE(GDScriptFunction::Opcode::OPCODE_ITERATE_OBJECT)

STATIC_OP(GDScriptFunction::Opcode::OPCODE_STORE_GLOBAL)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int global_idx;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS( &opcode->dst )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_STORE_NAMED_GLOBAL)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    int global_name_idx;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        int & get_global_name_idx() override {
            TypeInvariantStatus(TypeInvariantStatus::KNOWN, VariantAddress(0));
            return opcode->global_name_idx;
        }

        ARGS( &opcode->dst )
    };
};

#define STATIC_TYPE_ADJUST_OP(OP, TO_TYPE)\
STATIC_OP(OP)\
{\
public:\
    RuntimeTypeLookup<ArgType::_VARIANT>::type arg;\
\
    class Dyn : public DynOpcode {\
    DYN_HEADER\
\
        virtual int get_size() override { return opcode->size(); }\
\
        Vector<TypeInvariantStatus> type_invariant_statuses() override {\
            return Vector<TypeInvariantStatus>({ TypeInvariantStatus::KNOWN });\
        }                                 \
                                          \
        Vector<Variant::Type> type_invariants() override {\
             return Vector<Variant::Type>({ TO_TYPE });\
        }\
        ARGS( &opcode->arg )\
    };\
};

STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BOOL, Variant::Type::BOOL)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_INT, Variant::Type::INT)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_FLOAT, Variant::Type::FLOAT)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING, Variant::Type::STRING)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2, Variant::Type::VECTOR2)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2I, Variant::Type::VECTOR2I)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2, Variant::Type::RECT2)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2I, Variant::Type::RECT2I)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3, Variant::Type::VECTOR3)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3I, Variant::Type::VECTOR3I)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM2D, Variant::Type::TRANSFORM2D)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4, Variant::Type::VECTOR4)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4I, Variant::Type::VECTOR4I)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PLANE, Variant::Type::PLANE)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_QUATERNION, Variant::Type::QUATERNION)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_AABB, Variant::Type::AABB)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BASIS, Variant::Type::BASIS)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM3D, Variant::Type::TRANSFORM3D)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PROJECTION, Variant::Type::PROJECTION)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_COLOR, Variant::Type::COLOR)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING_NAME, Variant::Type::STRING_NAME)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_NODE_PATH, Variant::Type::NODE_PATH)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RID, Variant::Type::RID)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_OBJECT, Variant::Type::OBJECT)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_CALLABLE, Variant::Type::CALLABLE)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_SIGNAL, Variant::Type::SIGNAL)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_DICTIONARY, Variant::Type::DICTIONARY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_ARRAY, Variant::Type::ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY, Variant::Type::PACKED_BYTE_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY, Variant::Type::PACKED_INT32_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY, Variant::Type::PACKED_INT64_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY, Variant::Type::PACKED_FLOAT32_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY, Variant::Type::PACKED_FLOAT64_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY, Variant::Type::PACKED_STRING_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY, Variant::Type::PACKED_VECTOR2_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY, Variant::Type::PACKED_VECTOR3_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY, Variant::Type::PACKED_COLOR_ARRAY)
STATIC_TYPE_ADJUST_OP(GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY, Variant::Type::PACKED_VECTOR4_ARRAY)

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSERT)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type test;
    RuntimeTypeLookup<ArgType::_VARIANT>::type message;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }

        ARGS( &opcode->test, &opcode->message )
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_BREAKPOINT)
{
public:
    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_LINE)
{
public:
    int line;

    class Dyn : public DynOpcode {
    DYN_HEADER

        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_END)
{
public:
    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};

/*
STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_FROM_PRIMITIVE)
{
public:
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    RuntimeTypeLookup<ArgType::_TYPE>::type primitive_type;

    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_TO_PRIMITIVE)
{
public:
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_ASSIGN_FROM_TO_PRIMITIVE)
{
public:
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type src;
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;

    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_PRIMITIVE_UNARY_OP)
{
public:
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type val;
    // encoded primitive
    Variant::Operator unary_op;

    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};

STATIC_OP(GDScriptFunction::Opcode::OPCODE_PRIMITIVE_INT_BINARY_OP)
{
public:
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type lhs;
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type rhs;
    // encoded primitive
    RuntimeTypeLookup<ArgType::_VARIANT>::type dst;
    Variant::Operator binary_op;

    class Dyn : public DynOpcode {
    DYN_HEADER
        virtual int get_size() override { return opcode->size(); }
    };
};
*/

#endif // GDSCRIPT_OPCODES_H
