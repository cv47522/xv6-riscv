# Lab Note Readability Implementation Plan

**Goal:** Make the utility lab notes easy to scan without removing technical detail, add prerequisite maps to every utility exercise note, and provide a focused hosted-C example for the byte-memory functions used by the memdump exercise.

**Constraints:** Preserve the required lab-note section order, do not include a memdump solution or pseudocode, keep every Markdown prose paragraph on one physical line, use no more than two purposeful diagrams per lab note, and leave unrelated worktree changes untouched.

## Task 1: Strengthen the authoring prompts

**Files:**

- Modify: `docs/prompts/derive-lab-exercise.md`
- Modify: `docs/prompts/add-lecture-exercise.md`

Add explicit rules for prerequisite maps, layered explanations, information-shaped visuals, and a final scan audit that checks whether headings and visual aids expose the key invariants, branches, ownership, and traps.

## Task 2: Add prerequisite maps to every utility note

**Files:**

- Modify: `docs/labs/util-sleep.md`
- Modify: `docs/labs/util-sixfive.md`
- Modify: `docs/labs/util-memdump.md`
- Modify: `docs/labs/README.md`

Place each map after the exercise contract and before the code walkthrough. Distinguish required reading from just-in-time references, link to the owning xv6 notes, and link to the relevant top-level C notes and examples where they teach a prerequisite rather than the exercise answer.

## Task 3: Add scan layers to the utility notes

**Files:**

- Modify: `docs/labs/util-sleep.md`
- Modify: `docs/labs/util-sixfive.md`
- Modify: `docs/labs/util-memdump.md`

Restructure dense concept sections with compact tables, lists, callouts, and one additional pointer-layout diagram in the memdump note. Retain nuanced causal explanations immediately below those summaries and keep each note within its two-diagram budget.

## Task 4: Add and index the hosted-C memory example

**Files:**

- Create: `/home/wahsieh/personal/c-programming/notes/src/main/memory/byte_memory_functions.c`
- Modify: `/home/wahsieh/personal/c-programming/C_Standard_Library.md`
- Modify: `/home/wahsieh/personal/c-programming/C_Pointers.md`
- Modify: `/home/wahsieh/personal/c-programming/C_Pointer_Arithmetic.md`

Demonstrate exact byte counts, byte-wise inspection, `memset`, non-overlapping `memcpy`, overlapping `memmove`, and `memcmp` without relying on undefined behavior. Register the example where readers already look for memory-block and byte-pointer material.

## Task 5: Verify structure, links, source safety, and builds

Run the lab-note checker, validate every fenced C quotation against non-target source, check new links and heading structure, detect newly hard-wrapped prose, compile and run the new C example, and inspect both repositories' diffs and status without changing staging.
