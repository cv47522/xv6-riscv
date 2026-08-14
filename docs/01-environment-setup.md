# Environment Setup

---

[toc]

---

Installing the toolchain for xv6-riscv, and — more usefully — what each package is actually for. Based on MIT's [6.1810 tools page](https://pdos.csail.mit.edu/6.1810/2026/tools.html), with the reasoning filled in.

## The one-liner

On Debian, Ubuntu, or WSL 2 running Ubuntu:

```bash
sudo apt-get install git build-essential gdb-multiarch qemu-system-misc \
                     gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

> [!IMPORTANT]
> Ubuntu 24.04 or newer. Older releases ship a QEMU below the 7.2 minimum that xv6's virtio disk driver requires, and the Makefile's `check-qemu-version` target will refuse to boot.

## Why each package

The central fact that explains this list: **xv6 is cross-compiled**. Your machine is x86-64; the kernel you build targets RISC-V and can only run under emulation. So you need two complete toolchains — one for the host, one for the target — plus an emulator to run the result.

| Package                        | What it provides                                   | Why xv6 needs it                                                                                                                                                                      |
| ------------------------------ | -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **git**                        | Version control                                    | Cloning this repo, and the lab workflow in [00-repo-workflow.md](00-repo-workflow.md).                                                                                                |
| **build-essential**            | Host `gcc`, `g++`, `make`, `libc6-dev`, `dpkg-dev` | Two things. `make` drives the whole build. And `mkfs/mkfs` — the tool that writes `fs.img` — is compiled with the **host** compiler, because it runs on your machine, not inside xv6. |
| **gcc-riscv64-linux-gnu**      | The cross compiler, `riscv64-linux-gnu-gcc`        | Compiles every kernel and user source into RISC-V machine code. This is the `riscv64-linux-gnu-` that the Makefile's `TOOLPREFIX` probe finds.                                        |
| **binutils-riscv64-linux-gnu** | Cross `ld`, `as`, `objdump`, `objcopy`, `nm`       | `ld` links `kernel/kernel` against `kernel.ld`. `objdump` generates `kernel.asm` and `kernel.sym` on every build — the disassembly you will live in during the traps and pgtbl labs.  |
| **qemu-system-misc**           | `qemu-system-riscv64`                              | Emulates the whole RISC-V machine xv6 boots on. Debian bundles the less common architectures into this "misc" package rather than shipping one per ISA.                               |
| **gdb-multiarch**              | A GDB that understands many target architectures   | Plain `gdb` on an x86 host can only debug x86. `gdb-multiarch` can attach to QEMU's RISC-V gdb stub.                                                                                  |

### Also required, but already present

These come preinstalled on Ubuntu, so MIT's command does not list them. Worth knowing about, because a missing one produces a confusing failure rather than a clear "not found":

- **perl** — `user/usys.pl` generates `user/usys.S`, the assembly stubs for every system call. Without perl the build fails at a generated file that is not in git.
- **python3** — `gradelib.py` and every `grade-lab-*` script are Python 3. Needed only for `make grade`.
- **bc** — the Makefile's `check-qemu-version` compares version numbers by piping an expression to `bc`. Without it, the comparison yields an empty string and `make qemu` fails with an obscure shell error rather than a version message.

## Other platforms

**Arch Linux:**

```bash
sudo pacman -S riscv64-linux-gnu-binutils riscv64-linux-gnu-gcc \
               riscv64-linux-gnu-gdb qemu-emulators-full bc git base-devel
```

**macOS** — the toolchain comes from a Homebrew tap rather than distro packages, and uses the bare-metal `riscv64-unknown-elf-` prefix instead of the Linux one:

```bash
xcode-select --install
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew tap riscv/riscv
brew install riscv-tools
brew install qemu
```

Then add the toolchain to your `PATH` in your shell rc file:

```bash
PATH=$PATH:/usr/local/opt/riscv-gnu-toolchain/bin
```

**Windows** — install WSL 2 with Ubuntu 24.04, then follow the Debian/Ubuntu instructions inside it. Everything runs in the Linux environment.

## Verifying the install

```bash
qemu-system-riscv64 --version
riscv64-linux-gnu-gcc --version
gdb-multiarch --version
```

A known-good set, from the machine these notes were written on (Ubuntu 24.04.4 LTS under WSL 2):

| Tool                      | Version |
| ------------------------- | ------- |
| **qemu-system-riscv64**   | 8.2.2   |
| **riscv64-linux-gnu-gcc** | 13.3.0  |
| **gdb-multiarch**         | 15.1    |
| **GNU Make**              | 4.3     |
| **python3**               | 3.12.10 |

> [!NOTE]
> The MIT page also lists `riscv64-unknown-elf-gcc` and `riscv64-unknown-linux-gnu-gcc` among the version checks. You do not need those if you installed the `riscv64-linux-gnu-` packages — they are alternative prefixes for the same job. The Makefile probes for all of them in turn and uses whichever it finds first; see the `TOOLPREFIX` block in the [Makefile](../Makefile).

## Confirming it all works

The real test is a build and boot:

```bash
make qemu
```

You should see the kernel boot and land at a shell prompt. Quit with `Ctrl-a` then `x`. If anything goes wrong here, [02-build-boot-and-usage.md](02-build-boot-and-usage.md) walks through the output line by line and covers the common failures.

## Behind a corporate proxy

If `git` operations hang rather than fail, see the network section of [00-repo-workflow.md](00-repo-workflow.md). The short version: HTTPS and SSH work, `git://` does not, and the failure mode is an indefinite hang rather than an error message.
