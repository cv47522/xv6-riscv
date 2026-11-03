---
name: apply-review
description: Apply only the selected findings from the latest review-changes table, then propose Decisions entries for rejected ones. Use when the user answers a review with a selection such as "accept 1,3; reject rest".
argument-hint: "accept 1,3,5; 4 but keep \"...\"; reject rest"
---

Follow the "Applying a review" section of `.agents/rules/review.md`.

Selection: $ARGUMENTS

1. Find the most recent findings table in this conversation. If there isn't one, ask the user to paste it; **do not** guess.
2. Apply each accepted finding with the smallest edit. **Never** reflow surrounding lines, and **do not** stage or commit.
3. Run `make lint` and report the result.
4. Show a numbered list of proposed `## Decisions` entries for rejected `rule` and `preference` findings, each naming its target rule file. Write only the entries the user confirms.
