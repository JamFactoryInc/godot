/**************************************************************************/
/*  gdscript_jit_compiler.h                                               */
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

#ifndef GODOT_GDSCRIPT_JIT_COMPILER_H
#define GODOT_GDSCRIPT_JIT_COMPILER_H

#include "gdscript_opcodes.h"
#include "modules/jit_codegen/asm.h"

/// A unique identifier for a value via its variant address and the instruction at which it was assigned
struct UniqueValue {
    UniqueValue(VariantAddress _address, int _assigned_index): address(_address), assigned_index(_assigned_index) { }

    VariantAddress address;
    int assigned_index;

    bool operator==(const UniqueValue& other) const {
        return address == other.address && assigned_index == other.assigned_index;
    }

    static std::size_t hash(const UniqueValue& value) {
        return HashMapHasherDefault::hash(value.address.address_data) + value.assigned_index;
    }
};

class TypeInvariantSpan {
public:
    TypeInvariantStatus status = TypeInvariantStatus();
    Variant::Type type = Variant::Type::NIL;
    int from_index = -1;
    int to_index = -1;

    TypeInvariantSpan() { }

    TypeInvariantSpan(int _from_index): from_index(_from_index) { }

    TypeInvariantSpan(
        TypeInvariantStatus _status, Variant::Type _type, int _from_index, int _to_index
    ) : status(_status), type(_type), from_index(_from_index), to_index(_to_index) { }

    bool contains(int index) const {
        return index >= from_index && index <= to_index;
    }

    bool exists() const {
        return *this == Variant::Type::NIL;
    }

    bool operator<(const TypeInvariantSpan& other) const {
        return from_index < other.from_index;
    }
    bool operator==(const TypeInvariantSpan& other) const {
        return from_index == other.from_index && to_index == other.to_index;
    }
};

class CodeGraphNode {
public:
    static const CodeGraphNode NIL;
    // idx of first instruction covered by this node
    // this acts as the source of truth for equality and comparison
    int start_index = 0;

    // idx of last instruction covered by this node
    int end_index = 0;

    // each node can have 0 or 2 child nodes as a result of ending in a branch statement
    // these children will be stored as the index of their first instruction, or -1 if no child is present
    int children[2] = { -1, -1 };

    CodeGraphNode() { }

    CodeGraphNode(int _start_index, int _end_index) : start_index(_start_index), end_index(_end_index) { }

    bool operator==(const CodeGraphNode& other) const {
        return start_index == other.start_index;
    }

    static std::size_t hash(const CodeGraphNode& node) { return node.start_index; }
};
const CodeGraphNode CodeGraphNode::NIL = CodeGraphNode();

class CodeGraphNodeInfo {
    bool has_side_effects = false;
};

class CodeGraph {
public:
    HashMap<CodeGraphNode, CodeGraphNodeInfo, CodeGraphNode> nodes;
    HashMap<VariantAddress, Vector<TypeInvariantSpan>, VariantAddress> variable_type_invariants;
    // a map of unique instances of value assignments and their source variable
    HashMap<UniqueValue, UniqueValue, UniqueValue> dependent_invariants;

    void add(CodeGraphNode node, CodeGraphNodeInfo info) {
        nodes.insert(node, info);
    }

    bool index_valid_and_uncrawled(int op_index) {
        if (op_index < 0) {
            return false;
        }
        return !nodes.has(CodeGraphNode(op_index, 0));
    }

    const CodeGraphNode get_node_for(int op_index) {
        auto iter = nodes.find(CodeGraphNode(op_index, 0));
        if (iter) {
            return iter->key;
        } else {
            return CodeGraphNode::NIL;
        }
    }

    TypeInvariantSpan get_type_invariant(VariantAddress address, int effective_index) {
        auto invariant_spans = variable_type_invariants.find(address);
        if (!invariant_spans) {
            // no info known at this time
            return TypeInvariantSpan();
        }
        auto search_key = TypeInvariantSpan(effective_index);
        auto span_index = invariant_spans->value.bsearch(search_key, false);
        return invariant_spans->value[span_index];
    }

