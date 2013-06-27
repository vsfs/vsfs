/*
 * Copyright 2013 (c) Lei Xu <eddyxu@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VSFS_QUERY_AST_H_
#define VSFS_QUERY_AST_H_

#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

namespace vsfs {
namespace query {

/**
 * \class AST
 * \brief Basic class of Abstract Syntax Tree (AST).
 */
class AST {
 public:
  enum class Type {
    UNKNOWN,
    BINARY_OP,
  };

  /// Default constructor.
  AST();

  /// Destructor.
  virtual ~AST();

  /// Returns the type of the AST node.
  virtual Type type() const = 0;

  /// Returns true if this AST node is valid.
  virtual bool valid() const = 0;

  /// Returns the left child of this AST node. It can be nullptr.
  AST* left() const;

  /**
   * \brief Sets the new left child of this AST node.
   * \param l the new left child. Its ownership is transferred to this AST
   * node.
   *
   * The existed left child (and its sub-tree) will be deleted.
   */
  void set_left(AST* l);

  /// Returns the right child of this AST node. It can be nullptr.
  AST* right() const;

  /**
   * \brief Sets a new right child for this AST node.
   * \param r the new right child. Its ownership is transferred to this AST
   * node.
   *
   * The existed right child will be deleted.
   */
  void set_right(AST* r);

 protected:
  unique_ptr<AST> left_;
  unique_ptr<AST> right_;
};

/**
 * \class ExprAst "vsfs/query/ast.h"
 * \brief Expression AST node.
 */
class ExprAst : public AST {
 public:
  enum class ValueType {
    UNNOWN,
    BOOL,
    INT8,
    UINT8,
    INT16,
    UINT16,
    INT32,
    UINT32,
    INT64,
    UINT64,
    FLOAT,
    DOUBLE,
    STRING,
  };

  /// Returns the value type of this expression.
  virtual ValueType value_type() const = 0;
};

/**
 * \class BinaryOpAst "vsfs/query/ast.h"
 * \brief AST Node for a binary operator.
 */
class BinaryOpAst : public ExprAst {
 public:
  enum class OpCode {
    UNKNOWN,
    EQ,   /// Equal
    NEQ,  /// Not equal
    GT,   /// Greater than
    GE,   /// Greater or equal
    LT,   /// Less than
    LE,   /// Less or equal
    ADD,  /// Add
    SUB,  /// Substract
    MUL,  /// multiply
    DIV,  /// Divide
    MOD,  /// Module
    AND,  /// Logical and
    OR,   /// Logical or
    XOR,  /// Logical xor
  };

  /// Constructs a default BinaryOpAst.
  BinaryOpAst();

  /// Constructs a BinaryOpAst with given op code.
  explicit BinaryOpAst(OpCode op);

  /// Constructs a BinaryOpAst with the operator string.
  explicit BinaryOpAst(const string& opstr);

  /// Destructor
  virtual ~BinaryOpAst();

  /// Returns the AST node type.
  virtual Type type() const {
    return Type::BINARY_OP;
  }

  /// Returns the expression value type of this binary op.
  virtual ValueType value_type() const;

  /// Returns true if this binary op is valid.
  virtual bool valid() const;

  /// Returns the opcode.
  OpCode opcode() const;

  /// Sets the new opcode.
  void set_opcode(OpCode code);

 protected:
  OpCode opcode_;
};

}  // namespace query
}  // namespace vsfs

#endif  // VSFS_QUERY_AST_H_
