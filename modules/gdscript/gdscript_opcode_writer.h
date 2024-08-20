#include "modules/gdscript/gdscript_byte_codegen.h"
#include "modules/gdscript/gdscript_opcodes.h"

class OpcodeWriter {
	static void append_opcode(GDScriptByteCodeGenerator *gen, GDScriptFunction::Opcode opcode) {
		gen->append_opcode(opcode);
	}

	static void append_address(GDScriptByteCodeGenerator *gen, const GDScriptCodeGenerator::Address &address) {
		gen->append(address);
	}

	// static void append_signature_storage(GDScriptByteCodeGenerator *gen, GDScriptOpcodes::Signature signature) {
	// 	gen->append(signature);
	// }

	static void append_type_storage(GDScriptByteCodeGenerator *gen, Variant::Type type) {
		gen->append(type);
	}

	template <typename PTR_TYPE>
	static void append_ptr(GDScriptByteCodeGenerator *gen) {
		for (int i = 0; i < GDScriptOpcodes::EncodedPointer<>::PTR_SIZE; ++i) {
			gen->append(0);
		}
	}
};