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

**vsfs index destroy** _DIR_ _NAME_

**vsfs index insert** [-s|--stdin] [-b|--batch NUM] _NAME_ [FILE KEY]...

**vsfs index remove** [-s|--stdin] [-b|--batch NUM] _NAME_ [FILE KEY]...

**vsfs index stat** _DIR_ _NAME_

**vsfs index list** [-r|--recursive] _DIR_

**vsfs search** _QUERY_


sth
