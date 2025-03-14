/**************************************************************************/
/*  gdscript_opcodes.cpp                                                       */
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

#include "gdscript_opcodes.h"
#include "gdscript_byte_codegen.h"

const Signature Signature::UNINIT = Signature(0);
const Signature Signature::OPAQUE = Signature(0xFFFF);

std::unique_ptr<DynOpcode> DynOpcode::from_opcode_ptr(GDScriptFunction *fn, int *opcode_ptr) {
    auto opcode = *reinterpret_cast<GDScriptFunction::Opcode *>(opcode_ptr);
    switch (opcode) {
        case GDScriptFunction::Opcode::OPCODE_OPERATOR:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_OPERATOR>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_OPERATOR_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_OPERATOR_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_TEST_BUILTIN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_TEST_BUILTIN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_TEST_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_TEST_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_TEST_NATIVE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_TEST_NATIVE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_TEST_SCRIPT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_TEST_SCRIPT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_KEYED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_KEYED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_KEYED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_KEYED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_INDEXED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_INDEXED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_KEYED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_KEYED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_KEYED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_KEYED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_INDEXED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_INDEXED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_NAMED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_NAMED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_NAMED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_NAMED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_NAMED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_NAMED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_NAMED_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_NAMED_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_MEMBER:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_MEMBER>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_MEMBER:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_MEMBER>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_SET_STATIC_VARIABLE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_SET_STATIC_VARIABLE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_GET_STATIC_VARIABLE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_GET_STATIC_VARIABLE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_NULL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_NULL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_TRUE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_FALSE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_BUILTIN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_NATIVE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSIGN_TYPED_SCRIPT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CAST_TO_BUILTIN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CAST_TO_NATIVE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CAST_TO_SCRIPT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CONSTRUCT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CONSTRUCT_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CONSTRUCT_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_TYPED_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CONSTRUCT_TYPED_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CONSTRUCT_DICTIONARY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_ASYNC:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_ASYNC>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_UTILITY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_UTILITY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_UTILITY_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_UTILITY_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_GDSCRIPT_UTILITY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_GDSCRIPT_UTILITY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_TYPE_VALIDATED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_TYPE_VALIDATED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_SELF_BASE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_RET:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_RET>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_STATIC:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_BUILTIN_STATIC>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_NATIVE_STATIC_VALIDATED_NO_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CALL_METHOD_BIND_VALIDATED_NO_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_AWAIT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_AWAIT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_AWAIT_RESUME:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_AWAIT_RESUME>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CREATE_LAMBDA:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CREATE_LAMBDA>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_CREATE_SELF_LAMBDA:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_CREATE_SELF_LAMBDA>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_JUMP:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_JUMP>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_JUMP_IF>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_JUMP_IF_NOT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_JUMP_TO_DEF_ARGUMENT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_JUMP_IF_SHARED:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_JUMP_IF_SHARED>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_RETURN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_RETURN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_BUILTIN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_BUILTIN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_NATIVE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_NATIVE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_SCRIPT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_RETURN_TYPED_SCRIPT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_INT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_INT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_FLOAT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_FLOAT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR2I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_VECTOR3I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_STRING:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_STRING>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_DICTIONARY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_DICTIONARY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_BYTE_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_INT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_FLOAT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_STRING_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR2_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR3_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_COLOR_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_PACKED_VECTOR4_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_OBJECT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_BEGIN_OBJECT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_INT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_INT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_FLOAT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_FLOAT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR2I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_VECTOR3I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_STRING:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_STRING>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_DICTIONARY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_DICTIONARY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_BYTE_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_BYTE_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_INT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_FLOAT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_STRING_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_STRING_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR2_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR2_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR3_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR3_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_COLOR_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_COLOR_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR4_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_PACKED_VECTOR4_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ITERATE_OBJECT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ITERATE_OBJECT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_STORE_GLOBAL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_STORE_GLOBAL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_STORE_NAMED_GLOBAL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_STORE_NAMED_GLOBAL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BOOL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BOOL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_INT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_INT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_FLOAT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_FLOAT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR2I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RECT2I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR3I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM2D:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM2D>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4I:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_VECTOR4I>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PLANE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PLANE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_QUATERNION:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_QUATERNION>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_AABB:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_AABB>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BASIS:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_BASIS>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM3D:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_TRANSFORM3D>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PROJECTION:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PROJECTION>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_COLOR:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_COLOR>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING_NAME:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_STRING_NAME>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_NODE_PATH:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_NODE_PATH>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RID:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_RID>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_OBJECT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_OBJECT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_CALLABLE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_CALLABLE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_SIGNAL:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_SIGNAL>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_DICTIONARY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_DICTIONARY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_BYTE_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_INT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT32_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_FLOAT64_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_STRING_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR2_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR3_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_COLOR_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_TYPE_ADJUST_PACKED_VECTOR4_ARRAY>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_ASSERT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_ASSERT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_BREAKPOINT:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_BREAKPOINT>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_LINE:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_LINE>::Dyn(fn, opcode_ptr));
        case GDScriptFunction::Opcode::OPCODE_END:
            return std::unique_ptr<DynOpcode>(new StaticOpcode<GDScriptFunction::Opcode::OPCODE_END>::Dyn(fn, opcode_ptr));
    }
}

