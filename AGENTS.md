# AGENTS.md

## Documentation Visual Readability

- Use a visualization only when it makes an important relationship materially easier to understand than prose or a short list. Do not add one merely because a section is long.
- Preserve every technical detail when restructuring documentation. A visual may reorganize or front-load information, but it must not silently simplify away invariants, failure behavior, exceptions, values, or source references.
- Layer complex explanations when necessary: lead with a compact visual overview, then retain the detailed explanation immediately below it.
- Keep nuanced causal reasoning in prose. Do not cram paragraph-sized explanations into Mermaid nodes or oversized table cells.
- Prefer the smallest visual that matches the information shape:

| Information shape                                                      | Preferred presentation                |
| ---------------------------------------------------------------------- | ------------------------------------- |
| **Control flow, lifecycle, or state transition**                       | Mermaid flowchart or sequence diagram |
| **Pointers, ownership, sharing, or reference counts**                  | Object graph                          |
| **Conditions, branches, and outcomes**                                 | Decision or state table               |
| **Ordered operations without meaningful branching**                    | Numbered list                         |
| **Repeated fields, alternatives, naming, or architecture differences** | Comparison table                      |
| **Hierarchy, nesting, or parent/child relationships**                  | Tree or flowchart                     |
| **Critical invariant or deliberate exception**                         | GitHub-style callout                  |

- Use callouts deliberately: `NOTE` for context or invariants, `TIP` for practical workflow advice, `IMPORTANT` for decisions readers must preserve, `WARNING` for likely mistakes, and `CAUTION` for destructive or difficult-to-recover consequences. Do not use callouts as decoration or as a substitute for ordinary exposition.
- In tables that compare or summarize named items, bold the first-column value unless it is a raw identifier, command, path, URL, or value readers need to copy exactly.
- In Mermaid diagrams, label every phase and outcome so meaning survives monochrome rendering. Use color consistently, and never make color the only carrier of meaning.
- Use the repository's light-theme Mermaid palette consistently: request `#fff0f0`, decision or routing `#f0f0ff`, data movement `#f0fff0`, processing `#fffff0`, commit `#f0ffff`, and stall or failure `#ffd9d9`.
- Keep diagrams focused. If a diagram needs dense prose, many cross-links, or more than a quick glance to decode, split it or return the detail to prose.
- Preserve existing effective comprehension aids. Tables, examples, summaries, and callouts are not redundant when they help readers orient, compare, or retain information.

## Book Notes

- Apply the visual-readability rules to `docs/book/*.md` while preserving the ownership model in `docs/book/README.md`: OSTEP notes own portable concepts, and xv6 notes own this tree's implementation.
- Keep each chapter's standard sections unless the chapter has no relevant content for one: `What xv6 actually does`, `Divergences`, `Code walked through`, `Questions I had`, and `Lab connection`.
- Use one or two purposeful visuals per substantive chapter by default. Add more only when the chapter contains additional relationships that cannot be scanned effectively in prose.
- Preserve detailed source-symbol explanations near their visual overview. A chapter diagram is an entry point into the explanation, not a replacement for implementation evidence.
- Leave placeholder chapters structurally minimal until substantive content exists; do not decorate empty scaffolding.
