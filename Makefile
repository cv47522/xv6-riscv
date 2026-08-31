#
# ============================================================================
#  xv6-riscv build system
# ============================================================================
#
#  Build outputs:
#
#    kernel/kernel  RISC-V ELF linked by kernel/kernel.ld; QEMU loads it
#                   directly without a bootloader.
#    user/_NAME     standalone user ELF; `_` distinguishes it from NAME.o.
#    fs.img         root filesystem containing the user ELFs and README.
#
#  `make qemu` boots kernel/kernel with fs.img as a virtio disk.
#
#  Kernel and user objects target RISC-V, not the host. They cannot run on the
#  host or link its libc; this requires -ffreestanding, -nostdlib,
#  -fno-builtin-*, and -mcmodel=medany.
#
# ============================================================================

# ---------------------------------------------------------------------------
#  Lab selection
# ---------------------------------------------------------------------------
#  conf/lab.mk optionally sets LAB=NAME. LAB adds -DLAB_NAME/-DSOL_NAME,
#  selects lab-specific objects and programs, and makes `grade` run
#  grade-lab-NAME. Edit conf/lab.mk and run `make clean` when switching labs.
#  The leading `-` lets plain xv6 build when conf/lab.mk is absent.
-include conf/lab.mk

# Short aliases used throughout, purely to keep the lists below readable.
K=kernel
U=user

# ---------------------------------------------------------------------------
#  Kernel object files
# ---------------------------------------------------------------------------
#  Every object linked into kernel/kernel. entry.o contains QEMU's first
#  instruction, _entry, and kernel.ld places it first.
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
#  These objects run before KCSAN is ready or implement machinery it uses.
#  Instrumenting them would recurse, so only OBJS receives KCSANFLAG below.
#
#  Upstream renamed kernel/printf.c to printk.c after the 2025 lab branches;
#  this merged tree therefore uses printk.o.
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
#  TOOLPREFIX turns gcc into a cross tool such as riscv64-linux-gnu-gcc.
#  Set it manually for toolchains in nonstandard locations.
# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
#TOOLPREFIX =

#  Otherwise, probe candidate objdump tools for the `elf64-big` marker, which
#  indicates 64-bit RISC-V ELF support.
#
#  Bare-metal prefixes are preferred. Linux-targeted tools also work because
#  -nostdlib and -ffreestanding remove their hosted assumptions.
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
#  QEMU 7.2+ is required for the non-legacy virtio-mmio interface used by
#  kernel/virtio_disk.c; check-qemu-version enforces this before boot.
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
#  -Wall -Werror            keep kernel code warning-clean.
#  -Wno-unknown-attributes  tolerate unknown upstream annotations.
#  -O                       satisfy assembly/layout assumptions; shrink code.
#  -fno-omit-frame-pointer  preserve s0 for r_fp() and stack backtraces.
#  -ggdb -gdwarf-2          emit course-compatible gdb information.
CFLAGS = -Wall -Werror -Wno-unknown-attributes -O -fno-omit-frame-pointer -ggdb -gdwarf-2

#  rv64gc is 64-bit RISC-V with the G extensions and compressed instructions.
CFLAGS += -march=rv64gc

#  xv6 uses GNU C99 extensions, including inline assembly.
CFLAGS += -std=gnu99

#  LAB=util defines LAB_UTIL and SOL_UTIL. LAB_* selects starter differences;
#  SOL_* selects solution code. XCFLAGS also reaches host-built mkfs and
#  notxv6 programs without adding RISC-V-only CFLAGS.
ifdef LAB
LABUPPER = $(shell echo $(LAB) | tr a-z A-Z)
XCFLAGS += -DSOL_$(LABUPPER) -DLAB_$(LABUPPER)
endif

CFLAGS += $(XCFLAGS)

#  -MD emits header dependencies consumed by the -include rule below.
CFLAGS += -MD

#  medany uses PC-relative addressing within +/-2 GiB, allowing the kernel to
#  link at 0x80000000 instead of the low addresses required by medlow.
CFLAGS += -mcmodel=medany

#  xv6 has no hosted C environment, libc, or conventional main() entry point.
CFLAGS += -ffreestanding

