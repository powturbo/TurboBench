<!-- DRAFT: note to add to the GitHub Release v2.1.0. NOT PUBLISHED. -->

**Known issue (fixed in v2.2.0): concurrent API calls could return wrong output.**

In 2.1.0 the library kept per-call state (block size, decode error flag, DNA hint) in process
globals. When several threads call `aceapex_compress` / `aceapex_decompress` at the same time -
lzbench with `-T`, a server, a thread pool - one call can pick up another call's block size or
error state. In a test with two threads over the 4569 files of lzbench's source tree, 2.1.0
returned wrong bytes for 22 files. Single-threaded use (one call at a time, including the CLI and
calls with an internal thread budget) is not affected, and archives written that way are valid.

Upgrade to v2.2.0, where the state is per call (claim `head_api_concurrent` in the judge). If you
must stay on 2.1.0, serialize calls into the library with a mutex. The aceapex copy in lzbench 2.4
("aceapex 1.0.1") is not affected.
