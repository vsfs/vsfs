// vim:ts=2:sw=2:sts=2:et:ft=thrift
namespace cpp vsfs.rpc.testing

struct Directory {
  1: string name,
  2: i64 mode,
  3: i32 uid,
  4: i32 gid,
  5: set<string> subdirs,
}