#  -fno-common rejects duplicate tentative globals; -nostdlib excludes libc
#  and host startup files because xv6 supplies both.
CFLAGS += -fno-common -nostdlib

#  -nostdlib does not disable compiler builtins. Disable them explicitly so
#  gcc neither emits missing libc calls nor bypasses xv6 implementations.
CFLAGS += -fno-builtin-strncpy -fno-builtin-strncmp -fno-builtin-strlen -fno-builtin-memset
CFLAGS += -fno-builtin-memmove -fno-builtin-memcmp -fno-builtin-log -fno-builtin-bzero
CFLAGS += -fno-builtin-strchr -fno-builtin-exit -fno-builtin-malloc -fno-builtin-putc
CFLAGS += -fno-builtin-free
#  xv6 main() signatures need not match a hosted program's signature.
CFLAGS += -fno-builtin-memcpy -Wno-main
CFLAGS += -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-vprintf

#  Let sources say #include "kernel/types.h" relative to the repo root.
CFLAGS += -I.

#  Stack canaries require libc's __stack_chk_fail. Probe support before
#  disabling them so older toolchains still build.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# The net lab's tests need to agree with the host on a UDP port number.
ifeq ($(LAB),net)
CFLAGS += -DNET_TESTS_PORT=$(SERVERPORT)
endif

# KCSAN instruments memory accesses for race detection; -fno-inline keeps
# reports readable. Only OBJS receives KCSANFLAG.
ifdef KCSAN
CFLAGS += -DKCSAN
KCSANFLAG = -fsanitize=thread -fno-inline
endif

#  xv6 is fixed-address and has no dynamic loader, so it cannot be PIE. Probe
#  compiler specs for both opt-out spellings; `[^f]` distinguishes -no-pie
#  from -fno-pie.
# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

#  Match linker segment alignment to Sv39's 4 KiB pages and kernel.ld.
LDFLAGS = -z max-page-size=4096

# ---------------------------------------------------------------------------
#  Linking the kernel
# ---------------------------------------------------------------------------
#  kernel.ld links at QEMU RAM address 0x80000000, places _entry first, and
#  defines etext/end for memory initialization.
#
#    kernel/kernel.asm  source-interleaved disassembly for panic addresses
#    kernel/kernel.sym  address-to-symbol table; sed removes objdump headers
$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym
# EXTRAFLAG applies KCSAN only to OBJS, excluding OBJS_KCSAN.
$(OBJS): EXTRAFLAG := $(KCSANFLAG)

# $@ is kernel/foo.o; $< is kernel/foo.c.
$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<

# Assembly uses minimal flags because it runs without a C environment and must
# not be optimized or instrumented.
$K/%.o: $K/%.S
	$(CC) -march=rv64gc -g -c -o $@ $<

