# Assembly rules

Applies to `codes/**/*.s`, `codes/**/*.S`, `codes/**/*.asm`, and every `asm` fence or inline assembly reference in `docs/`.

The user chose uppercase instruction mnemonics and register names to distinguish CPU operations from lowercase labels and symbols. Apply this convention consistently to authored assembly source, documentation examples, and inline assembly references.

## Rules

- Write instruction mnemonics in uppercase, including condition and flag suffixes: `MOV`, `ADDS`, `BEQ`, `ADDLT`, `LDR`, `CALL`, and `CSRR`. *(lint: `uppercase-mnemonic`)*
- Write ARM register names in uppercase: `R0`–`R15`, `SP`, `LR`, `PC`, `CPSR`, and `SPSR`.
- Keep RISC-V register and CSR names lowercase, such as `a0`, `sp`, `ra`, and `mstatus`. GNU assembler rejects uppercase RISC-V register operands; this is a required syntax exception.
- Use lowercase `snake_case` for labels and uppercase `SNAKE_CASE` for named constants. Preserve externally defined symbol names and ABI interfaces.
- Keep assembler directives lowercase, such as `.global`, `.text`, `.word`, and `.equ`, and NASM directives such as `section`, `global`, and `db`.
- Preserve literal tool output, external quotations, file paths, and case-sensitive debugger syntax. Disassemblers may print lowercase instructions; GDB register names remain lowercase. Put such output in a `text` or `gdb` fence, **not** an `asm` fence, so the lint does not flag it.
- Keep ordinary C/C++ identifiers in their language's existing style; this assembly convention does **not** uppercase C variables or function names.

## Review focus

- Does every comment state intent or a non-obvious fact rather than restating the instruction?
- When source changes, are the lesson text, line references, and screenshots that show it updated too? See [images.md](images.md).
- Does `make check` (RISC-V) still pass after edits to `codes/riscv/`?

## Decisions

Preferences settled during past reviews. Reviewers **must not** propose changes that reverse these.

<!-- - YYYY-MM-DD: <decision>. Reason: <why>. -->
