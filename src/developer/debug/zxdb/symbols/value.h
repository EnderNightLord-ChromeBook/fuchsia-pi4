// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_VALUE_H_
#define SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_VALUE_H_

#include <string>

#include "src/developer/debug/zxdb/symbols/symbol.h"

namespace zxdb {

// A value is the base class for data with names: Variable objects (for stack vars and function
// parameters), and DataMember objects (for struct/class members).
class Value : public Symbol {
 public:
  // Don't construct by itself, used as a base class for Variable and DataMember.

  // Symbol overrides.
  const Value* AsValue() const override;
  const std::string& GetAssignedName() const final { return assigned_name_; }

  // The name of the variable, parameter, or member name. See
  // Symbol::GetAssignedName().
  void set_assigned_name(const std::string& n) { assigned_name_ = n; }

  const LazySymbol& type() const { return type_; }
  void set_type(const LazySymbol& t) { type_ = t; }

  // Artificial values are ones generated by the compiler that don't appear in the source. The most
  // common example is "this" parameters to functions.  Other examples are GCC-generated "__func__"
  // variables and the discriminant data member in a rust enum.
  bool artificial() const { return artificial_; }
  void set_artificial(bool a) { artificial_ = a; }

  // This could add the decl_file/line if we need it since normally such entries have this
  // information.

 protected:
  explicit Value(DwarfTag tag);
  Value(DwarfTag tag, const std::string& assigned_name, LazySymbol type);
  ~Value();

 private:
  std::string assigned_name_;
  LazySymbol type_;
  bool artificial_ = false;
};

}  // namespace zxdb

#endif  // SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_VALUE_H_
