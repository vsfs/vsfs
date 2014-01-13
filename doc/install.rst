Installation
============

Vsfs consists with 3 major components: ``masterd``, ``indexd``, ``fuse-client``.

Masterd
--------

Masterd Cluster is the metadata cluster of VSFS, which includes one *primary*
masterd, and zero or more *secondary* masterd

.. code-block:: sh

    # Start primary master.
    /path/to/masterd -primary -dir /path/to/metadata/db -d
