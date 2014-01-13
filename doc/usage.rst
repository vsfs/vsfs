Usage
-------------

VSFS provides a command line utility :doc:`vsfs(1) <man/vsfs.1>` to manipulate
indices. You can use it to *create/destroy* file indices and *insert/remove*
records from given indices.

.. code-block:: sh

    # Index two files with float keys.
    $ vsfs index insert energy /foo/bar/file0 100 /foo/bar/file1 201.2

    # Index two files with string keys. Each file has space(s) in its file path.
    $ vsfs index insert "string index" "/foo/bar/drug 100" "mark 100" \
      "/foo/another bar/drug-101" "invalid value"

    # Feeds index through pipeline.
    $ gen_index.py | vsfs index insert --stdin energy
    # or ...
    $ cat /path/preprocessed-symbols.txt | vsfs index insert --stdin symbols

    # Run program on the search result directory:
    $ mvd "/foo/bar/?energy>114.5"


For more details, please refer to man :doc:`vsfs(1) <man/vsfs.1>`.
