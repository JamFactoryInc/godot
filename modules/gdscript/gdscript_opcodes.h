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

namespace GDScriptOpcodes {

const int POINTER_SIZE = sizeof(size_t);

/**
 * @brief A
 * 
 * @tparam POINTER_TYPE 
 */
template<typename POINTER_TYPE = void>
struct EncodedPointer {
	constexpr static const int PTR_SIZE = POINTER_SIZE / sizeof(int);

	int ptr_parts[EncodedPointer::PTR_SIZE];

public:
	// inline EncodedPointer(POINTER_TYPE ptr) {
	// 	*this = *reinterpret_cast<EncodedPointer<POINTER_TYPE> *>(&ptr);
	// }
	EncodedPointer(POINTER_TYPE ptr) {
		int *data = reinterpret_cast<int *>(&this->ptr_parts);
		*data = *reinterpret_cast<int *>(&ptr);
	}

	// inline POINTER_TYPE get() const {
	// 	return reinterpret_cast<POINTER_TYPE>(this);
	// }
	constexpr inline POINTER_TYPE get() const {
		if constexpr (PTR_SIZE == 1) {
			return reinterpret_cast<POINTER_TYPE>(ptr_parts[0]);
		} else if constexpr (PTR_SIZE == 2) {
			size_t ptr = ptr_parts[0];
			return reinterpret_cast<POINTER_TYPE>((ptr << sizeof(int)) & ptr_parts[1]);
		} else {
			static_assert(false, "Unhandled system pointer size")
		}
	}
};

struct VariantAddress {
	int address_data;

public:
	inline Variant *get_variant_address(Variant **variant_addresses) {
		return &variant_addresses[this->get_address_type()][this->get_address_index()];
	}

	inline int get_address_index() {
		return this->address_data & GDScriptFunction::Address::ADDR_MASK;
	}

	inline int get_address_type() {
		return (this->address_data & GDScriptFunction::Address::ADDR_TYPE_MASK) >> GDScriptFunction::Address::ADDR_BITS;
	}
};

struct alignas(uint8_t) Test {
	uint8_t padding[2];
	uint8_t a;
	uint8_t b;

	Test from_code(int *code) {
		return *reinterpret_cast<Test *>(code);
	}
};

/**
 * The signature of a binary operation represented by the left and right operand types
 */
struct Signature {
	int type_data;

public:
	static const Signature UNINIT;
	static const Signature OPAQUE;

	constexpr inline bool operator==(Signature &rhs) {
		return this->type_data == rhs.type_data;
	}

	inline Signature(Variant *left, Variant *right) :
			type_data((left->get_type() << 8) | right->get_type()) { }

