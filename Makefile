#
# ============================================================================
#  xv6-riscv build system
# ============================================================================
#
#  What this Makefile produces, end to end:
#
#    1. kernel/kernel   an ELF executable, cross-compiled for RISC-V, linked
#                       at a fixed physical address by kernel/kernel.ld.  QEMU
#                       loads it directly -- there is no bootloader.
#    2. user/_NAME      one standalone ELF per user program (the leading
#                       underscore distinguishes the linked binary from the
#                       .o file that Make also keeps around).
#    3. fs.img          a disk image built by mkfs/mkfs, containing all the
#                       user/_NAME binaries plus README.  This becomes xv6's
#                       root filesystem.
#
#  Then `make qemu` boots QEMU with kernel/kernel as the kernel and fs.img as
#  a virtio disk.
#
#  KEY IDEA -- CROSS COMPILATION: your host is x86-64, but every kernel and
#  user object here is compiled for RISC-V.  Nothing built for xv6 can run on
#  your machine, and nothing from your host's libc can be linked in.  That
#  single fact explains most of the unusual flags below (-ffreestanding,
#  -nostdlib, the long -fno-builtin-* list, -mcmodel=medany).
#
# ============================================================================

# ---------------------------------------------------------------------------
#  Lab selection
# ---------------------------------------------------------------------------
#  conf/lab.mk is a one-line file containing e.g. `LAB=util`.  The leading `-`
#  on -include means "include it if it exists, stay silent if it does not", so
#  plain xv6 (no lab) still builds.  Setting LAB has three effects:
#    * -DLAB_UTIL / -DSOL_UTIL get added to CFLAGS (see the ifdef LAB block),
#      which switches on the #ifdef LAB_* code in kernel/param.h, riscv.h, ...
#    * extra per-lab user programs and kernel objects get built (the many
#      `ifeq ($(LAB),...)` blocks below)
#    * `make grade` runs ./grade-lab-$(LAB)
#  To switch labs, edit conf/lab.mk and `make clean` before rebuilding.
-include conf/lab.mk

# Short aliases used throughout, purely to keep the lists below readable.
K=kernel
U=user

# ---------------------------------------------------------------------------
#  Kernel object files
# ---------------------------------------------------------------------------
#  Every .o linked into kernel/kernel.  entry.o must come first: it holds
#  _entry, the very first instruction QEMU jumps to, and kernel.ld places it
#  at the start of the image.
OBJS = \
  $K/entry.o \
  $K/kalloc.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o

# ---------------------------------------------------------------------------
#  Kernel objects that must NOT be instrumented
# ---------------------------------------------------------------------------
#  KCSAN is the kernel concurrency sanitiser (a race detector).  When it is
#  enabled, every instrumented function calls back into the KCSAN runtime.
#  These five files cannot tolerate that: they either run before the kernel is
#  fully initialised (start, entry-time console/uart output) or they *are* the
#  machinery KCSAN itself depends on (spinlock, printk) -- instrumenting them
#  would recurse infinitely.  So they live in a separate list and are exempt
#  from EXTRAFLAG (see `$(OBJS): EXTRAFLAG := $(KCSANFLAG)` further down).
#
#  Note: this is printk.o, not printf.o.  Upstream renamed kernel/printf.c to
#  kernel/printk.c after the 2025 lab branches were cut, so the lab Makefile's
#  original printf.o entry was retargeted here during the merge.
OBJS_KCSAN = \
  $K/start.o \
  $K/console.o \
  $K/printk.o \
  $K/uart.o \
  $K/spinlock.o

# Built only with `make KCSAN=1`; kernel/kcsan.c is the sanitiser runtime and
# only exists on lab branches that use it.
ifdef KCSAN
OBJS_KCSAN += \
	$K/kcsan.o
endif

# --- Per-lab extra kernel objects ------------------------------------------
# The lock lab adds per-CPU counters and a small sprintf for reporting them.
ifeq ($(LAB),lock)
OBJS += \
	$K/stats.o\
	$K/sprintf.o