    void insert_span(VariantAddress address, TypeInvariantSpan span) {
        auto invariant_spans = variable_type_invariants.find(address);
        if (!invariant_spans) {
            // create the first span and exit
            variable_type_invariants.insert(address, Vector<TypeInvariantSpan>({ span }));
            return;
        }
        Vector<TypeInvariantSpan> &spans = invariant_spans->value;
        size_t insert_index = spans.bsearch(span, true);
        if (insert_index != 0) {
            spans.write[insert_index - 1].to_index = span.from_index - 1;
        }
        if (insert_index < spans.size()) {
            span.to_index = spans[insert_index].from_index - 1;
        }
        spans.insert(insert_index, span);
    }

    void set_span_type(VariantAddress address, int effective_index, TypeInvariantStatus new_status) {
        auto invariant_spans = variable_type_invariants.find(address);
        if (!invariant_spans) {
            // no info known at this time
            return;
        }
        auto search_key = TypeInvariantSpan(effective_index);
        auto span_index = invariant_spans->value.bsearch(search_key, false);
        invariant_spans->value.write[span_index].status = new_status;
    }

    void establish_new_type_invariant(TypeInvariantStatus status, VariantAddress address, Variant::Type type, int start_index) {
        auto span = TypeInvariantSpan(status, type, start_index, -1);
        insert_span(address, span);
    }

    void establish_dependent_type_invariant(TypeInvariantStatus status, VariantAddress address, int start_index) {
        auto dependent = UniqueValue(address, start_index);
        auto dependency = UniqueValue(status.dependency, start_index);
        dependent_invariants.insert(dependent, dependency);

        auto span = TypeInvariantSpan(status, Variant::Type::NIL, start_index, -1);
        insert_span(address, span);
    }

    void terminate_type_invariant(VariantAddress address, int start_index) {
        auto span = TypeInvariantSpan(TypeInvariantStatus(), Variant::Type::NIL, start_index, -1);
        insert_span(address, span);
    }

    // patch spans to be contiguous & only terminated when the type changes
    void patch_spans() {
        for (auto &E : variable_type_invariants) {
            auto spans = E.value;

            TypeInvariantSpan *prev_span = nullptr;
            for (int i = 0; i < spans.size();) {
                TypeInvariantSpan *span = spans.ptrw();
                if (prev_span) {
                    if (prev_span->type == span->type
                        && prev_span->status.status == span->status.status
                        // don't connect dependents, as they each have unique info that must be retained for dependency resolution
                        && prev_span->status.status != TypeInvariantStatus::DEPENDENT
                    ) {
                        // combine this and the previous span
                        spans.remove_at(i);
                        prev_span = &spans.write[i - 1];
                        continue;
                    }
                    prev_span->to_index = span->from_index - 1;
                }
                prev_span = span;

                ++i;
            }
            spans.write[spans.size() - 1].to_index = std::numeric_limits<int>::max();
        }
    }

    void resolve_dependencies() {
        HashSet<UniqueValue, UniqueValue> dependency_chain = HashSet<UniqueValue, UniqueValue>();
        while (!dependent_invariants.is_empty()) {
            auto entry = *dependent_invariants.begin();
            auto dependent = entry.key;
            auto dependency = entry.value;

            while (true) {
                dependency_chain.insert(dependent);

                if (dependency_chain.has(dependency)) {
                    // we've identified a cyclical dependency!
                    // this means we can't infer any type information for any value in the dependency chain,
                    // and should remove them from dependent_invariants and set them to UNKNOWN
                    for (auto cyclical_dependency : dependency_chain) {
                        set_span_type(
                                cyclical_dependency.address,
                                cyclical_dependency.assigned_index,
                                TypeInvariantStatus::UNKNOWN
                        );
                        auto to_remove = dependent_invariants.find(cyclical_dependency);
                        dependent_invariants.remove(to_remove);
                    }
                    dependency_chain.clear();
                    break;
                }

                if (dependent_invariants.has(dependency)) {
                    // the dependency is also a dependent, so continue down the dependency chain
                    dependent = dependency;
                    dependency = dependent_invariants.get(dependency);
                    continue;
                }

                auto maybe_source_type= get_type_invariant(dependency.address, dependency.assigned_index);
                if (maybe_source_type.exists()) {
                    // the dependency has a known type!
                    // update all the dependents in the chain to inherit this type
                    for (auto value_with_inferred_type : dependency_chain) {
                        set_span_type(
                                value_with_inferred_type.address,
                                value_with_inferred_type.assigned_index,
                                maybe_source_type.status
                        );
                        auto to_remove = dependent_invariants.find(value_with_inferred_type);
                        dependent_invariants.remove(to_remove);
                        dependency_chain.clear();
                    }
                    break;
                }
            }
        }

        // we potentially made some neighboring spans have the same type be resolving dependencies, so re-patch them
        patch_spans();
    }
};

