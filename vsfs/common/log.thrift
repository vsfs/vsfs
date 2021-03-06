// vim: ft=thrift:sw=2:sts=2
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

namespace cpp vsfs
namespace java vsfs
namespace py vsfs

/**
 * \brief A entity of log.
 */
struct LogEntity {
  1: i64 txn_id,  /// Transaction ID.
  2: binary data  /// The serialized data of updates.
}

/**
 * \brief A buffer of a sequence of entities (updates).
 */
struct LogBuffer {
  1: list<LogEntity> buffer
}