endif


# The net lab adds an e1000 NIC driver, a tiny IP/UDP stack, and PCI probing.
ifeq ($(LAB),net)
OBJS += \
	$K/e1000.o \
	$K/net.o \
	$K/pci.o
endif


# ---------------------------------------------------------------------------
#  Toolchain detection
# ---------------------------------------------------------------------------
#  TOOLPREFIX is the prefix on every cross tool, e.g. `riscv64-linux-gnu-`
#  turns `gcc` into `riscv64-linux-gnu-gcc`.  Uncomment and set it by hand if
#  your toolchain lives somewhere unusual (e.g. /opt/riscv/bin/riscv64-unknown-elf-).
# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
#TOOLPREFIX =

#  Otherwise probe for a working cross toolchain by running each candidate
#  objdump and checking that it understands 64-bit RISC-V ELF.  The `elf64-big`
#  string is just a cheap marker that appears in `objdump -i` output for a
#  64-bit-capable binutils; if the tool is missing, the shell command fails and
#  the grep finds nothing, so the next candidate is tried.
#
#  Bare-metal prefixes (*-elf-) are preferred over the Linux-targeted one
#  because they default to no libc and no OS assumptions.  The Linux prefix
#  works fine too -- that is what Debian/Ubuntu's gcc-riscv64-linux-gnu package
#  installs, and what the -nostdlib/-ffreestanding flags below neutralise.
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-elf-'; \
	elif riscv64-none-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-none-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# ---------------------------------------------------------------------------
#  Emulator
# ---------------------------------------------------------------------------
#  qemu-system-riscv64 emulates a whole 64-bit RISC-V machine.  7.2 is the
#  floor because older builds mishandle the modern (non-legacy) virtio-mmio
#  interface that xv6's disk driver now requires; check-qemu-version at the
#  bottom enforces this before `make qemu` runs.
QEMU = qemu-system-riscv64
MIN_QEMU_VERSION = 7.2

# The cross tools.  Expands to e.g. riscv64-linux-gnu-gcc.
CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# ---------------------------------------------------------------------------
#  Compiler flags
# ---------------------------------------------------------------------------
#  -Wall -Werror              every warning, and warnings are fatal.  xv6 is
#                             small enough to stay warning-clean, and in kernel
#                             code a warning is usually a real bug.
#  -Wno-unknown-attributes    tolerate attributes this gcc does not know about
#                             (used by the clang-oriented annotations upstream
#                             added); without it -Werror would reject them.
#  -O                         optimise.  Not -O0: xv6 relies on modest
#                             optimisation for some inline assembly and stack
#                             layout assumptions, and -O0 code is much larger.
#  -fno-omit-frame-pointer    keep the frame pointer in s0 so stack backtraces
#                             work.  kernel/riscv.h's r_fp() reads s0 directly,
#                             which is exactly what the traps lab's backtrace
#                             exercise depends on.
#  -ggdb -gdwarf-2            debug info for gdb, in the DWARF 2 flavour that
#                             the course's gdb setup expects.
CFLAGS = -Wall -Werror -Wno-unknown-attributes -O -fno-omit-frame-pointer -ggdb -gdwarf-2

#  -march=rv64gc  target the 64-bit RISC-V "GC" profile: the general-purpose
#  base integer ISA plus the standard extensions bundled as G (integer
#  multiply/divide, atomics, and float/double) plus C (16-bit compressed
#  instructions, which shrink the kernel noticeably).
CFLAGS += -march=rv64gc

#  C99 plus GNU extensions -- xv6 uses a few GNU-isms such as statement
#  expressions and inline asm syntax that strict -std=c99 would reject.
CFLAGS += -std=gnu99

