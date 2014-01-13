Vsfs Introduction
=================

Vsfs (Versatile Searchable File System) is a distributed POSIX-compliant file
system that can do **real-time search**. Besides providing a command line
utility :doc:`vsfs(1) <man/vsfs.1>` to manipulate indices and search files,
most notably, Vsfs allows to search files directly as a format of file system
directory. We will describe the details later.

File Index
----------

VSFS' search function is built on top of the notion of *Verstaile File Index*
(*index* for short). An index in concept is a (multi-dementional) key-value
store, where the "`key`" is an arbitrary value to describe the file, and the
"`value`" is the corresponding file. Therefore, the file search can be
conducted by searching the keys from one or more indices.

The index scheme in VSFS is very flexible and schema free. An index in VSFS is
defined as a 4-elements tuple: *(dir, name, type, key)*.

* `dir`, is the directory path where to build the file index.
* `name`, a directory-wide unique name to describe and identify the index.
* `type` describes the data structure used for this index, e.g., ``btree`` and
  ``hash``. ``btree`` is favorable for *range query*, and ``hash`` is favorable
  for *point query*. *More index types are under development*.
* `key` describes the data type for the key used in index. It currently can be
  any integer values, ``float``, ``double`` and ``string``. For details, please
  refer to :doc:`vsfs(1) <man/vsfs.1>`.

Apparently, an index is identified by *(dir, name)*. Indices can be create and
destroy dynamically. Therefore it is flexible to the users to determine *when*
and *what* to be indices. The content of each index can be also filled
dynamically. For more details, please refer to :doc:`usage` section.

Any file under the directory `dir`, can be indexed into the index *(dir,
name)*, and therefor can be searched out from this index.

Search
------

.. caution::
    The File Query semantics are under heavily *research* and development, and
    thus subjects to change in the future.

VSFS supports search files by using *File Query Language*:

The current form:

.. code-block:: sh

    /path/prefix?name0>123&name1=321.0/

It searches the directory ``/path/prefix`` and all its sub-directories for
files that satisfy the key in ``name0`` index is larger than 123 and the key in
``name1`` index is equal to 321.0. Note that *the searched indices are not
necessary on directory ``/path/prefix``*.
