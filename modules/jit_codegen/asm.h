//
// Created by jam on 11/09/2024.
//

#ifndef SLJIT2_ASM_H
#define SLJIT2_ASM_H

#include "mem.h"
#include <vector>
#include <functional>
#include <unordered_set>
#include <memory>

using namespace jt;

enum AbiArgType {
    TYPE_VOID = SLJIT2_ARG_TYPE_RET_VOID,
    TYPE_WORD = SLJIT2_ARG_TO_TYPE(W),
    TYPE_POINTER = SLJIT2_ARG_TO_TYPE(P),
    TYPE_FLOAT = SLJIT2_ARG_TO_TYPE(F64),
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
    sljit2_compiler *compiler;
    std::unordered_set<sljit2_s32> initialized_floats;
    std::vector<std::function<void()>> initializers;
    std::vector<std::function<void()>> instructions;
    int args = 0;
    int float_args = 0;
    int required_registers = 0;
    int required_float_registers = 0;
    int stack_size = 0;

    Asm() {
        compiler = sljit2_create_compiler(nullptr);
    }

    void write_init(const std::function<void()> &instr) {
        initializers.push_back(instr);
    }

    void write_instr(const std::function<void()> &instr) {
        instructions.push_back(instr);
    }

    void init_arg(Mem reg) {
        if (!reg.is_float()) {
            args = std::max(args, SLJIT2_NUMBER_OF_REGISTERS - reg.arg_1 + 1);
            return;
        }
        bool is_initialized = *initialized_floats.find(reg.arg_1);
        if (!is_initialized) {
            initialized_floats.insert(reg.arg_1);
            // copy our float reg to the associated save reg so we can freely use it as temp memory
            write_init([=]() {
                sljit2_emit_fop1(compiler, SLJIT2_MOV_F64, SLJIT2_FS(reg.arg_1), 0, SLJIT2_FR(reg.arg_1), 0);
            });
            float_args = std::max(float_args, reg.arg_1);
        }
    }

    void require_memory(std::initializer_list<Mem> registers) {
        for (Mem reg: registers) {
            if (reg.is_arg()) {
                init_arg(reg);
                continue;
            }

            if (reg.is_register()) {
                required_registers = std::max(required_registers, reg.arg_1);
            } else if (reg.is_float_register()) {
                required_float_registers = std::max(required_float_registers, reg.arg_1);
            }

            if (reg.is_uninit_local_address()) {
                int size = reg.get_uninit_local_size();
                reg.initialize_local(stack_size);
                stack_size += size;
            }
        }
    }

public:

