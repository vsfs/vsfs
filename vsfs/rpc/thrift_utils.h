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

#ifndef VSFS_RPC_THRIFT_UTILS_H_
#define VSFS_RPC_THRIFT_UTILS_H_

#include <boost/shared_ptr.hpp>
#include <glog/logging.h>
#include <protocol/TBinaryProtocol.h>
#include <transport/TBufferTransports.h>
#include <string>
#include "vobla/macros.h"

using std::string;

namespace vobla {
class Status;
}

namespace vsfs {
namespace rpc {

class RpcInvalidOp;

/**
 * \class ThriftUtils "vsfs/rpc/thrift_utils.h"
 * \brief Provides a set of utility functions to manipulate thrift-related
 * data structures.
 */
class ThriftUtils {
 public:
  /**
   * \brief Serialize a thrift struct.
   */
  template <typename Obj,
            typename Protocol = apache::thrift::protocol::TBinaryProtocol>
  static string serialize(const Obj &obj) {
    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer>
        buffer(new apache::thrift::transport::TMemoryBuffer);
    boost::shared_ptr<Protocol> protocol(new Protocol(buffer));
    obj.write(protocol.get());
    return buffer->getBufferAsString();
  }

  /**
   * \brief Deserialize an buffer to a object.
   * \return true if success.
   */
  template <typename Obj,
            typename Protocol = apache::thrift::protocol::TBinaryProtocol>
  static bool deserialize(const string &buf, Obj* obj) {
    CHECK_NOTNULL(obj);
    boost::shared_ptr<apache::thrift::transport::TMemoryBuffer>
        buffer(new apache::thrift::transport::TMemoryBuffer(
               reinterpret_cast<uint8_t*>(const_cast<char*>(buf.data())),
               buf.size()));
    boost::shared_ptr<Protocol> protocol(new Protocol(buffer));
    try {
      obj->read(protocol.get());
    } catch (...) {  // NOLINT
      return false;
    }
    return true;
  }

  /**
   * \brief Translate Status to RpcInvalidOp
   * Thrift RPC functions will throw RpcInvalidOp if something goes wrong.
   */
  static RpcInvalidOp StatusToRpcInvalidOp(const vobla::Status &status);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ThriftUtils);
};

}  // namespace rpc
}  // namespace vsfs

#endif  // VSFS_RPC_THRIFT_UTILS_H_