	inline Signature(Variant::Type left_type, Variant::Type right_type) :
			type_data((left_type << 8) | right_type) { }

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
			type_data(value) { }
};



#define _GET_TYPE(_, TYPE, IDENT) TYPE
#define _GET_IDENT(_, TYPE, IDENT) IDENT

#define _GET_FIELD(MACRO, TYPE, IDENT) MACRO(RuntimeTypeLookup<TYPE>::type IDENT;)

#define _DO_NOTHING(...) __VA_ARGS__
#define _IGNORE(...)

// this is just a hack to make expanded varargs correctly act as multiple arguments to subsequent macros
#define SEPARATE(_1, _2, _3, _4, _5, _6, _7) (_1, _2, _3, _4, _5, _6, _7)

#define ARGS(_1, _2, _3, _4, _5, _6, _7, ...) _GET_TYPE _1 _GET_TYPE _2 _GET_TYPE _3 _GET_TYPE _4 _GET_TYPE _5 _GET_TYPE _6 _GET_TYPE _7
#define FIELDS(_1, _2, _3, _4, _5, _6, _7, ...) _GET_FIELD _1 _GET_FIELD _2 _GET_FIELD _3 _GET_FIELD _4 _GET_FIELD _5 _GET_FIELD _6 _GET_FIELD _7

#define DEFINE_OPCODE(OPCODE, ...)                                                                                                             \
	template <>                                                                                                                                \
	class BaseOpcode<OPCODE> {                                                                                                                 \
	public:                                                                                                                                    \
		const GDScriptFunction::Opcode opcode;                                                                                                 \
		FIELDS SEPARATE(__VA_ARGS__, (_IGNORE, , ), (_IGNORE, , ), (_IGNORE, , ), (_IGNORE, , ), (_IGNORE, , ), (_IGNORE, , ), (_IGNORE, , )); \
		constexpr inline int size() {                                                                                                          \
			return sizeof(GDScriptOpcodes::BaseOpcode<OPCODE>) / sizeof(int);                                                                  \
		}                                                                                                                                      \
		static BaseOpcode<OPCODE> &from_instr_ptr(int *code_ptr, int instr_ptr) {                                                              \
			int &opcode_addr = code_ptr[instr_ptr];                                                                                           \
			return reinterpret_cast<BaseOpcode<OPCODE> &>(opcode_addr);                                                                        \
		};                                                                                                                                     \
	};

#define ARG(...) (_DO_NOTHING, __VA_ARGS__)

enum ArgType {
	TYPE,
	VARIANT,
	OPERATOR,
	SIGNATURE,
	OPERATOR_EVAL,
};

#define RUNTIME_ARG_TYPE(ARG_TYPE, TYPE_AT_RUNTIME) \
template <> \
struct RuntimeTypeLookup<ARG_TYPE> { using type = TYPE_AT_RUNTIME; };

#define COMPTIME_ARG_TYPE(ARG_TYPE, TYPE_AT_COMPILETIME) \
template <> \
struct CompiletimeTypeLookup<ARG_TYPE> { using type = TYPE_AT_COMPILETIME; };


template <ArgType>
struct RuntimeTypeLookup {
	using type = void;
};

template <ArgType>
struct CompiletimeTypeLookup {
	using type = void;
};

RUNTIME_ARG_TYPE(ArgType::TYPE, Variant::Type)
RUNTIME_ARG_TYPE(ArgType::VARIANT, GDScriptOpcodes::VariantAddress)
RUNTIME_ARG_TYPE(ArgType::OPERATOR, Variant::Operator)
RUNTIME_ARG_TYPE(ArgType::SIGNATURE, GDScriptOpcodes::Signature)
RUNTIME_ARG_TYPE(ArgType::OPERATOR_EVAL, GDScriptOpcodes::EncodedPointer<Variant::ValidatedOperatorEvaluator>)

template<GDScriptFunction::Opcode CODE>
class BaseOpcode {

};

DEFINE_OPCODE(GDScriptFunction::Opcode::OPCODE_OPERATOR,
		ARG(ArgType::VARIANT, left_operand),
		ARG(ArgType::VARIANT, right_operand),
		ARG(ArgType::VARIANT, destination),
		ARG(ArgType::OPERATOR, operation),
		ARG(ArgType::SIGNATURE, signature),
		ARG(ArgType::TYPE, return_type),
		ARG(ArgType::OPERATOR_EVAL, operator_fn));

// template <>
// class BaseOpcode<GDScriptFunction::Opcode::OPCODE_OPERATOR> {
// 	RuntimeTypeLookup<ArgType::VARIANT>::type left_operand;
// 	RuntimeTypeLookup<ArgType::VARIANT>::type right_operand;
// 	RuntimeTypeLookup<ArgType::VARIANT>::type destination;
// 	RuntimeTypeLookup<ArgType::OPERATOR>::type operation;
// 	RuntimeTypeLookup<ArgType::SIGNATURE>::type signature;
// 	RuntimeTypeLookup<ArgType::SIGNATURE>::type return_type;
// 	RuntimeTypeLookup<ArgType::POINTER>::type operator_fn;

	
// };


}

#endif // GDSCRIPT_OPCODES_H