    template<typename R>
    std::function<R(void *)> compile_array_args() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<sljit2_up>::value << 4;
        return reinterpret_cast<R (*)()>(_compile(arg_types));
    }

    template<typename R>
    std::function<R()> compile() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        return reinterpret_cast<R (*)()>(_compile(arg_types));
    }

    template<typename R, typename A1>
    std::function<R(A1)> compile() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        return reinterpret_cast<R (*)(A1)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2>
    std::function<R(A1, A2)> compile() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        return reinterpret_cast<R (*)(A1, A2)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2, typename A3>
    std::function<R(A1, A2, A3)> compile() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        arg_types |= ArgTypeOf<A3>::value << 12;
        return reinterpret_cast<R (*)(A1, A2, A3)>(_compile(arg_types));
    }

    template<typename R, typename A1, typename A2, typename A3, typename A4>
    std::function<R(A1, A2, A3, A4)> compile() {
        sljit2_s32 arg_types = ArgTypeOf<R>::value;
        arg_types |= ArgTypeOf<A1>::value << 4;
        arg_types |= ArgTypeOf<A2>::value << 8;
        arg_types |= ArgTypeOf<A3>::value << 12;
        arg_types |= ArgTypeOf<A4>::value << 16;
        return reinterpret_cast<R (*)(A1, A2, A3, A4)>(_compile(arg_types));
    }

    void *_compile(sljit2_s32 arg_types) {
        required_float_registers = std::max(required_float_registers, float_args);

        DEV_ASSERT(required_registers <= SLJIT2_NUMBER_OF_REGISTERS);
        DEV_ASSERT(required_float_registers <= required_float_registers);

        sljit2_emit_enter(
            compiler, 0, arg_types,
            required_registers | SLJIT2_ENTER_FLOAT(required_float_registers),
            args | SLJIT2_ENTER_FLOAT(float_args),
            stack_size
        );
        for (const auto& init: initializers) {
            init();
        }
        for (const auto& instr: instructions) {
            instr();
        }

        return sljit2_generate_code(compiler, 0, nullptr);
    }

    template<typename T>
    LocalVar<T> declare_variable() {
        Mem local = Mem::local_variable<T>();
        require_memory({ local });
        return LocalVar<T>(local.arg_2);
    }

    bool coerce_floats(Mem &lhs, Mem &rhs) {
        if (lhs.is_float() == rhs.is_float()) {
            return lhs.is_float();
        }
        Mem dst_reg = Mem::MFR0;
        if (lhs == Mem::MFR0 || rhs == Mem::MFR0) {
            dst_reg = Mem::MFR1;
        }
        require_memory({ dst_reg });
        if (lhs.is_float()) {
            int_to_float(lhs, dst_reg);
            lhs = dst_reg;
        } else if (rhs.is_float()) {
            int_to_float(rhs, dst_reg);
            rhs = dst_reg;
        }
        return true;
    }

    Mem move(Mem src, Mem dst) {
        bool float_op = coerce_floats(src, dst);
        require_memory({ src, dst });
        if (src == dst) {
            return dst;
        }

        if (float_op) {
            write_instr([=]() {
                sljit2_emit_fop1(compiler, SLJIT2_MOV_F64, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
            });
        } else {
            write_instr([=]() {
                sljit2_emit_op1(compiler, SLJIT2_MOV, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
            });
        }
        return dst;
    }

    void return_value(Mem src) {
        require_memory({ src });
        if (src.is_float()) {
            write_instr([=]() {
                sljit2_emit_return(compiler, SLJIT2_MOV_F64, src.arg_1, src.arg_2);
            });
        } else {
            write_instr([=]() {
                sljit2_emit_return(compiler, SLJIT2_MOV, src.arg_1, src.arg_2);
            });
        }
    }

    void return_void() {
        write_instr([=]() {
            sljit2_emit_return_void(compiler);
        });
    }

    Mem load_constf(float_jt value) {
        require_memory({ Mem::MFR0 });
        write_instr([=]() {
            sljit2_emit_fset64(compiler, SLJIT2_FR0, value);
        });
        return Mem::MFR0;
    }

    Mem set_constf(Mem dst, float_jt value) {
        require_memory({ dst });
        write_instr([=]() {
            sljit2_emit_fset64(compiler, dst.arg_1, value);
        });
        return dst;
    }

    Mem float_to_int(Mem src, Mem dst = Mem::MR0) {
        require_memory({ src, dst });
        write_instr([=]() {
            sljit2_emit_fop1(compiler, SLJIT2_CONV_SW_FROM_F64, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
        });
        return dst;
    }

    Mem int_to_float(Mem src, Mem dst = Mem::MFR0) {
        require_memory({ src, dst });
        write_instr([=]() {
            sljit2_emit_fop1(compiler, SLJIT2_CONV_F64_FROM_SW, dst.arg_1, dst.arg_2, src.arg_1, src.arg_2);
        });
        return dst;
    }

    Mem binary_op(int op, Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ lhs, rhs, dst });
        if (coerce_floats(lhs, rhs)) {
            if (dst == Mem::MR0) {
                dst = Mem::MFR0;
            }
            switch (op) {
                case SLJIT2_ADD: op = SLJIT2_ADD_F64; break;
                case SLJIT2_SUB: op = SLJIT2_SUB_F64; break;
                case SLJIT2_MUL: op = SLJIT2_MUL_F64; break;
                case SLJIT2_DIV_SW: op = SLJIT2_DIV_F64; break;
            }
            write_instr([=]() {
                sljit2_emit_fop2(compiler, op, dst.arg_1, dst.arg_2, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
            });
        } else {
            write_instr([=]() {
                sljit2_emit_op2(compiler, op, dst.arg_1, dst.arg_2, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
            });
        }
        return dst;
    }

    Mem binary_op_inplace(int op, Mem dst, Mem rhs) {
        return binary_op(op, dst, rhs, dst);
    }

    Mem add(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_ADD, lhs, rhs, dst);
    }

    Mem inplace_add(Mem dst, Mem rhs) {
        return binary_op_inplace(SLJIT2_ADD, dst, rhs);
    }

    Mem sub(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_SUB, lhs, rhs, dst);
    }

    Mem inplace_sub(Mem dst, Mem rhs) {
        return binary_op_inplace(SLJIT2_SUB, dst, rhs);
    }

    Mem mul(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_MUL, lhs, rhs, dst);
    }

    Mem inplace_mul(Mem dst, Mem rhs) {
        return binary_op_inplace(SLJIT2_MUL, dst, rhs);
    }

    Mem div(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        if (coerce_floats(lhs, rhs)) {
            if (dst == Mem::MR0) {
                dst = Mem::MFR0;
            }
            require_memory({ Mem::MR0, Mem::MR1, dst });
            move(lhs, Mem::MR0);
            move(rhs, Mem::MR1);
            write_instr([=]() {
                sljit2_emit_op0(compiler, SLJIT2_DIV_SW);
            });
            move(Mem::MR0, dst);
        } else {
            require_memory({ Mem::MR0, Mem::MR1, dst });
            move(lhs, Mem::MR0);
            move(rhs, Mem::MR1);
            write_instr([=]() {
                sljit2_emit_op0(compiler, SLJIT2_DIV_SW);
            });
            move(Mem::MR0, dst);
        }
        return dst;
    }

    Mem inplace_div(Mem dst, Mem rhs) {
        return div(dst, rhs, dst);
    }

    Mem mod(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1, dst });
        move(lhs, Mem::MR0);
        move(rhs, Mem::MR1);
        write_instr([=]() {
            sljit2_emit_op0(compiler, SLJIT2_DIVMOD_SW);
        });
        move(Mem::MR1, dst);
        return dst;
    }

    Mem inplace_mod(Mem dst, Mem rhs) {
        return mod(dst, rhs, dst);
    }

    Mem shift_left(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_SHL, lhs, rhs, dst);
    }

    Mem inplace_shift_left(Mem dst, Mem rhs) {
        return shift_left(dst, rhs, dst);
    }

    Mem shift_right(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return  binary_op(SLJIT2_SHL, lhs, rhs, dst);
    }

    Mem inplace_shift_right(Mem dst, Mem rhs) {
        return shift_right(dst, rhs, dst);
    }

    Mem bit_and(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_AND, lhs, rhs, dst);
    }

    Mem inplace_bit_and(Mem dst, Mem rhs) {
        return bit_and(dst, rhs, dst);
    }

    Mem bit_or(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_OR, lhs, rhs, dst);
    }

    Mem inplace_bit_or(Mem dst, Mem rhs) {
        return bit_or(dst, rhs, dst);
    }

    Mem bit_xor(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        return binary_op(SLJIT2_XOR, lhs, rhs, dst);
    }

    Mem inplace_bit_xor(Mem dst, Mem rhs) {
        return bit_xor(dst, rhs, dst);
    }

    Mem set_zero(Mem reg) {
        return inplace_bit_xor(reg, reg);
    }

    Mem set_const(Mem dst, int_jt value) {
        if (value == 0 && dst.is_register()) {
            return set_zero(dst);
        } else {
            return move(dst, Mem::integral_const(value));
        }
    }

    Mem set_from_flag(int flag_type, Mem dst = Mem::MR0) {
        require_memory({ dst });
        write_instr([=]() {
            sljit2_emit_op_flags(compiler, SLJIT2_MOV, dst.arg_1, dst.arg_2, flag_type);
        });
        return dst;
    }

    Mem less_than(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_LESS, lhs, rhs);
        set_from_flag(SLJIT2_LESS, dst);
        return dst;
    }

    Mem less_than_or_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_LESS_EQUAL, lhs, rhs);
        set_from_flag(SLJIT2_LESS_EQUAL, dst);
        return dst;
    }

    Mem greater_than(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_GREATER, lhs, rhs);
        set_from_flag(SLJIT2_GREATER, dst);
        return dst;
    }

    Mem greater_than_or_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_GREATER_EQUAL, lhs, rhs);
        set_from_flag(SLJIT2_GREATER_EQUAL, dst);
        return dst;
    }

    Mem equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_Z, lhs, rhs);
        set_from_flag(SLJIT2_EQUAL, dst);
        return dst;
    }

    Mem not_equal(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        binary_op(SLJIT2_SUB | SLJIT2_SET_Z, lhs, rhs);
        set_from_flag(SLJIT2_NOT_EQUAL, dst);
        return dst;
    }

    Mem logical_and(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        set_zero(Mem::MR0);
        not_equal(rhs, Mem::MR0, Mem::MR1);
        not_equal(lhs, Mem::MR0, Mem::MR0);
        bit_and(Mem::MR0, Mem::MR1, dst);
        return dst;
    }

    Mem logical_or(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        // dst = (lhs | rhs) != 0
        set_zero(Mem::MR0);
        bit_or(lhs, rhs, Mem::MR1);
        not_equal(Mem::MR0, Mem::MR1, dst);
        return dst;
    }

    Mem logical_xor(Mem lhs, Mem rhs, Mem dst = Mem::MR0) {
        require_memory({ Mem::MR0, Mem::MR1 });
        // dst = (lhs != 0) ^ (rhs != 0)
        set_zero(Mem::MR0);
        not_equal(rhs, Mem::MR0, Mem::MR1);
        not_equal(lhs, Mem::MR0, Mem::MR0);
        bit_xor(Mem::MR0, Mem::MR1, dst);
        return dst;
    }

    struct Jump {
        std::shared_ptr<sljit2_jump*> shared;

        Jump() {
            static sljit2_jump* dummy_jump;
            this->shared = std::shared_ptr<sljit2_jump*>(&dummy_jump);
        }
    };

    Jump jump() {
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit2_emit_jump(compiler, SLJIT2_JUMP);
        });
        return jump;
    }

    Jump jump_if_true(Mem cond = Mem::MR0) {
        require_memory({ cond, Mem::MR1 });
        Jump jump = Jump();
        write_instr([=]() {
            set_zero(Mem::MR1);
            *jump.shared = sljit2_emit_cmp(compiler, SLJIT2_NOT_EQUAL, cond.arg_1, cond.arg_2, SLJIT2_R1, 0);
        });
        return jump;
    }

    Jump jump_if_false(Mem cond = Mem::MR0) {
        require_memory({ cond, Mem::MR1 });
        Jump jump = Jump();
        write_instr([=]() {
            set_zero(Mem::MR1);
            *jump.shared = sljit2_emit_cmp(compiler, SLJIT2_EQUAL, cond.arg_1, cond.arg_2, SLJIT2_R1, 0);
        });
        return jump;
    }

    Jump jump_if_equal(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit2_emit_cmp(compiler, SLJIT2_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    Jump jump_if_not_equal(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit2_emit_cmp(compiler, SLJIT2_NOT_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    Jump jump_if_less_than(Mem lhs, Mem rhs) {
        require_memory({ lhs, rhs });
        Jump jump = Jump();
        write_instr([=]() {
            *jump.shared = sljit2_emit_cmp(compiler, SLJIT2_NOT_EQUAL, lhs.arg_1, lhs.arg_2, rhs.arg_1, rhs.arg_2);
        });
        return jump;
    }

    void jump_to(const Jump &from) {
        write_instr([=]() {
            sljit2_set_label(*from.shared, sljit2_emit_label(compiler));
        });
    }
};


#endif //SLJIT2_ASM_H
