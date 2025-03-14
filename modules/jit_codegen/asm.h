//
// Created by jam on 11/09/2024.
//

#ifndef SLJIT_ASM_H
#define SLJIT_ASM_H

#include <vector>
#include <functional>
#include <memory>
#include <unordered_set>
#include "mem.h"

using namespace jt;

enum AbiArgType {
    TYPE_VOID = SLJIT_ARG_TYPE_RET_VOID,
    TYPE_WORD = SLJIT_ARG_TO_TYPE(W),
    TYPE_POINTER = SLJIT_ARG_TO_TYPE(P),
    TYPE_FLOAT = SLJIT_ARG_TO_TYPE(F64),
};

template<typename T>
struct ArgTypeOf {
    static const AbiArgType value;
};

template<>
struct ArgTypeOf<void> { static const AbiArgType value = AbiArgType::TYPE_VOID; };
template<>
struct ArgTypeOf<int_jt> { static const AbiArgType value = AbiArgType::TYPE_WORD; };
template<typename T>
struct ArgTypeOf<T*> { static const AbiArgType value = AbiArgType::TYPE_POINTER; };
template<>
struct ArgTypeOf<float_jt> { static const AbiArgType value = AbiArgType::TYPE_FLOAT; };

struct Asm {
    sljit_compiler *compiler;
    std::unordered_set<sljit_s32> initialized_floats;
    std::vector<std::function<void()>> inits;
    std::vector<std::function<void()>> instructions;
    int args = 0;
    int float_args = 0;
    int min_registers = 0;
    int min_float_registers = 0;
    int stack_size = 0;

    Asm() {
        compiler = sljit_create_compiler(nullptr);
    }

    void write_init(const std::function<void()> &instr) {
        inits.push_back(instr);
    }

    void write_instr(const std::function<void()> &instr) {
        instructions.push_back(instr);
    }

    void require_memory(std::initializer_list<Mem> registers) {
        for (Mem reg: registers) {
            if (reg.is_arg) {
                if (reg.is_call_arg()) {
                    args = std::max(args, reg.arg_1);
                } else if (reg.is_float_call_arg() && !*initialized_floats.find(reg.arg_1)) {
                    initialized_floats.insert(reg.arg_1);
                    // copy our float reg to the associated save reg so we can freely use it as temp memory
                    write_init([=]() {
                        sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_FS(reg.arg_1), 0, SLJIT_FR(reg.arg_1), 0);
                    });
                    float_args = std::max(args, reg.arg_1);
                }
            } else {
                if (reg.is_register()) {
                    min_registers = std::max(min_registers, reg.arg_1);
                } else if (reg.is_float_register()) {
                    min_float_registers = std::max(min_float_registers, reg.arg_1);
                }
            }
        }
    }

