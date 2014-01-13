====
vsfs
====

--------------------------
VSFS Command Line Utility
--------------------------

:Author: dev@getvsfs.com
:Version: 0.0.1
:Manual section: 1
:Manual group: VSFS Manual

SYMOPSIS
========

**vsfs** `command` [ARGS] ...


DESCRIPTION
===========

**vsfs** is the command line utility to manipulate VSFS file system and index
store.

**vsfs index create** [--key KEY] [--type TYPE] `DIR NAME`

**vsfs index destroy** `DIR` `NAME`

**vsfs index insert** [-s|--stdin] [-b|--batch NUM] `NAME` [FILE KEY]...

**vsfs index remove** [-s|--stdin] [-b|--batch NUM] `NAME` [FILE KEY]...

**vsfs index stat** `DIR` `NAME`
  (unimplemeted)

**vsfs index list** [-r|--recursive] `DIR`

**vsfs search** `QUERY`

**vsfs help**

OPTIONS
============

-h, --help              Display this help message.
-H, --host=<host>       Specify the host of VSFS master node.
-p, --port=<port>       Specify the port number of VSFS master node (default: 9987).

File index operations and options
=================================

A file index in VSFS is identified by its location (i.e. `DIR`) and name (i.g.,
`NAME`), while it is possible to specify each index performance characteristics
independantly when create this index.


**vsfs index create** [--key KEY] [--type TYPE] `DIR` `NAME`
  Create a file index on directory `DIR` with `NAME`. The `NAME` must be unique
  within one directory. However, there is no *theorical* limit on the number of
  indices with different names on the same directory.


-k, --key=<KEY>         Choose the data type of the key, currently it supports `int8, int16, int32, int64, float, double, string`.

-t, --type=<TYPE>       Choose the structure of index (`btree` and `hash`).  Optional, default value is `btree`. `Btree` index is favorable for range query, and `hash` index is favorable for point query.


**vsfs index delete** `DIR` `NAME`:
  Delete the file index with `NAME` on directory `DIR`.

Insert records into a file index:

There are two ways to insert/update records into VSFS:

  * **vsfs index insert** [`options`] `NAME` `FILE` `KEY` [`FILE` `KEY`]...
     Directly specify files and its key in command line parameters.

  * **vsfs index insert** [`options`] \-\-stdin `NAME`
	Use pipeline to feed file index. vsfs accepts file index records (e.g., a
	(`path, key`) pair) from stdin. Each line in stdin is considered as
	one record with format "/file/path key".

If there are spaces in file path or key, you should put them in quotation marks.

Options:

-b, --batch=NUM           Sets the batch size of sending index records.
-s, --stdin               Use stdin to feed index records.

ENVIRONMENT
===========

* `VSFS_HOST`:
  The hostname of VSFS primary master.

* `VSFS_PORT`:
  The TCP port number of VSFS primary master.

FILES
=====

*$HOME/.config/vsfs.json*    (unimplemeted)

SEE ALSO
========

vsfs(8)
