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

using std::unique_ptr;

namespace vsfs {
namespace query {

/**
 * \class AST
 * \brief Basic class of Abstract Syntax Tree (AST).
 */
class AST {
 public:
  AST();

  virtual ~AST();

  virtual int type() = 0;

  AST* left() const;

  void set_left(AST* l);

  AST* right() const;

  void set_right(AST* r);

 private:
  unique_ptr<AST> left_;
  unique_ptr<AST> right_;
};

class BinaryOpAst : public AST {
};

}  // namespace query
}  // namespace vsfs

#endif  // VSFS_QUERY_AST_H_