tags: $(OBJS)
	etags kernel/*.S kernel/*.c

# ---------------------------------------------------------------------------
#  User space
# ---------------------------------------------------------------------------
#  Every user program links xv6's complete C library:
#    ulib.o     strings, gets, stat helpers
#    usys.o     the system call stubs (generated -- see usys.pl below)
#    printf.o   user-side printf writing to fd 1
#    umalloc.o  a simple free-list malloc built on the sbrk system call
ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

ifeq ($(LAB),lock)
ULIB += $U/statistics.o
endif

#  `user/_cat` links user/cat.o with ULIB. The underscore avoids colliding with
#  the object; mkfs copies the ELF unchanged into fs.img.
#
#  $* is the matched stem, so .asm/.sym stay beside the source. user/user.ld
#  links user programs at virtual address 0.
_%: %.o $(ULIB) $U/user.ld
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $< $(ULIB)
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

#  usys.pl generates each user-side system-call stub:
#        .global fork
#        fork:  li a7, SYS_fork ;  ecall ;  ret
#  It loads a7 and executes ecall; kernel/syscall.c provides the kernel table.
#
#  Edit usys.pl, kernel/syscall.h, kernel/syscall.c, and user/user.h when
#  adding a syscall. usys.S is generated, ignored, and removed by `make clean`.
$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

#  forktest omits printf and malloc and uses a compact link layout, preventing
#  memory exhaustion before it fills the process table.
$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

#  mkfs runs on the host, so host gcc builds it. fs.h and param.h keep its disk
#  layout and FSSIZE synchronized with the kernel.
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
#  Each entry is built by `_%` and copied into fs.img. Unlisted programs do not
#  appear in xv6, so the shell reports "exec ... failed".
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
	$U/_sleep\




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
#  mkfs writes the superblock, inode table, log, bitmap, README, and supplied
#  files. Its nmeta/balloc output checks the layout against kernel/param.h.
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
# Derive a per-user gdb port to avoid collisions on shared hosts.
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# Support both forms of QEMU's gdb-stub option.
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

#  -machine virt  board layout defined in kernel/memlayout.h
#  -bios none     boot xv6 directly; kernel/start.c performs machine setup
#  -kernel        ELF loaded at 0x80000000
#  -m 128M        physical memory
#  -smp           emulated harts
#  -nographic     connect the UART here; quit with Ctrl-a x
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
# kernel/virtio_disk.c implements modern, not legacy, virtio-mmio.
QEMUOPTS += -global virtio-mmio.force-legacy=false
# Attach raw fs.img as xv6's virtio block device.
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

ifeq ($(LAB),net)
QEMUOPTS += -netdev user,id=net0,hostfwd=udp::$(FWDPORT1)-:2000,hostfwd=udp::$(FWDPORT2)-:2001 -object filter-dump,id=net0,netdev=net0,file=packets.pcap
QEMUOPTS += -device e1000,netdev=net0,bus=pcie.0
endif

# Rotate fs.img to fs.img.bk before each boot; quit QEMU with Ctrl-a x.
qemu: check-qemu-version newfs.img $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

# Keep fs.img so guest files survive reboot.
qemu-fs: check-qemu-version $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

# Generate .gdbinit from the template, substituting this user's GDB port so
# gdb connects to the right stub automatically.
.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

# Halt before the first instruction and expose a gdb stub.
qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

ifeq ($(LAB),net)
# try to generate a unique port for the echo server
SERVERPORT = $(shell expr `id -u` % 5000 + 25099)

endif

# ---------------------------------------------------------------------------
#  Grading
# ---------------------------------------------------------------------------

# Pass -v to the grade script unless output was explicitly quietened.
ifneq ($(V),@)
GRADEFLAGS += -v
endif

print-gdbport:
	@echo $(GDBPORT)

# Clean before grade-lab-$(LAB) so stale flags cannot test the wrong binary.
# Tests drive QEMU and leave failures in xv6.out.<testname>. Run a named test
# directly, for example `./grade-lab-util sleep`.
grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
          (echo "'make clean' failed.  HINT: Do you have another running instance of xv6?" && exit 1)
	./grade-lab-$(LAB) $(GRADEFLAGS)

# ---------------------------------------------------------------------------
#  Submission
# ---------------------------------------------------------------------------

# Require a git repo, confirm a non-lab branch, reject tracked changes, and
# warn that untracked files are excluded. The `study` branch prompts here.
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

# Archive only committed HEAD content into lab.zip.
zipball: clean submit-check
	git archive --verbose --format zip --output lab.zip HEAD

# .PHONY marks targets that are commands, not files, so Make runs them even if
# a file of that name happens to exist in the directory.
.PHONY: zipball clean grade submit-check check-qemu-version

# Parse QEMU's major.minor version and reject releases too old for modern
# virtio-mmio, avoiding an otherwise unexplained boot hang.
QEMU_VERSION := $(shell $(QEMU) --version | head -n 1 | sed -E 's/^QEMU emulator version ([0-9]+\.[0-9]+)\..*/\1/')
check-qemu-version:
	@if [ "$(shell echo "$(QEMU_VERSION) >= $(MIN_QEMU_VERSION)" | bc)" -eq 0 ]; then \
		echo "ERROR: Need qemu version >= $(MIN_QEMU_VERSION)"; \
		exit 1; \
	fi

# Reformat every C source. Avoid this with lab work in progress because the
# whole-tree diff will conflict with later lab merges.
.PHONY: fmt
fmt:
	clang-format -i $(wildcard kernel/*.[ch] user/*.[ch] mkfs/*.c)