#  When conf/lab.mk sets LAB=util, this defines -DLAB_UTIL and -DSOL_UTIL.
#  LAB_* guards the starter-code differences in kernel headers (e.g. the
#  larger USERSTACK the util lab needs); SOL_* guards reference-solution code
#  in MIT's private tree and is harmless here.  XCFLAGS is kept separate from
#  CFLAGS because the host-compiled mkfs and the notxv6 pthread programs need
#  the same -D flags but none of the freestanding RISC-V ones.
ifdef LAB
LABUPPER = $(shell echo $(LAB) | tr a-z A-Z)
XCFLAGS += -DSOL_$(LABUPPER) -DLAB_$(LABUPPER)
endif

CFLAGS += $(XCFLAGS)

#  -MD  emit a .d file beside each .o listing that source's header
#  dependencies.  Those files are pulled back in by the `-include kernel/*.d
#  user/*.d` line further down, which is what makes editing a header trigger
#  rebuilds of everything that includes it.
CFLAGS += -MD

#  -mcmodel=medany  the "medium-any" code model.  It generates PC-relative
#  addressing with a +/-2 GiB reach, so the same code works whatever address
#  it is linked at.  xv6 needs this because the kernel is linked at
#  0x80000000, far outside the low 2 GiB that the default medlow model can
#  reach with absolute addressing.
CFLAGS += -mcmodel=medany

#  -ffreestanding  tell gcc it is not targeting a hosted C environment: no
#  libc is present, and it must not assume the standard library's semantics
#  or that main() is a normal program entry point.
CFLAGS += -ffreestanding

#  -fno-common   give every tentative definition its own storage, so two
#                files declaring the same global collide at link time instead
#                of being silently merged.
#  -nostdlib     do not link libc or the compiler's startup files; xv6
#                provides its own entry path and its own string routines.
CFLAGS += -fno-common -nostdlib

#  Even with -nostdlib, gcc knows the *semantics* of standard functions and
#  will happily replace your code with its own builtin version -- e.g. turning
#  a loop into a call to memcpy, or constant-folding strlen.  In a kernel that
#  is a disaster: the builtin may reference a libc symbol that does not exist,
#  or bypass xv6's own implementation in kernel/string.c.  Each -fno-builtin-X
#  below forces gcc to call *our* X.
CFLAGS += -fno-builtin-strncpy -fno-builtin-strncmp -fno-builtin-strlen -fno-builtin-memset
CFLAGS += -fno-builtin-memmove -fno-builtin-memcmp -fno-builtin-log -fno-builtin-bzero
CFLAGS += -fno-builtin-strchr -fno-builtin-exit -fno-builtin-malloc -fno-builtin-putc
CFLAGS += -fno-builtin-free
#  -Wno-main: xv6's user programs declare main() in ways a hosted compiler
#  would object to (no argc/argv, non-int return); silence that check.
CFLAGS += -fno-builtin-memcpy -Wno-main
CFLAGS += -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-vprintf

#  Let sources say #include "kernel/types.h" relative to the repo root.
CFLAGS += -I.

#  Stack-protector canaries call __stack_chk_fail in libc, which xv6 does not
#  have.  This probes whether the compiler accepts -fno-stack-protector (by
#  preprocessing an empty file) and adds it only if so, keeping older or
#  unusual toolchains working.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# The net lab's tests need to agree with the host on a UDP port number.
ifeq ($(LAB),net)
CFLAGS += -DNET_TESTS_PORT=$(SERVERPORT)
endif

# `make KCSAN=1` builds with the concurrency sanitiser.  -fsanitize=thread
# instruments memory accesses so races can be detected; -fno-inline keeps the
# reports readable.  KCSANFLAG is applied only to $(OBJS), never to
# $(OBJS_KCSAN) -- see the EXTRAFLAG assignment below.
ifdef KCSAN
CFLAGS += -DKCSAN
KCSANFLAG = -fsanitize=thread -fno-inline
endif