public:

    template<typename R>
    std::function<R(void *)> compile_array_args() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<sljit_up>::value << 4;
        return reinterpret_cast<R (*)()>(_compile(arg_types));
    }

    template<typename R>
    std::function<R()> compile() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        return reinterpret_cast<R (*)()>(_compile(arg_types));
    }

    template<typename R, typename A1>
    std::function<R(A1)> compile() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        return reinterpret_cast<R (*)(A1)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2>
    std::function<R(A1, A2)> compile() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        return reinterpret_cast<R (*)(A1, A2)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2, typename A3>
    std::function<R(A1, A2, A3)> compile() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        arg_types |= ArgTypeOf<A3>::value << 12;
        return reinterpret_cast<R (*)(A1, A2, A3)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2, typename A3, typename A4>
    std::function<R(A1, A2, A3, A4)> compile() {
        sljit_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        arg_types |= ArgTypeOf<A3>::value << 12;
        arg_types |= ArgTypeOf<A4>::value << 16;
        return reinterpret_cast<R (*)(A1, A2, A3, A4)>(_compile(arg_types));
    }

    void *_compile(sljit_s32 arg_types) {
        sljit_emit_enter(
            compiler, 0, arg_types,
            min_registers | SLJIT_ENTER_FLOAT(std::max(min_float_registers, float_args)),
            args | SLJIT_ENTER_FLOAT(float_args),
            stack_size
        );
        for (const auto& init: inits) {
            init();
        }
        for (const auto& instr: instructions) {
            instr();
        }

        return sljit_generate_code(compiler, 0, nullptr);
    }

    void move(Mem src, Mem dst) {
        require_memory({ src, dst });
        if (src != dst) {
            write_instr([=]() {
                sljit_emit_op1(compiler, SLJIT_MOV, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
            });
        }
    }

    void load_constf(float_jt value) {
        require_memory({ Mem::MFR0 });
        write_instr([=]() {
            sljit_emit_fset64(compiler, SLJIT_FR0, value);
        });
    }

    void set_constf(Mem dst, float_jt value) {
        require_memory({ dst });
        if (dst.is_float_register()) {
            write_instr([=]() {
                sljit_emit_fset64(compiler, dst.arg_1, value);
            });
        } else {
            load_constf(value);
            write_instr([=]() {
                sljit_emit_fop1(compiler, SLJIT_MOV_F64, dst.arg_1, dst.arg_2, SLJIT_FR0, 0);
            });
        }
    }

    void binary_op(int op, Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ lhs, rhs, dst });
        write_instr([=]() {
            sljit_emit_op2(compiler, op, dst.arg_1, dst.arg_2, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
    }

    void binary_op_inplace(int op, Mem dst, Mem rhs) {
        binary_op(op, dst, rhs, dst);
    }

    void binary_opf(int op, Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ lhs, rhs, dst });
        write_instr([=]() {
            sljit_emit_fop2(compiler, op, dst.arg_1, dst.arg_2, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
    }

    void binary_opf_inplace(int op, Mem dst, Mem rhs) {
        binary_opf(op, dst, rhs, dst);
    }

    void add(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_ADD, lhs, rhs, dst);
    }

    void addf(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_ADD, lhs, rhs, dst);
    }

    void inplace_add(Mem dst, Mem rhs) {
        binary_op_inplace(SLJIT_ADD, dst, rhs);
    }

    void sub(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB, lhs, rhs, dst);
    }

    void inplace_sub(Mem dst, Mem rhs) {
        binary_op_inplace(SLJIT_SUB, dst, rhs);
    }

    void mul(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_MUL, lhs, rhs, dst);
    }

    void inplace_mul(Mem dst, Mem rhs) {
        binary_op_inplace(SLJIT_MUL, dst, rhs);
    }

    void div(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        move(lhs, Mem::MR0);
        move(rhs, Mem::MR1);
        write_instr([=]() {
            sljit_emit_op0(compiler, SLJIT_DIV_SW);
        });
        move(Mem::MR0, dst);
    }

    void inplace_div(Mem dst, Mem rhs) {
        div(dst, rhs, dst);
    }

    void mod(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        move(lhs, Mem::MR0);
        move(rhs, Mem::MR1);
        write_instr([=]() {
            sljit_emit_op0(compiler, SLJIT_DIVMOD_SW);
        });
        move(Mem::MR1, dst);
    }

    void inplace_mod(Mem dst, Mem rhs) {
        mod(dst, rhs, dst);
    }

    void shift_left(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SHL, lhs, rhs, dst);
    }

    void inplace_shift_left(Mem dst, Mem rhs) {
        shift_left(dst, rhs, dst);
    }

    void shift_right(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SHL, lhs, rhs, dst);
    }

    void inplace_shift_right(Mem dst, Mem rhs) {
        shift_right(dst, rhs, dst);
    }

    void bit_and(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_AND, lhs, rhs, dst);
    }

    void inplace_bit_and(Mem dst, Mem rhs) {
        bit_and(dst, rhs, dst);
    }

    void bit_or(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_OR, lhs, rhs, dst);
    }

    void inplace_bit_or(Mem dst, Mem rhs) {
        bit_or(dst, rhs, dst);
    }

    void bit_xor(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_XOR, lhs, rhs, dst);
    }

    void inplace_bit_xor(Mem dst, Mem rhs) {
        bit_xor(dst, rhs, dst);
    }

    void set_zero(Mem reg) {
        inplace_bit_xor(reg, reg);
    }

    void set_const(Mem dst, int_jt value) {
        if (value == 0 && dst.is_register()) {
            set_zero(dst);
        } else {
            move(dst, Mem::integral_const(value));
        }
    }

    void set_from_flag(int flag_type, Mem dst = Mem::MR0) {
        require_memory({ dst });
        write_instr([=]() {
            sljit_emit_op_flags(compiler, SLJIT_MOV, dst.arg_1, dst.arg_2, flag_type);
        });
    }

    void less_than(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_LESS, lhs, rhs);
        set_from_flag(SLJIT_LESS, dst);
    }

    void less_than_or_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_LESS_EQUAL, lhs, rhs);
        set_from_flag(SLJIT_LESS_EQUAL, dst);
    }

    void greater_than(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_GREATER, lhs, rhs);
        set_from_flag(SLJIT_GREATER, dst);
    }

    void greater_than_or_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_GREATER_EQUAL, lhs, rhs);
        set_from_flag(SLJIT_GREATER_EQUAL, dst);
    }

    void equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_Z, lhs, rhs);
        set_from_flag(SLJIT_EQUAL, dst);
    }

    void not_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT_SUB | SLJIT_SET_Z, lhs, rhs);
        set_from_flag(SLJIT_NOT_EQUAL, dst);
    }

    void logical_and(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        set_zero(Mem::MR0);
        not_equal(rhs, Mem::MR0, Mem::MR1);
        not_equal(lhs, Mem::MR0, Mem::MR0);
        bit_and(Mem::MR0, Mem::MR1, dst);
    }

    void logical_or(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        // dst = (lhs | rhs) != 0
        set_zero(Mem::MR0);
        bit_or(lhs, rhs, Mem::MR1);
        not_equal(Mem::MR0, Mem::MR1, dst);
    }

    void logical_xor(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        // dst = (lhs != 0) ^ (rhs != 0)
        set_zero(Mem::MR0);
        not_equal(rhs, Mem::MR0, Mem::MR1);
        not_equal(lhs, Mem::MR0, Mem::MR0);
        bit_xor(Mem::MR0, Mem::MR1, dst);
    }

    void float_jto_int(Mem src, Mem dst = Mem::MR0) {
        require_memory({ src, dst });
        write_instr([=]() {
            sljit_emit_fop1(compiler, SLJIT_CONV_SW_FROM_F64, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
        });
    }

    void int_jto_float(Mem src, Mem dst = Mem::MFR0) {
        require_memory({ src, dst });
        write_instr([=]() {
            sljit_emit_fop1(compiler, SLJIT_CONV_F64_FROM_SW, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
        });
    }

    struct Jump {
        std::shared_ptr<sljit_jump*> shared;

        Jump() {
            static sljit_jump* dummy_jump;
            this->shared = std::shared_ptr<sljit_jump*>(&dummy_jump);
        }
    };

    Jump jump() {
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit_emit_jump(compiler, SLJIT_JUMP);
        });
        return jump;
    }

    Jump jump_if_true(Mem cond = Mem::MR0) {
        require_memory({ cond, Mem::MR1 });
        Jump jump = Jump();
        write_instr([=]() {
            set_zero(Mem::MR1);
            *jump.shared = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, cond.arg_1, cond.arg_2, SLJIT_R1, 0);
        });
        return jump;
    }

    Jump jump_if_false(Mem cond = Mem::MR0) {
        require_memory({ cond, Mem::MR1 });
        Jump jump = Jump();
        write_instr([=]() {
            set_zero(Mem::MR1);
            *jump.shared = sljit_emit_cmp(compiler, SLJIT_EQUAL, cond.arg_1, cond.arg_2, SLJIT_R1, 0);
        });
        return jump;
    }

    Jump jump_if_equal(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit_emit_cmp(compiler, SLJIT_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    Jump jump_if_not_equal(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    Jump jump_if_less_than(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    void jump_to(const Jump &from) {
        write_instr([=]() {
            sljit_set_label(*from.shared, sljit_emit_label(compiler));
        });
    }
};


#endif //SLJIT_ASM_H
