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

#include <gtest/gtest.h>
#include "vsfs/query/ast.h"

namespace vsfs {
namespace query {

TEST(BinaryOpAstTest, TestParseByOpStr) {
  EXPECT_EQ(BinaryOpAst::OpCode::UNKNOWN, BinaryOpAst().opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::EQ, BinaryOpAst("=").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::NEQ, BinaryOpAst("!=").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::GT, BinaryOpAst(">").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::GE, BinaryOpAst(">=").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::LT, BinaryOpAst("<").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::LE, BinaryOpAst("<=").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::ADD, BinaryOpAst("+").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::SUB, BinaryOpAst("-").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::MUL, BinaryOpAst("*").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::DIV, BinaryOpAst("/").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::MOD, BinaryOpAst("%").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::AND, BinaryOpAst("&").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::OR, BinaryOpAst("|").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::XOR, BinaryOpAst("^").opcode());
  EXPECT_EQ(BinaryOpAst::OpCode::UNKNOWN, BinaryOpAst("<>").opcode());
}

}  // namespace query
}  // namespace vsfs
