//
// Created by jam on 11/09/2024.
//

#ifndef SLJIT2_MEM_H
#define SLJIT2_MEM_H

#include "thirdparty/sljit/src/sljitLir.h"

#include "core/error/error_macros.h"
#include "jit_types.h"
#include "constructed_type.h"
#include <unordered_map>

using namespace jt;

struct Register {
    static const Register R0;
    static const Register R1;
    static const Register FR0;
    static const Register FR1;

    sljit2_s32 arg_1;

    Register(sljit2_s32 _arg_1): arg_1(_arg_1) { }

    operator sljit2_s32() const {
        return arg_1;
    }
};

template<typename F>
FieldReference _field(int size, int stack_offset, const void *struct_addr, const void *field_addr, bool _is_float) {
    static_assert(false, "Field type not supported");
}

template<typename T>
struct LocalVar {
    // shared dummy instance just used to get the address of fields
    static const T instance;
    int stack_offset;
    bool is_float;

    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }

    T const &operator *() const {
        return instance;
    }

    template<typename F>
    FieldReference field(const F &field) const {
        return _field<F>();
    }
};

template<>
struct LocalVar<float_jt> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0, true); }
};

template<>
struct LocalVar<int_jt> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0); }
};






template<>
struct LocalVar<float_jt*> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0, true); }
};

template<typename T>
struct LocalVar<T*> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0); }
};

template<const int LEN>
struct LocalVar<float_jt[LEN]> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0, true); }
    FieldReference index(int idx) {
        return FieldReference(stack_offset, idx * sizeof(float_jt));
    }
};

template<typename T, const int LEN>
struct LocalVar<T[LEN]> {
    int stack_offset;
    LocalVar(int _stack_offset): stack_offset(_stack_offset) { }
    operator FieldReference() { return FieldReference(stack_offset, 0); }
    FieldReference index(int idx) {
        return FieldReference(stack_offset, idx * sizeof(T));
    }
};

struct Mem {
    static std::unordered_map<float_jt, int_jt> float_const_registers;

    static const Mem MR0;
    static const Mem MR1;
    static const Mem MFR0;
    static const Mem MFR1;
    static const Mem MTRUE;
    static const Mem MFALSE;

    int flags = 0;
    sljit2_s32 arg_1 = 0;
    int_jt arg_2 = 0;

    static const int IS_ARG = 0b0001;
    static const int IS_FLOAT = 0b0010;
    static const int IS_LOCAL = 0b0100;

    Mem(Register reg): arg_1(reg.arg_1) { }
    Mem(FieldReference field): arg_1() { }
    Mem(sljit2_s32 _arg_1, int_jt _arg_2): arg_1(_arg_1), arg_2(_arg_2) { }
    Mem(sljit2_s32 _arg_1, int_jt _arg_2, int _flags):
        flags(_flags),
        arg_1(_arg_1),
        arg_2(_arg_2) { }

public:

    static Mem reg(int reg_number) {
        return Mem {SLJIT2_R(reg_number), 0 };
    }

    static Mem float_reg(int reg_number) {
        return Mem {SLJIT2_FR(reg_number), 0 };
    }

    static Mem arg(int arg_index) {
        return Mem {SLJIT2_S(arg_index), 0, IS_ARG };
    }

    static Mem float_arg(int arg_index) {
        return Mem {SLJIT2_FS(arg_index), 0, IS_ARG | IS_FLOAT };
    }

    static Mem integral_const(int_jt const_val) {
        return Mem { SLJIT2_IMM, const_val };
    }

    static Mem static_address(void* address) {
        return Mem { SLJIT2_MEM0(), reinterpret_cast<int_jt>(address) };
    }

    static Mem address_offset(Register address_register, int_jt byte_offset) {
        return Mem { SLJIT2_MEM1(address_register), byte_offset };
    }

    static Mem array_index(Register address_register, Register index_register, int_jt size) {
        return Mem { SLJIT2_MEM2(address_register, index_register), size };
    }

    template<typename T>
    static Mem local_variable() {
        return Mem { sizeof(T), -1, IS_LOCAL };
    }

    template<typename ARRAY_TYPE>
    static Mem array_index(Register address_register, Register index_register) {
        return Mem { SLJIT2_MEM2(address_register, index_register), sizeof(ARRAY_TYPE) };
    }

    bool is_arg() const {
        return (flags & IS_ARG) != 0;
    }

    bool is_float() const {
        return (flags & IS_FLOAT) != 0;
    }

    bool is_local() const {
        return (flags & IS_LOCAL) != 0;
    }

    bool is_register() const {
        return arg_2 == 0
            && (arg_1 >= SLJIT2_R0 && arg_1 < SLJIT2_R(SLJIT2_NUMBER_OF_REGISTERS));
    }

    bool is_float_register() const {
        return arg_2 == 0
            && is_float()
            && (arg_1 >= SLJIT2_FR0 && arg_1 < SLJIT2_FR(SLJIT2_NUMBER_OF_FLOAT_REGISTERS));
    }

    bool is_call_arg() const {
        return is_arg() && is_register();
    }

    bool is_float_call_arg() const {
        return is_arg() && is_float_register();
    }

    bool is_local_address() const {
        return is_local();
    }

    bool is_uninit_local_address() const {
        return is_local_address() && arg_2 == -1;
    }

    sljit2_s32 get_uninit_local_size() const {
        return arg_1;
    }

    void initialize_local(int stack_offset) {
        arg_1 = SLJIT2_MEM1(SLJIT2_SP);
        arg_2 = stack_offset;
    }

    bool operator==(const Mem &other) const {
        return arg_1 == other.arg_1
               && arg_2 == other.arg_2;
    }
    bool operator!=(const Mem &other) const {
        return !(*this == other);
    }
};




#endif //SLJIT2_MEM_H
