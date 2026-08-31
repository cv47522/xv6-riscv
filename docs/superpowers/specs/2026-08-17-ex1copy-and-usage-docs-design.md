# `ex1copy` and xv6 Usage Documentation Design

## Goal

Make the official MIT 6.1810 `ex1.c` example easy to build, run, understand, and stop in this repository while improving its error handling and preserving its filter semantics. Expand the existing usage documentation with the keyboard shortcuts and exercise integration rules that explain the original failure modes.

## Source and Ownership

The exercise is based on MIT 6.1810's 2026 Lecture 1 source at <https://pdos.csail.mit.edu/6.1810/2026/lec/l-overview/ex1.c>. Its intended behavior is to copy bytes from standard input to standard output until `read()` reports EOF; it must not become a one-line command or print an application prompt.

Generic operating-system concepts remain owned by the sibling `../../../../operating-system/` repository. `Introduction_to_Operating_Systems.md` already owns OS purpose, resource management, isolation, abstraction, and design tradeoffs. `The_Process_Abstraction.md` already owns generic system-call, process, and file-descriptor theory. This repository will link to those notes and document only xv6-specific implementation and usage details.

## Documentation Changes

### Build, Boot, and Usage Guide

Expand `docs/02-build-boot-and-usage.md` in two places.

The keyboard section will distinguish keys handled by xv6 from escape sequences handled by QEMU. It will document `Ctrl-d`, `Ctrl-u`, `Ctrl-p`, Backspace/Delete, `Ctrl-a` then `x`, `Ctrl-a` then `c`, and `Ctrl-a` then `h`. It will state that `Ctrl-a` is a prefix: press and release it before pressing the second key without `Ctrl`. It will explain terminal focus, `tmux` or GNU Screen interception, how `Ctrl-d` differs from quitting QEMU, and how to identify a stale emulator process.

A new exercise workflow will explain that host files are not automatically visible inside xv6. A user program must live directly under `user/`, use a guest filename no longer than xv6's 14-byte `DIRSIZ`, include xv6 headers instead of the host `<stdio.h>`, use integer descriptors rather than `FILE *`, and appear in `UPROGS` so `mkfs` installs it in `fs.img`. The workflow will show how to rebuild, confirm the command with `ls`, run it, and diagnose `exec ... failed`.

The guide will include an `ex1copy` terminal transcript. It will explain that xv6 console input is line-buffered, the UART echoes typed characters, the program writes the same bytes again, and the next `read()` then blocks waiting for another line. Pressing `Ctrl-d` at an empty input position produces EOF, lets the program exit, and returns control to the shell. This is expected filter behavior, not a deadlock. A pipeline example will demonstrate why the program must remain prompt-free.

### Lab Workflow

Update `docs/03-lab-workflow.md` only where the Lecture 1 notes add durable, nonduplicated guidance. Keep the existing practical advice and add a concise statement that labs span user-space systems programming, operating-system primitives, and kernel extensions. Preserve the distinction between discussing concepts and independently writing the implementation. Do not copy grading percentages, schedules, office hours, deadlines, or other course logistics.

### Terminology

Add concise xv6-specific entries to `docs/04-terminology.md` for file descriptor and EOF only if the usage guide needs stable local anchors. Definitions will point to concrete xv6 structures or console behavior and will link outward for generic theory rather than duplicating it.

### Unchanged Documentation

Do not add a general OS overview file. Do not add the supplied generic OS concepts to `docs/book/ch01-operating-system-interfaces.md`, because the book-note policy explicitly assigns those concepts to the sibling repository.

## Program Changes

Keep the staged `Makefile` registration for `$U/_ex1copy` and preserve the command name `ex1copy`.

Refine `user/ex1copy.c` to match xv6 style and use ASCII comments. Attribute the official source, explain descriptors 0, 1, and 2, document that console reads wait for a complete line, and tell an interactive reader to press `Ctrl-d` at an empty input position to finish.

The loop will continue until EOF. A negative `read()` result will be reported to descriptor 2 and exit with status 1. A failed or short `write()` will also be reported to descriptor 2 and exit with status 1. EOF will exit with status 0. The program will not add a prompt, alter bytes, assume text input, or stop after one read.

## Verification

Verification will build the registered program and filesystem image, then boot xv6 and exercise both usage modes. A pipeline will prove that finite input is copied and the command returns. An interactive run will prove that one entered line is copied, the process intentionally remains blocked for more input, and `Ctrl-d` returns to the shell. The build and existing relevant tests must remain clean.

Documentation verification will run the repository's note checker where applicable, check links and headings, scan renamed terms and commands, and confirm that no edited prose paragraph was hard-wrapped. A final diff review will ensure the changes do not reintroduce generic theory already owned by `../../../../operating-system/` and do not include transient course logistics.
