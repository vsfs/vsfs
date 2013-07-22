/**
 * \file index_controller_interface.h
 *
 * \brief The interface of IndexController.
 *
 * It can either be used for real-implementation or the mock class.
 *
 * Copyright 2012 (c) Lei Xu <eddyxu@gmail.com>
 */

#ifndef VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_
#define VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_

#include <string>
#include "vobla/status.h"

using std::string;
using vobla::Status;

namespace vsfs {
namespace indexd {

/**
 * \class IndexControllerInterface
 * \brief Interface for the index controller.
 */
class IndexControllerInterface {
 public:
  virtual ~IndexControllerInterface() {}

  /**
   * \brief Creates a file-index with a name.
   * \param root_path the path to create the file-index for.
   * \param name the name of the index.
   * \param index_type The type of the index (internal data structure). It
   * could be one of the BTREE/HASH or etc.
   * \param key_type the c++ type of the key.
   */
  virtual Status create_index(const string &root_path, const string &name,
                              int index_type, int key_type) = 0;
};

}  // namespace indexd
}  // namespace vsfs

#endif  // VSFS_INDEXD_INDEX_CONTROLLER_INTERFACE_H_
