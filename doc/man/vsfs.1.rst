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

**vsfs** is the command line utility to manipulate VSFS file system.

**vsfs index create** [--key KEY] [--type TYPE] `DIR NAME`

**vsfs index destroy** `DIR` `NAME`

**vsfs index insert** [-s|--stdin] [-b|--batch NUM] `NAME` [FILE KEY]...

**vsfs index remove** [-s|--stdin] [-b|--batch NUM] `NAME` [FILE KEY]...

**vsfs index stat** `DIR` `NAME`

**vsfs index list** [-r|--recursive] `DIR`

**vsfs search** `QUERY`

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

Create a file index on directory `DIR`:

**vsfs index create** [--key KEY] [--type TYPE] `DIR` `NAME`

-k, --key=<KEY>            Choose the data type of the key, currently it supports `int8, int16, int32, int64, float, double, string`.

-t, --type=<TYPE>       Choose the structure of index (`btree` and `hash`).  Optional, default value is `btree`. `Btree` index is favorable for range query, and `hash` index is favorable for point query.

Delete a file index on directory `DIR` with `NAME`:

**vsfs index delete** `DIR` `NAME`:

Insert records into a file index:

There are two ways to insert/update records into VSFS: 1) specify the file path
and its corrensponding keys from parameters and 2) feed the index records from
the pipeline. If there are spaces in file path or keys, you should put them
in quotation marks.

  * **vsfs index insert** [`options`] `NAME` `FILE` `KEY` [`FILE` `KEY`]...
     Directly specify files and its key in command line parameters.

  * **vsfs index insert** [`options`] --stdin `NAME`
	Use pipeline to feed file index. vsfs accepts file index records (e.g., a
	(`path, key`) pair) from stdin. Each line in stdin is considered as
	one record with format "/file/path key".

Options:

-b, --batch=NUM           Sets the batch size of sending index records.

ENVIRONMENT
===========

* `VSFS_HOST`:
  The hostname of VSFS primary master.

* `VSFS_PORT`:
  The TCP port number of VSFS primary master.

FILES
=====

*$HOME/.config/vsfs.json*

SEE ALSO
========

vsfs(8)
