//
// Created by jam on 11/09/2024.
//

#ifndef SLJIT_MEM_H
#define SLJIT_MEM_H


#include "thirdparty/sljit/src/sljitLir.h"
#include "jit_types.h"
#include "constructed_type.h"
#include <unordered_map>

using namespace jt;

struct Register {
    static const Register R0;
    static const Register R1;
    static const Register FR0;

    sljit_s32 arg_1;

    Register(sljit_s32 _arg_1): arg_1(_arg_1) { }

    operator sljit_s32() const {
        return arg_1;
    }
};
const Register Register::R0 = Register { SLJIT_R0 };
const Register Register::R1 = Register { SLJIT_R1 };
const Register Register::FR0 = Register { SLJIT_FR0 };

template<typename STRUCT>
struct LocalVar {
    int stack_offset = 0;
    ConstructedType<STRUCT> constructed_type;

    LocalVar(int offset): stack_offset(offset), constructed_type(ConstructedType<STRUCT>()) {

    }

    STRUCT operator *() {
        return constructed_type.data;
    }
};

struct Mem {
    static std::unordered_map<float_jt, int_jt> float_const_registers;

    static const Mem MR0;
    static const Mem MR1;
    static const Mem MFR0;
    static const Mem MTRUE;
    static const Mem MFALSE;

    sljit_s32 arg_1 = 0;
    int_jt arg_2 = 0;
    bool is_arg = false;

    Mem(Register reg): arg_1(reg.arg_1) { }
    Mem(sljit_s32 _arg_1, int_jt _arg_2): arg_1(_arg_1), arg_2(_arg_2) { }
    Mem(sljit_s32 _arg_1, int_jt _arg_2, bool _is_arg): arg_1(_arg_1), arg_2(_arg_2), is_arg(_is_arg) { }

public:

    static Mem reg(int reg_number) {
        return Mem {SLJIT_R(reg_number), 0 };
    }

    static Mem float_reg(int reg_number) {
        return Mem {SLJIT_FR(reg_number), 0 };
    }

    static Mem arg(int arg_index) {
        return Mem {SLJIT_S(arg_index), 0, true };
    }

    static Mem float_arg(int arg_index) {
        return Mem {SLJIT_FS(arg_index), 0, true };
    }

    static Mem integral_const(int_jt const_val) {
        return Mem { SLJIT_IMM, const_val };
    }

    static Mem static_address(void* address) {
        return Mem { SLJIT_MEM0(), reinterpret_cast<int_jt>(address) };
    }

    static Mem address_offset(Register address_register, int_jt byte_offset) {
        return Mem { SLJIT_MEM1(address_register), byte_offset };
    }

    static Mem array_index(Register address_register, Register index_register, int_jt size) {
        return Mem { SLJIT_MEM2(address_register, index_register), size };
    }

    static Mem local_variable(int stack_offset) {
        return Mem { SLJIT_MEM1(SLJIT_SP), stack_offset };
    }

    template<typename ARRAY_TYPE>
    static Mem array_index(Register address_register, Register index_register) {
        return Mem { SLJIT_MEM2(address_register, index_register), sizeof(ARRAY_TYPE) };
    }

    bool is_register() const {
        return arg_2 == 0 && (arg_1 >= SLJIT_R0 && arg_1 < SLJIT_R(SLJIT_NUMBER_OF_REGISTERS));
    }

    bool is_float_register() const {
        return arg_2 == 0 && (arg_1 >= SLJIT_FR0 && arg_1 < SLJIT_FR(SLJIT_NUMBER_OF_FLOAT_REGISTERS));
    }

    bool is_call_arg() const {
        return is_arg && is_register();
    }

    bool is_float_call_arg() const {
        return is_arg && is_float_register();
    }

    bool operator==(const Mem &other) const {
        return arg_1 == other.arg_1
               && arg_2 == other.arg_2;
    }
    bool operator!=(const Mem &other) const {
        return !(*this == other);
    }
};
const Mem Mem::MTRUE = Mem { SLJIT_IMM, 1 };
const Mem Mem::MFALSE = Mem { SLJIT_IMM, 0 };
const Mem Mem::MR0 = Mem { Register::R0 };
const Mem Mem::MR1 = Mem { Register::R1 };
const Mem Mem::MFR0 = Mem { Register::FR0 };



#endif //SLJIT_MEM_H
