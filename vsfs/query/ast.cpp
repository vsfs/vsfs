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

#include <string>
#include "vsfs/query/ast.h"

using std::string;

namespace vsfs {
namespace query {

AST::AST() {
}

AST::~AST() {
}

AST* AST::left() const {
  return left_.get();
}

void AST::set_left(AST* l) {
  left_.reset(l);
}

AST* AST::right() const {
  return right_.get();
}

namespace {

BinaryOpAst::OpCode parse_op(const string& op) {
  if (op == "=") {
    return BinaryOpAst::OpCode::EQ;
  } else if (op == "!=") {
    return BinaryOpAst::OpCode::NEQ;
  } else if (op == ">") {
    return BinaryOpAst::OpCode::GT;
  } else if (op == ">=") {
    return BinaryOpAst::OpCode::GE;
  } else if (op == "<") {
    return BinaryOpAst::OpCode::LT;
  } else if (op == "<=") {
    return BinaryOpAst::OpCode::LE;
  } else if (op == "+") {
    return BinaryOpAst::OpCode::ADD;
  } else if (op == "-") {
    return BinaryOpAst::OpCode::SUB;
  } else if (op == "*") {
    return BinaryOpAst::OpCode::MULTIPLY;
  } else if (op == "/") {
    return BinaryOpAst::OpCode::DIVIDE;
  } else if (op == "&") {
    return BinaryOpAst::OpCode::AND;
  } else if (op == "|") {
    return BinaryOpAst::OpCode::OR;
  } else if (op == "^") {
    return BinaryOpAst::OpCode::XOR;
  } else {
    return BinaryOpAst::OpCode::UNKNOWN;
  }
}

}  // namespace

BinaryOpAst::BinaryOpAst() : opcode_(OpCode::UNKNOWN) {
}

BinaryOpAst::BinaryOpAst(OpCode code) : opcode_(code) {
}

BinaryOpAst::BinaryOpAst(const string& opstr) : opcode_(parse_op(opstr)) {
}

}  // namespace query
}  // namespace vsfs
