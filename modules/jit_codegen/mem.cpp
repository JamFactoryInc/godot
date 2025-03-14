//
// Created by jam on 13/09/2024.
//

#include "mem.h"

template<>
FieldReference _field<void>(int size, int stack_offset, const void *struct_addr, const void *field_addr, bool _is_float) {
    // ensure that we received a field of this struct instance
    size_t field_addr_val = reinterpret_cast<size_t>(field_addr);
    size_t struct_addr_val = reinterpret_cast<size_t>(struct_addr);
    DEV_ASSERT(field_addr_val >= struct_addr_val && field_addr_val < (struct_addr_val + size))
    return FieldReference(stack_offset, field_addr_val - struct_addr_val, _is_float);
}

template<>
FieldReference _field<float_jt>(int size, int stack_offset, const void *struct_addr, const void *field_addr, bool _is_float) {
    return _field<void>(size, stack_offset, struct_addr, field_addr, _is_float);
}

template<>
FieldReference _field<int_jt>(int size, int stack_offset, const void *struct_addr, const void *field_addr, bool _is_float) {
    return _field<void>(size, stack_offset, struct_addr, field_addr, _is_float);
}

template<>
FieldReference _field<void *>(int size, int stack_offset, const void *struct_addr, const void *field_addr, bool _is_float) {
    return _field<void>(size, stack_offset, struct_addr, field_addr, _is_float);
}

const Register Register::R0 = Register { SLJIT2_R0 };
const Register Register::R1 = Register { SLJIT2_R1 };
const Register Register::FR0 = Register { SLJIT2_FR0 };
const Register Register::FR1 = Register { SLJIT2_FR1 };

const Mem Mem::MTRUE = Mem { SLJIT2_IMM, 1 };
const Mem Mem::MFALSE = Mem { SLJIT2_IMM, 0 };
const Mem Mem::MR0 = Mem { Register::R0 };
const Mem Mem::MR1 = Mem { Register::R1 };
const Mem Mem::MFR0 = Mem { Register::FR0 };
const Mem Mem::MFR1 = Mem { Register::FR1 };