class CodeCrawler {
    // a list of jump targets not traced yet
    Vector<int> not_taken_branches;

    int pop_not_taken_branch() {
        if (not_taken_branches.is_empty()) {
            return -1;
        }
        int last_idx = not_taken_branches.size() - 1;
        not_taken_branches.remove_at(last_idx);
        return not_taken_branches[last_idx];
    }

    int get_jump_target(CodeGraph &graph, std::unique_ptr<DynOpcode> &op) {
        if (graph.index_valid_and_uncrawled(op->get_jump_target_a())) {
            int target_to_skip = op->get_jump_target_b();
            if (graph.index_valid_and_uncrawled(target_to_skip)) {
                not_taken_branches.append(target_to_skip);
            }

            return op->get_jump_target_a();
        }

        // we've already crawled the 'a' path, so take the other one
        if (graph.index_valid_and_uncrawled(op->get_jump_target_b())) {
            return op->get_jump_target_b();
        }

        // we've crawled the 'a' and 'b' paths already, so go back to one we skipped
        return pop_not_taken_branch();
    }

    CodeGraph create_graph(GDScriptFunction *fn) {
        auto code = fn->_code_ptr;

        auto graph = CodeGraph();

        auto op= DynOpcode::from_opcode_ptr(fn, code);
        auto node = CodeGraphNode(0, -1);
        auto info = CodeGraphNodeInfo();

        // crawl all the code paths and generate a graph of control flow
        while (true) {
            int op_index = op->index();

            if (op->is_branching() || !op->has_logical_next()) {
                node.end_index = op_index;
                int children[2] = { op->get_jump_target_a(), op->get_jump_target_b() };
                std::copy(std::begin(children), std::end(children), std::begin(node.children));

                graph.add(node, info);

                int jump_target = get_jump_target(graph, op);
                if (jump_target == -1) {
                    // we've crawled every branch!
                    break;
                }

                op = DynOpcode::from_opcode_ptr(fn, &code[jump_target]);
                node = CodeGraphNode(jump_target, -1);
                info = CodeGraphNodeInfo();
                continue;
            }

            auto variant_type_updates = op->type_invariant_statuses();

            if (variant_type_updates.size() == 1 && variant_type_updates[0].status == TypeInvariantStatus::UNKNOWN) {
                // default type status update -- no new info is known
                continue;
            }

            auto addresses = op->_variant_addresses();
            auto variant_types = op->type_invariants();

            // update type invariants for each variable encompassed by this instruction (if any)
            // TODO: assert that these vecs are all the same size
            for (int i = 0; i < variant_type_updates.size(); ++i) {
                const auto status = variant_type_updates[i];
                const auto type = variant_types[i];
                const auto address = *addresses[i];

                switch (status.status) {
                    case TypeInvariantStatus::UNCHANGED: continue;
                    case TypeInvariantStatus::KNOWN: {
                        graph.establish_new_type_invariant(status, address, type, op_index);
                        break;
                    }
                    case TypeInvariantStatus::DEPENDENT: {
                        graph.establish_dependent_type_invariant(status, address, op_index);
                        break;
                    }
                    case TypeInvariantStatus::UNKNOWN: {
                        graph.terminate_type_invariant(address, op_index);
                        break;
                    }
                }
            }
        }

        graph.resolve_dependencies();

        return graph;
    }
};

class GDScriptJitCompiler {
    CodeGraph graph;
    GDScriptFunction fn;

    void optimize() {

    }
};

#endif //GODOT_GDSCRIPT_JIT_COMPILER_H
