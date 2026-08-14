# Lab Workflow

---

[toc]

---

Running MIT's [6.1810](https://pdos.csail.mit.edu/6.1810/2026/) labs from this repository: selecting a lab, merging its starter code, and grading your work.

## How labs work here

MIT distributes each lab as a separate branch of `xv6-labs-2025`. All nine branches were cut from the same base commit and are **independent siblings** — `util` is not an ancestor of `syscall`. Each branch contains stock xv6 plus that lab's starter files, grading script, and `#ifdef` guards.

This repository consolidates them: lab starter code is merged into `study` as you reach each lab, and your solutions accumulate on that one branch. The reasoning is in [00-repo-workflow.md](00-repo-workflow.md#why-one-branch-instead-of-one-per-lab).

| Lab         | Branch         | Grading script      | Topic                              |
| ----------- | -------------- | ------------------- | ---------------------------------- |
| **util**    | `labs/util`    | `grade-lab-util`    | Unix utilities, first system call  |
| **syscall** | `labs/syscall` | `grade-lab-syscall` | Adding system calls, tracing       |
| **pgtbl**   | `labs/pgtbl`   | `grade-lab-pgtbl`   | Page tables, superpages            |
| **traps**   | `labs/traps`   | `grade-lab-traps`   | Trap handling, backtraces, alarms  |
| **cow**     | `labs/cow`     | `grade-lab-cow`     | Copy-on-write fork                 |
| **net**     | `labs/net`     | `grade-lab-net`     | e1000 driver, UDP                  |
| **lock**    | `labs/lock`    | `grade-lab-lock`    | Lock contention, per-CPU freelists |
| **fs**      | `labs/fs`      | `grade-lab-fs`      | Large files, symbolic links        |
| **mmap**    | `labs/mmap`    | `grade-lab-mmap`    | Memory-mapped files                |

## The lab switch: `conf/lab.mk`

One line selects everything:

```makefile
LAB=util
```

That single variable drives three separate mechanisms:

1. **Conditional compilation.** The Makefile turns `LAB=util` into `-DLAB_UTIL -DSOL_UTIL`, which activates `#ifdef LAB_*` blocks in the kernel headers. For example `kernel/param.h` gives the util lab two user stack pages instead of one, and the fs lab a filesystem a hundred times larger.
2. **Extra build targets.** `ifeq ($(LAB),util)` blocks add per-lab user programs to `UPROGS` and data files to `UEXTRA`, so they land in `fs.img`.
3. **Grading.** `make grade` runs `./grade-lab-$(LAB)`.

> [!WARNING]
> Always `make clean` after changing `conf/lab.mk`. The lab name changes preprocessor defines, and Make cannot see that dependency — a stale object tree will silently test the wrong binary. `make grade` runs `make clean` for you for exactly this reason.

## Starting a lab

```bash
git fetch labs
git merge labs/syscall          # starter code, grading script, #ifdef guards
cat conf/lab.mk                 # confirm it now says LAB=syscall
make clean && make qemu
```

Expect conflicts on `conf/lab.mk` every time — it is a one-line file naming the active lab, and each branch names a different one. Resolve it by taking the incoming lab name.

Other conflicts should be rare after the first merge, because `rerere` replays your earlier resolutions. If you have not enabled it yet:

```bash
git config rerere.enabled true
```

## Grading

```bash
make grade                      # every test for the current lab
./grade-lab-util sleep          # a single test, by name
```

The harness (`gradelib.py`) boots QEMU for each test, drives the shell, and pattern-matches the output. A fresh, unimplemented lab scores zero — that is the correct starting state, not a broken setup:

```
== Test exec, recursive find ==
$ make qemu-gdb
exec, recursive find: FAIL (1.2s)
    Number of appearances of 'hello'
    got:
      0
    expected:
      3
    QEMU output saved to xv6.out.test_find_sh
Score: 0/131
```

Two things to note. Failures leave the full QEMU transcript in `xv6.out.<testname>` — read it, it usually shows the shell command that failed and exactly what your program printed. And the `$ make qemu-gdb` line is the harness reporting how it launched QEMU, not a command you need to run.

### The util lab's tests

15 tests, 131 points:

| Points | Tests                                                |
| ------ | ---------------------------------------------------- |
| **20** | `sleep` — no arguments, returns, makes syscall       |
| **30** | `sixfive` — test, readme, all                        |
| **20** | `memdump` — examples, and format `ii, S, p`          |
| **30** | `find` — current directory, sub-directory, recursive |
| **30** | `exec` — basic, multiple args, recursive find        |
| **1**  | `time` — reads `time.txt`, the hours you spent       |

The last one is why a full-marks run still fails if you have not created `time.txt`. Write a number into it:

```bash
echo 5 > time.txt
```

## Finishing a lab

Tag the snapshot, then push both:

```bash
git add -A && git commit -m "feat(util): implement find, sixfive, memdump"
git tag -a lab-util-done -m "util lab complete, 131/131"
git push origin study
git push origin lab-util-done
```

Tags are not pushed by `git push` alone — they need to be named explicitly, or `git push origin --tags` for all of them.

Afterwards you can always recover that state:

```bash
git diff lab-util-done                    # everything you have done since
git show lab-util-done:user/find.c        # that file, as it was then
```

## Re-running an earlier lab's grader

```bash
echo 'LAB=util' > conf/lab.mk
make grade
echo 'LAB=syscall' > conf/lab.mk          # switch back
make clean
```

> [!NOTE]
> Expect this to fail occasionally as you progress. Later labs change kernel internals that earlier graders assert on — the lock lab restructures `kalloc` and the buffer cache, cow rewrites page-fault handling. Sometimes that is a genuine regression worth fixing, and sometimes the later lab deliberately changed the behaviour the old grader checked. The transcript in `xv6.out.*` tells you which.

## Submission

`make zipball` produces `lab.zip` from the **committed** tree via `git archive`, so uncommitted work is excluded. It runs `submit-check` first, which verifies you are on a branch named after the lab.

In this consolidated repository you work on `study`, not on a branch called `util`, so that check will prompt you to confirm. Answering `y` is correct here.

This matters only if you are submitting to Gradescope as an enrolled student. For self-study, `make grade` is the whole feedback loop.

## Compatibility note

The lab branches were cut from xv6 as of 2025-08-25, and `study` tracks a newer upstream. One consequence has already been handled: upstream renamed `kernel/printf.c` to `kernel/printk.c`, so the Makefile's `OBJS_KCSAN` list was retargeted during the util merge. If a future lab merge fails to link with an error about a missing object, check whether upstream renamed the file and update the list the same way.
