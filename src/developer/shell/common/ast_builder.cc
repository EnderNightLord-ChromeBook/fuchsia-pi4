// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/developer/shell/common/ast_builder.h"

#include <algorithm>

namespace shell::console {

using NodeId = AstBuilder::NodeId;

NodeId AstBuilder::AddNode(llcpp::fuchsia::shell::Node&& node, bool is_root) {
  llcpp::fuchsia::shell::NodeDefinition definition;
  definition.node = std::move(node);
  definition.root_node = is_root;
  llcpp::fuchsia::shell::NodeId id;
  id.file_id = file_id_;
  id.node_id = ++next_id_;
  definition.node_id = std::move(id);
  nodes_.push_back(std::move(definition));
  return id;
}

fidl::VectorView<llcpp::fuchsia::shell::NodeDefinition> AstBuilder::DefsAsVectorView() {
  fidl::VectorView<llcpp::fuchsia::shell::NodeDefinition> result(allocator_, nodes_.size());

  for (size_t i = 0; i < nodes_.size(); ++i) {
    result[i] = std::move(nodes_[i]);
  }

  nodes_.clear();

  return std::move(result);
}

fidl::VectorView<llcpp::fuchsia::shell::Node> AstBuilder::NodesAsVectorView() {
  struct {
    bool operator()(const llcpp::fuchsia::shell::NodeDefinition& a,
                    const llcpp::fuchsia::shell::NodeDefinition& b) const {
      return a.node_id.node_id < b.node_id.node_id;
    }
  } cmp;
  std::sort(nodes_.begin(), nodes_.end(), cmp);

  fidl::VectorView<llcpp::fuchsia::shell::Node> raw_nodes(allocator_, nodes_.size());
  for (size_t i = 0; i < nodes_.size(); ++i) {
    raw_nodes[i] = std::move(nodes_[i].node);
  }

  nodes_.clear();

  return std::move(raw_nodes);
}

void AstBuilder::SetRoot(NodeId node_id) {
  for (size_t i = 0; i < nodes_.size(); i++) {
    if (nodes_[i].node_id.node_id == node_id.node_id &&
        nodes_[i].node_id.file_id == node_id.file_id) {
      nodes_[i].root_node = true;
      return;
    }
  }
}

NodeId AstBuilder::AddVariableDeclaration(const std::string& identifier,
                                          llcpp::fuchsia::shell::ShellType&& type, NodeId node_id,
                                          bool is_const, bool is_root) {
  fidl::ObjectView<llcpp::fuchsia::shell::VariableDefinition> def(allocator_);
  def->name.Set(allocator_, identifier);
  def->type = std::move(type);
  def->mutable_value = !is_const;
  def->initial_value = node_id;
  return AddNode(llcpp::fuchsia::shell::Node::WithVariableDefinition(def), is_root);
}

NodeId AstBuilder::AddVariable(const std::string& identifier) {
  fidl::ObjectView<fidl::StringView> name(allocator_, allocator_, identifier);
  return AddNode(llcpp::fuchsia::shell::Node::WithVariable(name));
}

NodeId AstBuilder::AddIntegerLiteral(uint64_t i, bool is_negative) {
  fidl::ObjectView<llcpp::fuchsia::shell::IntegerLiteral> literal(allocator_);
  literal->absolute_value.Allocate(allocator_, 1);
  literal->absolute_value[0] = i;
  literal->negative = is_negative;
  return AddNode(llcpp::fuchsia::shell::Node::WithIntegerLiteral(literal));
}

NodeId AstBuilder::AddIntegerLiteral(int64_t i) {
  uint64_t absolute_value = (i == LLONG_MIN) ? (static_cast<uint64_t>(LLONG_MAX) + 1) : llabs(i);

  fidl::ObjectView<llcpp::fuchsia::shell::IntegerLiteral> literal(allocator_);
  literal->absolute_value.Allocate(allocator_, 1);
  literal->absolute_value[0] = absolute_value;
  literal->negative = i < 0;
  return AddNode(llcpp::fuchsia::shell::Node::WithIntegerLiteral(literal));
}

NodeId AstBuilder::AddStringLiteral(const std::string& s) {
  fidl::ObjectView<fidl::StringView> literal(allocator_, allocator_, s);
  return AddNode(llcpp::fuchsia::shell::Node::WithStringLiteral(literal));
}

void AstBuilder::AddEmitResult(NodeId expression) {
  fidl::ObjectView<llcpp::fuchsia::shell::NodeId> emit_result(allocator_, expression);
  AddNode(llcpp::fuchsia::shell::Node::WithEmitResult(emit_result), /*root_node=*/true);
}

NodeId AstBuilder::AddAssignment(NodeId destination, NodeId source) {
  fidl::ObjectView<llcpp::fuchsia::shell::Assignment> assignment(allocator_);
  assignment->destination = destination;
  assignment->source = source;
  return AddNode(llcpp::fuchsia::shell::Node::WithAssignment(assignment), /*root_node=*/true);
}

NodeId AstBuilder::AddAddition(bool with_exceptions, NodeId left_id, NodeId right_id) {
  fidl::ObjectView<llcpp::fuchsia::shell::Addition> addition(allocator_);
  addition->left = left_id;
  addition->right = right_id;
  addition->with_exceptions = with_exceptions;
  return AddNode(llcpp::fuchsia::shell::Node::WithAddition(addition), /*root_node=*/false);
}

void AstBuilder::OpenObject() { object_stack_.emplace_back(); }

AstBuilder::NodePair AstBuilder::CloseObject() {
  fidl::ObjectView<llcpp::fuchsia::shell::ObjectSchemaDefinition> object_schema(allocator_);
  fidl::ObjectView<llcpp::fuchsia::shell::ObjectDefinition> object(allocator_);

  // Create a vector of nodes for the fields.
  std::vector<FidlNodeIdPair>& fields = object_stack_.back();
  object_schema->fields.Allocate(allocator_, fields.size());
  object->fields.Allocate(allocator_, fields.size());
  for (size_t i = 0; i < fields.size(); i++) {
    object_schema->fields[i] = fields[i].schema_id;
    object->fields[i] = fields[i].value_id;
  }

  // We construct an unnamed schema => local schema (only used by one object).
  NodeId schema_node_id =
      AddNode(llcpp::fuchsia::shell::Node::WithObjectSchema(object_schema), /*root_node=*/false);

  object->object_schema = schema_node_id;
  NodeId value_node_id = AddNode(llcpp::fuchsia::shell::Node::WithObject(object));

  object_stack_.pop_back();

  NodePair result;
  result.schema_node = schema_node_id;
  result.value_node = value_node_id;
  return result;
}

AstBuilder::NodePair AstBuilder::AddField(const std::string& key, NodeId expression_node_id,
                                          llcpp::fuchsia::shell::ShellType&& type) {
  NodePair result;

  // Create the type.
  fidl::ObjectView<llcpp::fuchsia::shell::ObjectFieldSchemaDefinition> field_schema(allocator_);
  field_schema->name.Set(allocator_, key);
  field_schema->type = std::move(type);
  result.schema_node = AddNode(llcpp::fuchsia::shell::Node::WithFieldSchema(field_schema));

  // Create the object
  fidl::ObjectView<llcpp::fuchsia::shell::ObjectFieldDefinition> field(allocator_);
  field->object_field_schema.file_id = file_id_;
  field->object_field_schema.node_id = result.schema_node.node_id;
  field->value = expression_node_id;
  result.value_node = AddNode(llcpp::fuchsia::shell::Node::WithObjectField(field));

  std::vector<FidlNodeIdPair>& fields = object_stack_.back();
  fields.emplace_back(result.schema_node, result.value_node);

  return result;
}

}  // namespace shell::console
