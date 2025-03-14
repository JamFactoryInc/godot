//
// Created by jam on 13/09/2024.
//

#ifndef GODOT_JIT_CODEGEN_TEST_H
#define GODOT_JIT_CODEGEN_TEST_H

#include "tests/test_macros.h"
#include "../asm.h"
#include <cstdio>
#include <chrono>
#include <ctime>

TEST_CASE("JIT Compilation sanity check") {

    Asm asm_generator = Asm();

    auto sum = asm_generator.add(Mem::arg(0), Mem::arg(1));
    asm_generator.return_value(sum);

    auto fn = asm_generator.compile<int_jt, int_jt, int_jt>();
    int_jt result = fn(1, 2);

    CHECK((result == 3));
}

TEST_CASE("Local variables") {

    Asm asm_generator = Asm();

    auto a = Mem::local_variable<int_jt>();
    auto b = Mem::local_variable<int_jt>();
    asm_generator.set_const(a, 2);
    asm_generator.set_const(a, 3);
    auto sum = asm_generator.add(a, b);
    asm_generator.return_value(sum);

    auto fn = asm_generator.compile<int_jt>();
    int_jt result = fn();

    CHECK((result == 5));
}

#endif //GODOT_JIT_CODEGEN_TEST_H
