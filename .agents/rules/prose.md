# Prose and Markdown rules

Applies to every `*.md` file. The user's global `AGENTS.md`, when an agent has it, holds the full Markdown guide; this file repeats what this repository's reviews depend on so agents without it still follow the rules.

## Rules

- Write each prose paragraph on one continuous line. **Do not** hard-wrap paragraphs or reflow unrelated text. *(lint: `no-hard-wrap`)*
- **Do not** reference files or directories under gitignored `transcripts/` in any Markdown file, whether as links or plain filenames. Replace those references with topic descriptions and links to maintained documentation or runnable code. Answers **must** remain understandable without the ignored notes. *(lint: `no-transcripts`)*
- **Bold important keywords** in prose, especially requirements and prohibitions such as **must**, **must not**, **do not**, and **never**, so readers can scan constraints. Emphasize the words that carry the rule; preserve literal code, commands, quotations, and tool output.
- Use `-` for unordered lists and ATX (`#`) headings. *(lint: `bullet-style`, `atx-headings`)*
- Keep relative links and image paths valid. *(lint: `broken-link`)*
- In tables that compare named items, bold the first-column value unless it is a code literal, path, URL, or ID.
- Use GitHub callouts (`> [!NOTE]`, `> [!WARNING]`, …) for decisions and risks; do not stack them.
- Keep documentation practical and example-driven, and put commands next to the concept they explain. Avoid marketing language.

## Commas

- Use the serial comma in a series of three or more.
- Use a comma before `and`, `but`, `or`, `so`, and the other coordinating conjunctions only when they join two independent clauses; never for a compound subject, object, or predicate.
- Never join independent clauses with a comma alone.
- Follow introductory dependent clauses, participial phrases, and transitional expressions with a comma.
- Set off nonessential elements with paired commas; do not set off essential ones.

## Review focus

- Is each claim technically correct for the architecture and tool version the lesson uses?
- Does the text explain *why*, not only *what*, without repeating what the code shows?
- Are terms used consistently with earlier lessons in the same track?
- Do renamed headings or files still match every link and table of contents?

## Decisions

Wording and style preferences settled during past reviews. Reviewers **must not** propose changes that reverse these.

<!-- - YYYY-MM-DD: Keep "<phrase>"; do not change it to "<alternative>". Reason: <why>. -->