#  Many distro toolchains default to building position-independent executables.
#  A kernel linked to a fixed address must not be PIE, and xv6 has no dynamic
#  loader to process relocations.  Different gcc builds spell the opt-out
#  differently, so probe -dumpspecs for which spelling this compiler supports.
#  (The `[^f]` in the grep avoids matching the *-fno-pie* spelling when looking
#  for the linker's *-no-pie*.)
# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

#  -z max-page-size=4096  stop the linker from aligning segments to its
#  default (much larger) page size.  RISC-V Sv39 paging in xv6 uses 4 KiB
#  pages, and oversized alignment would waste megabytes and break the tight
#  layout kernel.ld assumes.
LDFLAGS = -z max-page-size=4096

# ---------------------------------------------------------------------------
#  Linking the kernel
# ---------------------------------------------------------------------------
#  kernel.ld is the linker script: it fixes the load address at 0x80000000
#  (where QEMU's `virt` machine starts RAM), puts _entry first, and defines
#  symbols such as `etext` and `end` that kalloc.c uses to find free memory.
#
#  Two by-products are generated every link and are worth knowing about:
#    kernel/kernel.asm  full disassembly interleaved with source (-S).  This
#                       is the single most useful file for the traps and
#                       pgtbl labs -- look up any address you see in a panic.
#    kernel/kernel.sym  address-to-symbol table, used to map a raw PC back to
#                       a function name.
#  The sed strips objdump's header so only "address name" lines remain.
$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym
# A target-specific variable: EXTRAFLAG is set only while building the files
# in $(OBJS).  $(OBJS_KCSAN) is deliberately excluded, so those five files
# compile without sanitiser instrumentation.  EXTRAFLAG is empty unless KCSAN=1.
$(OBJS): EXTRAFLAG := $(KCSANFLAG)

# Pattern rule: how to compile any kernel .c into a .o.
# $@ is the target (kernel/foo.o), $< is the first prerequisite (kernel/foo.c).
$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<

# Assembly sources (entry.S, swtch.S, trampoline.S, kernelvec.S) are compiled
# with a deliberately minimal flag set -- no CFLAGS.  These files are hand-
# written and must not be touched by the optimiser or the sanitiser; they set
# up stacks, switch contexts, and are entered with no valid C environment.
$K/%.o: $K/%.S
	$(CC) -march=rv64gc -g -c -o $@ $<

