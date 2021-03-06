Setup VSFS Cluster
==================

.. caution::

    - VSFS is under heavily development. The content of this document is
      subjected to change in future.
    - VSFS is not ready to be deployed in production system.

Vsfs consists with 3 major components: ``masterd``, ``indexd``, ``mount.vsfs``.
These components can run on Linux. The offical supported distributions are
`CentOS`, `Debian` and `Ubuntu`.

Masterd
--------

Masterd Cluster is the metadata cluster of VSFS, which includes one *primary*
masterd, and zero or more *secondary* masterd

.. code-block:: sh

    # Start primary master.
    /path/to/masterd -primary -dir /path/to/metadata/db --daemon

    # Start additional master.
    /path/to/masterd -primary_host HOST -primary_port PORT -dir /path/to/metadata/db --daemon

Indexd
-------

Indexd cluster is the file index store cluster of VSFS.

.. code-block:: sh

    # Start indexd
    /path/to/indexd -master_addr PRIMARY_MASTER_HOST -master_port PRIMARY_MASTER_PORT \
    -datadir /path/to/index/store -daemon


.. caution::
    * Masterd and Indexd clusters are two Consistent Hashing Rings. Thus the
      metadata and file indices are well balanced and *stastically* distributed
      on each server. However, the data migration process between servers has
      not been implemented (yet). Therefore, in this early stage, both clusters
      could not dynamically change the size. *Please plan the system
      architecture accordingly.*
    * For the similar reason, the `datadir` for each `masterd` and `indexd` must
      be different. By locating them on shared storage (e.g., NFS), it will be
      easiler to recovery masterd or indexd from a different physical server.

Mount VSFS on worker nodes
--------------------------

Current, vsfs client-side file system is implemented through FUSE, which means
that you have to mount vsfs through FUSE on your worker nodes, similar to
mounting NFS or Lustre for home directory on worker nodes.

Currently, VSFS delegates the responsbility of managing raw data to *Storage
Backend*. Two storage backends are supported now:

 * `Posix store`, it completely maps the directory structure on another file
   system. Therefore, you can mount it to a NFS share. **Posix store** is the
   default storage backend in VSFS. It is suggested to mount the posix store on
   a persistant and shared file system (e.g., NFS or Lustre), therefore the data
   can still be accessible if masterd is corrupted, considering it is the early
   phase of VSFS development.
 * `Object store`. It hashes file path to layouted directory structure for
   better metadata performance. For example, it creates 8192 sub-directories by
   default on the base directory and put the files into the directory that has
   name for "`md5(file_path) mod 8192`". However, it could not recover from a corrupt
   VSFS metadata server. *Object store is NOT suggested to be used in
   PRODUCTION environment.*

.. code-block:: sh

    # Mount fuse to /mnt/point
    /path/to/mount.vsfs --basedir /base/path -H PRIAMRY_MASTER_HOST \
    -p PRIMARY_MASTER_PORT /mnt/point
