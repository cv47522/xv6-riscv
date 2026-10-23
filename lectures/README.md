# MIT 6.1810 lecture archive

Fetched from the [2026 course schedule](https://pdos.csail.mit.edu/6.1810/2026/schedule.html) on 2026-10-23. This archive contains 81 downloaded files: 22 published lecture texts, four FAQs, example programs and their headers/build files, the 16550 UART manual, directory listings, and the raw schedule snapshot. [Study progress](../STUDY-PROGRESS.md) tracks learning separately from downloading.

## Provenance and refresh

- Downloaded files retain their upstream bytes, comments, formatting, and copyright notices. They are reference material; author your practice versions directly under [user/](../user/) using this repository’s conventions.
- The course warns that future material may still come from 2025. Lecture 19 (Verification) has no linked text yet, so no substitute file was invented.
- [manifest.json](manifest.json) records each original URL, resolved URL, retrieval time in UTC, Last-Modified value when available, byte count, SHA-256 hash, and discovery source. Download failures are recorded there; this snapshot has none.
- [schedule.html](schedule.html) and directory `index.html` files are unmodified source snapshots. Use this Markdown inventory for local navigation; the snapshots retain site navigation, styling dependencies, and upstream base URLs.
- The three former texts in `docs/lectures/` now live here and were refreshed from MIT. The exercise guide and reusable exercise prompt link to this location.
- To refresh, revisit the schedule and the lecture-text links first, retrieve the resources from their recorded URLs, and update the manifest hashes and timestamps. Preserve your study checkboxes and distinguish publication changes from learning progress.
- Attribution: MIT 6.1810 course staff and the authors named in each source. The [course page](https://pdos.csail.mit.edu/6.1810/2026/schedule.html) links to [Creative Commons Attribution 3.0 US](https://creativecommons.org/licenses/by/3.0/us/); preserve any file-specific notices.

## Example sets and practice targets

| Material | Local files | Practice context |
| --- | --- | --- |
| **Lecture 1: Unix interfaces** | [ex1.c](l-overview/ex1.c) through [ex9.c](l-overview/ex9.c), plus [echo.c](l-overview/echo.c) | Already practised as [user/ex1copy.c](../user/ex1copy.c) through [user/ex9ls.c](../user/ex9ls.c); see the [exercise guide](../docs/07-exercises.md). |
| **Lecture 2: C and memory** | [ptr.c](l-c/ptr.c), [array.c](l-c/array.c), [str.c](l-c/str.c) | These are xv6 examples; create distinct practice programs directly under user/. |
| **Lecture 10: application VM** | [mmap.c](mmap.c), [sqrts.c](sqrts.c), and [rfork/](rfork/) | The schedule links mmap.c; the text names sqrts.c. These and rfork use host facilities; an xv6 version needs an explicit adaptation to its available calls. |
| **Lecture 13: switching** | [aaa.c](aaa.c), [bbb.c](bbb.c) | Links occur inside l-threads.txt; these are xv6 scheduling examples. |
| **Lecture 14: coordination** | [l-coordination.c](l-coordination.c) | Companion kernel excerpts published beside the text, including a deliberately broken sleep example; study them against kernel/uart.c and kernel/proc.c. |
| **Lecture 20: RCU** | [rcu/](rcu/) with source, headers, and Makefile | Named inside l-rcu.txt; host threading examples, not direct xv6 user-program drop-ins. |
| **Lecture 22: BPF** | [bpf/](bpf/) and [bpf/engine/](bpf/engine/) | l-bpf.txt explicitly links the 2025 demo directory, so this set retains that exact provenance. It includes host/Linux and architecture-specific code. |

Some examples depend on host libc, signals, virtual-memory facilities, threading, Linux BPF, or a particular instruction set. Read them here, then select the part you can reproduce with [user/user.h](../user/user.h); keep practice files flat and follow the [user-program workflow](../docs/02-build-boot-and-usage.md#adding-and-running-a-user-exercise). The root xv6 build does not include these archive files.

## Downloaded file inventory

Every downloaded file has a relative link and an exact remote link. Example-directory listings are included to document what was available at retrieval time. The BPF entries alone use the explicitly linked 2025 directory; all other downloaded files use the 2026 course site.

| Saved file | Remote source |
| --- | --- |
| [`16550.pdf`](16550.pdf) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/16550.pdf) |
| [`aaa.c`](aaa.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/aaa.c) |
| [`bbb.c`](bbb.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/bbb.c) |
| [`bpf/Makefile`](bpf/Makefile) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/Makefile) |
| [`bpf/drop_icmp.c`](bpf/drop_icmp.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/drop_icmp.c) |
| [`bpf/engine/Makefile`](bpf/engine/Makefile) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/Makefile) |
| [`bpf/engine/bpf.h`](bpf/engine/bpf.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/bpf.h) |
| [`bpf/engine/fast_interp.c`](bpf/engine/fast_interp.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/fast_interp.c) |
| [`bpf/engine/index.html`](bpf/engine/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/) |
| [`bpf/engine/interp.c`](bpf/engine/interp.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/interp.c) |
| [`bpf/engine/jit.c`](bpf/engine/jit.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/jit.c) |
| [`bpf/engine/main.c`](bpf/engine/main.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/main.c) |
| [`bpf/engine/snippets.S`](bpf/engine/snippets.S) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/snippets.S) |
| [`bpf/engine/util.c`](bpf/engine/util.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/engine/util.c) |
| [`bpf/genseccomp.c`](bpf/genseccomp.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/genseccomp.c) |
| [`bpf/index.html`](bpf/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/) |
| [`bpf/set_ttl.c`](bpf/set_ttl.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2025/lec/bpf/set_ttl.c) |
| [`l-bpf.txt`](l-bpf.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-bpf.txt) |
| [`l-c.txt`](l-c.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c.txt) |
| [`l-c/array.c`](l-c/array.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/array.c) |
| [`l-c/index.html`](l-c/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/) |
| [`l-c/ptr.c`](l-c/ptr.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/ptr.c) |
| [`l-c/str.c`](l-c/str.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-c/str.c) |
| [`l-coordination.c`](l-coordination.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-coordination.c) |
| [`l-coordination.txt`](l-coordination.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-coordination.txt) |
| [`l-crash.txt`](l-crash.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-crash.txt) |
| [`l-fs.txt`](l-fs.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-fs.txt) |
| [`l-internal.txt`](l-internal.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-internal.txt) |
| [`l-interrupt.txt`](l-interrupt.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-interrupt.txt) |
| [`l-isolation.txt`](l-isolation.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-isolation.txt) |
| [`l-janus.txt`](l-janus.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-janus.txt) |
| [`l-lockv2.txt`](l-lockv2.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-lockv2.txt) |
| [`l-meltdown.txt`](l-meltdown.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-meltdown.txt) |
| [`l-net.txt`](l-net.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-net.txt) |
| [`l-organization.txt`](l-organization.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-organization.txt) |
| [`l-os.txt`](l-os.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-os.txt) |
| [`l-overview.txt`](l-overview.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview.txt) |
| [`l-overview/echo.c`](l-overview/echo.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/echo.c) |
| [`l-overview/ex1.c`](l-overview/ex1.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c) |
| [`l-overview/ex2.c`](l-overview/ex2.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex2.c) |
| [`l-overview/ex3.c`](l-overview/ex3.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex3.c) |
| [`l-overview/ex4.c`](l-overview/ex4.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex4.c) |
| [`l-overview/ex5.c`](l-overview/ex5.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex5.c) |
| [`l-overview/ex6.c`](l-overview/ex6.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex6.c) |
| [`l-overview/ex7.c`](l-overview/ex7.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex7.c) |
| [`l-overview/ex8.c`](l-overview/ex8.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex8.c) |
| [`l-overview/ex9.c`](l-overview/ex9.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex9.c) |
| [`l-overview/index.html`](l-overview/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/) |
| [`l-pgfaults.txt`](l-pgfaults.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-pgfaults.txt) |
| [`l-rcu.txt`](l-rcu.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-rcu.txt) |
| [`l-shenango.txt`](l-shenango.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-shenango.txt) |
| [`l-superpages.txt`](l-superpages.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-superpages.txt) |
| [`l-threads.txt`](l-threads.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-threads.txt) |
| [`l-uservm.txt`](l-uservm.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-uservm.txt) |
| [`l-vm.txt`](l-vm.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l-vm.txt) |
| [`l4-faq.txt`](l4-faq.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/l4-faq.txt) |
| [`livelock-faq.txt`](livelock-faq.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/livelock-faq.txt) |
| [`meltdown-faq.txt`](meltdown-faq.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/meltdown-faq.txt) |
| [`mmap.c`](mmap.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/mmap.c) |
| [`rcu/Makefile`](rcu/Makefile) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/Makefile) |
| [`rcu/clist.c`](rcu/clist.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/clist.c) |
| [`rcu/index.html`](rcu/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/) |
| [`rcu/rcu.c`](rcu/rcu.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/rcu.c) |
| [`rcu/rcu.h`](rcu/rcu.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/rcu.h) |
| [`rcu/rwlock.c`](rcu/rwlock.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/rwlock.c) |
| [`rcu/rwlock.h`](rcu/rwlock.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/rwlock.h) |
| [`rcu/slock.c`](rcu/slock.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/slock.c) |
| [`rcu/slock.h`](rcu/slock.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rcu/slock.h) |
| [`rfork/Makefile`](rfork/Makefile) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/Makefile) |
| [`rfork/demo.c`](rfork/demo.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/demo.c) |
| [`rfork/index.html`](rfork/index.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork) |
| [`rfork/rfork.c`](rfork/rfork.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/rfork.c) |
| [`rfork/rfork.h`](rfork/rfork.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/rfork.h) |
| [`rfork/rfork_ctx.S`](rfork/rfork_ctx.S) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/rfork_ctx.S) |
| [`rfork/rfork_proto.h`](rfork/rfork_proto.h) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/rfork_proto.h) |
| [`rfork/rforkd.c`](rfork/rforkd.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/rforkd.c) |
| [`rfork/stub.c`](rfork/stub.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/stub.c) |
| [`rfork/stub_start.S`](rfork/stub_start.S) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/rfork/stub_start.S) |
| [`schedule.html`](schedule.html) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/schedule.html) |
| [`sqrts.c`](sqrts.c) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/sqrts.c) |
| [`uservm-faq.txt`](uservm-faq.txt) | [MIT original](https://pdos.csail.mit.edu/6.1810/2026/lec/uservm-faq.txt) |
