---
name: review-changes
description: Review this repository's pending changes against .agents/rules and output a numbered findings table without editing files. Use when the user asks to review changes, a diff, docs, code, images, or diagrams in this repo.
argument-hint: "[paths | --staged | --last-commit]"
---

Follow `.agents/rules/review.md` exactly.

Scope: $ARGUMENTS. With no arguments, review the unstaged diff (`git diff --word-diff`). With `--staged`, review `git diff --cached --word-diff`. With `--last-commit`, review `git show --word-diff HEAD`. With paths, review those files in full.

1. List the changed files and load only the matching rule files from the table in `review.md`.
2. Run `make lint` and include its output as is.
3. Read every changed image with the Read tool before judging it.
4. Output the findings table and one-line summary. **Do not** edit, stage, or commit anything.