tags: $(OBJS)
	etags kernel/*.S kernel/*.c

# ---------------------------------------------------------------------------
#  User space
# ---------------------------------------------------------------------------
#  xv6's entire "C library".  Every user program links against these four:
#    ulib.o     strings, gets, stat helpers
#    usys.o     the system call stubs (generated -- see usys.pl below)
#    printf.o   user-side printf writing to fd 1
#    umalloc.o  a simple free-list malloc built on the sbrk system call
ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

ifeq ($(LAB),lock)
ULIB += $U/statistics.o
endif

#  The rule that builds every user program.  `user/_cat` is produced from
#  `user/cat.o` plus ULIB.  The underscore prefix keeps the final executable
#  from colliding with the intermediate object, and mkfs later copies these
#  _NAME files into fs.img (stripping nothing -- xv6's exec reads ELF directly).
#
#  $* is the stem matched by `%`, so for user/_cat it is "user/cat", which is
#  how the .asm and .sym land next to the source rather than beside the binary.
#  user/user.ld links user programs at virtual address 0.
_%: %.o $(ULIB) $U/user.ld
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $< $(ULIB)
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

#  Generated code: usys.pl prints an assembly stub for each system call, of
#  the form
#        .global fork
#        fork:  li a7, SYS_fork ;  ecall ;  ret
#  i.e. load the syscall number into register a7 and trap into the kernel with
#  `ecall`.  This is the user-side half of the system call interface; the
#  kernel half is the SYS_* table in kernel/syscall.c.
#
#  Because usys.S is generated, it is listed in .gitignore and deleted by
#  `make clean`.  When you add a system call in a lab you must edit usys.pl
#  (not usys.S), plus kernel/syscall.h, kernel/syscall.c and user/user.h.
$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

#  forktest is the one user program with a hand-written link rule.  It links
#  only ulib.o and usys.o (no printf, no malloc) and uses -N -e main -Ttext 0
#  to produce a deliberately tiny binary, because its job is to fork until the
#  process table is full -- which only works if each process is small enough
#  that memory does not run out first.
$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

#  mkfs is the odd one out: it is compiled with the HOST gcc, not the cross
#  compiler, because it runs on your machine to build fs.img.  It includes
#  kernel/fs.h and kernel/param.h so the image it writes matches exactly the
#  on-disk layout the kernel expects -- which is why those headers are
#  prerequisites, and why changing FSSIZE forces mkfs to be rebuilt.
mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc $(XCFLAGS) -Wno-unknown-attributes -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

# ---------------------------------------------------------------------------
#  The User programs that go into the filesystem image
# ---------------------------------------------------------------------------
#  TODO: Anything listed here gets built by the `_%` rule above and copied into
#  fs.img, which is how it becomes runnable inside xv6.  Adding a new user
#  program in a lab means adding it to this list -- otherwise it compiles but
#  never appears in the guest, and the shell reports "exec ... failed".
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_logstress\
	$U/_forphan\
	$U/_dorphan\
	$U/_sync\
	$U/_ex1copy\
	$U/_ex2create\
	$U/_ex3fork\
	$U/_ex4exec\
	$U/_ex5forkexec\
	$U/_ex6redirect\
	$U/_ex7pipe\
	$U/_ex8pipefork\
	$U/_ex9ls\




ifeq ($(LAB),syscall)
UPROGS += \
	$U/_attack\
	$U/_secret
endif

ifeq ($(LAB),lock)
UPROGS += \
	$U/_stats
endif

ifeq ($(LAB),traps)
UPROGS += \
	$U/_call\
	$U/_bttest
endif

ifeq ($(LAB),lazy)
UPROGS += \
	$U/_lazytests
endif

ifeq ($(LAB),cow)
UPROGS += \
	$U/_cowtest
endif

ifeq ($(LAB),thread)
UPROGS += \
	$U/_uthread

$U/uthread_switch.o : $U/uthread_switch.S
	$(CC) $(CFLAGS) -c -o $U/uthread_switch.o $U/uthread_switch.S

$U/_uthread: $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_uthread $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(OBJDUMP) -S $U/_uthread > $U/uthread.asm

ph: notxv6/ph.c
	gcc -o ph -g -O2 $(XCFLAGS) notxv6/ph.c -pthread

barrier: notxv6/barrier.c
	gcc -o barrier -g -O2 $(XCFLAGS) notxv6/barrier.c -pthread
endif

ifeq ($(LAB),pgtbl)
UPROGS += \
	$U/_pgtbltest
endif

ifeq ($(LAB),lock)
UPROGS += \
	$U/_kalloctest\
	$U/_bcachetest
endif

ifeq ($(LAB),fs)
UPROGS += \
	$U/_bigfile
endif


ifeq ($(LAB),mmap)
UPROGS += \
	$U/_mmaptest
endif

ifeq ($(LAB),net)
UPROGS += \
	$U/_nettest
endif

# UEXTRA holds non-executable files that still need to be inside fs.img.
# The util lab ships a shell script and a text file that its tests read.
UEXTRA=
ifeq ($(LAB),util)
	UEXTRA += user/findtest.sh
	UEXTRA += user/sixfive.txt
	UPROGS += $U/_memdump
endif


# ---------------------------------------------------------------------------
#  The filesystem image
# ---------------------------------------------------------------------------
#  mkfs writes a complete xv6 filesystem into fs.img: a superblock, an inode
#  table, a log for crash recovery, a bitmap, and then one file per argument.
#  README is included so there is something to `cat` on first boot.
#  Its output line ("nmeta 47 ... balloc: ...") is a useful sanity check that
#  the layout matches kernel/param.h.
fs.img: mkfs/mkfs README $(UEXTRA) $(UPROGS)
	mkfs/mkfs fs.img README $(UEXTRA) $(UPROGS)

# Rotates the current image aside so the next `make qemu` starts from a fresh
# filesystem.  The leading `-` means "ignore failure" (there may be no fs.img).
newfs.img:
	-mv -f fs.img fs.img.bk

# Pull in the .d files generated by -MD.  Each one says which headers a given
# .o depends on, so touching kernel/proc.h rebuilds every file that includes
# it.  The leading `-` keeps a clean tree (no .d files yet) from erroring.
-include kernel/*.d user/*.d

# Removes every build product, including the generated usys.S and .gdbinit.
# Note it deletes fs.img too, so anything you created inside xv6 is lost.
clean:
	rm -rf *.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.dSYM *.zip *.pcap \
	*/*.o */*.d */*.asm */*.sym \
	$K/kernel fs.img \
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS)

# ---------------------------------------------------------------------------
#  Running under QEMU
# ---------------------------------------------------------------------------
#  Derive a port from your uid so that several users on a shared machine do
#  not collide on the same gdb stub port.
# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
# Number of emulated harts (hardware threads = cores).  3 by default, which is
# enough to exercise xv6's locking without making output unreadable.  Override
# with `make CPUS=1 qemu` to make scheduling deterministic while debugging.
ifndef CPUS
CPUS := 3
endif
# The fs lab forces a single core so its filesystem tests are reproducible.
ifeq ($(LAB),fs)
CPUS := 1
endif

FWDPORT1 = $(shell expr `id -u` % 5000 + 25999)
FWDPORT2 = $(shell expr `id -u` % 5000 + 30999)

#  -machine virt   QEMU's generic RISC-V board.  It is not real hardware; it
#                  is a documented layout of RAM at 0x80000000, a UART at
#                  0x10000000, a PLIC interrupt controller, and virtio-mmio
#                  slots.  kernel/memlayout.h hard-codes these addresses.
#  -bios none      no firmware.  Normally OpenSBI would load first; xv6 skips
#                  it and QEMU jumps straight to our kernel, which is why
#                  kernel/start.c has to do machine-mode setup itself.
#  -kernel         the ELF to load at 0x80000000.
#  -m 128M         physical RAM.
#  -smp $(CPUS)    number of harts.
#  -nographic      no window; the guest UART is wired to this terminal.  That
#                  is why quitting is Ctrl-a x rather than closing a window.
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
#  Use the modern virtio-mmio interface rather than the legacy one; xv6's
#  driver in kernel/virtio_disk.c implements the non-legacy register layout.
QEMUOPTS += -global virtio-mmio.force-legacy=false
#  Attach fs.img as a raw drive with id x0, then expose it as a virtio block
#  device on the mmio bus.  This pair is what makes fs.img appear to xv6 as
#  its one and only disk.
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

ifeq ($(LAB),net)
QEMUOPTS += -netdev user,id=net0,hostfwd=udp::$(FWDPORT1)-:2000,hostfwd=udp::$(FWDPORT2)-:2001 -object filter-dump,id=net0,netdev=net0,file=packets.pcap
QEMUOPTS += -device e1000,netdev=net0,bus=pcie.0
endif

# The main entry point.  Depends on newfs.img, so each `make qemu` starts from
# a freshly built filesystem -- files you created in a previous session are
# rotated away to fs.img.bk.  Quit the emulator with Ctrl-a x.
# makes a new fs.img
qemu: check-qemu-version newfs.img $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

# Same, but keeps the existing fs.img so guest files survive a reboot.
# runs with existing fs.img, if present
qemu-fs: check-qemu-version $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

# Generate .gdbinit from the template, substituting this user's GDB port so
# gdb connects to the right stub automatically.
.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

# Boot with the CPU halted (-S) and a gdb stub listening, so you can attach
# before the first instruction runs.  Run this in one terminal, then
# `gdb-multiarch kernel/kernel` (or just `gdb`) in another.
qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

ifeq ($(LAB),net)
# try to generate a unique port for the echo server
SERVERPORT = $(shell expr `id -u` % 5000 + 25099)

endif

##
##  FOR testing lab grading script
##

# Pass -v to the grade script unless output was explicitly quietened.
ifneq ($(V),@)
GRADEFLAGS += -v
endif

print-gdbport:
	@echo $(GDBPORT)

# Runs the grading script for whichever lab conf/lab.mk selects, e.g.
# ./grade-lab-util.  It always starts with `make clean`, because the graders
# rebuild with lab-specific -D flags and a stale object tree would silently
# test the wrong binary.  Each test boots QEMU, drives the shell, and matches
# output; failures leave the transcript in xv6.out.<testname> for inspection.
# To grade a single test instead: ./grade-lab-util sleep
grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
          (echo "'make clean' failed.  HINT: Do you have another running instance of xv6?" && exit 1)
	./grade-lab-$(LAB) $(GRADEFLAGS)

##
## FOR submissions
##

# Sanity checks before handing in: that this is a git repo, that you are on
# the branch named after the lab, and that nothing is uncommitted or untracked
# that you meant to include.  (In this consolidated repo you work on `study`
# rather than a branch named after the lab, so the branch check will prompt.)
submit-check:
	@if ! test -d .git; then \
		echo No .git directory, is this a git repository?; \
		false; \
	fi
	@if test "$$(git symbolic-ref HEAD)" != refs/heads/$(LAB); then \
		git branch; \
		read -p "You are not on the $(LAB) branch.  Hand-in the current branch? [y/N] " r; \
		test "$$r" = y; \
	fi
	@if ! git diff-files --quiet || ! git diff-index --quiet --cached HEAD; then \
		git status -s; \
		echo; \
		echo "You have uncomitted changes.  Please commit or stash them."; \
		false; \
	fi
	@if test -n "`git status -s`"; then \
		git status -s; \
		read -p "Untracked files will not be handed in.  Continue? [y/N] " r; \
		test "$$r" = y; \
	fi

# Produces lab.zip from the committed tree (git archive, so uncommitted work
# is NOT included -- hence the submit-check above).
zipball: clean submit-check
	git archive --verbose --format zip --output lab.zip HEAD

# .PHONY marks targets that are commands, not files, so Make runs them even if
# a file of that name happens to exist in the directory.
.PHONY: zipball clean grade submit-check check-qemu-version

# Parse "QEMU emulator version 8.2.2" down to "8.2" and refuse to boot if it
# is older than MIN_QEMU_VERSION.  Older QEMU predates the non-legacy
# virtio-mmio support that kernel/virtio_disk.c requires, and the failure mode
# without this check is a confusing hang rather than a clear error.
QEMU_VERSION := $(shell $(QEMU) --version | head -n 1 | sed -E 's/^QEMU emulator version ([0-9]+\.[0-9]+)\..*/\1/')
check-qemu-version:
	@if [ "$(shell echo "$(QEMU_VERSION) >= $(MIN_QEMU_VERSION)" | bc)" -eq 0 ]; then \
		echo "ERROR: Need qemu version >= $(MIN_QEMU_VERSION)"; \
		exit 1; \
	fi

# Reformats all sources with clang-format using the repo's .clang-format.
# Upstream runs this periodically; avoid running it on a branch with lab work
# in progress, since a whole-tree reformat makes future merges from the lab
# branches conflict on nearly every file.
.PHONY: fmt
fmt:
	clang-format -i $(wildcard kernel/*.[ch] user/*.[ch] mkfs/*.c)
