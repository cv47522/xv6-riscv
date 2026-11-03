# Diagram rules

Applies to `mermaid` fences in `docs/` and to drawn diagrams under `docs/images/diagrams/` (keep the editable `.excalidraw` source beside its exported `.png`).

## Rules

- Use a diagram only when relationships, state transitions, or branching paths are clearer visually than in prose.
- In sequence diagrams with three or more phases, wrap each phase in a `rect rgb(r, g, b)` block and title it with `Note over`. Skip bands in short diagrams.
- Keep one color meaning per document and state the legend once near the first diagram that uses it. Default light-theme palette:

| Fill                 | Meaning                    |
| -------------------- | -------------------------- |
| `rgb(255, 240, 240)` | **Request**                |
| `rgb(240, 240, 255)` | **Decision or routing**    |
| `rgb(240, 255, 240)` | **Data movement**          |
| `rgb(255, 255, 240)` | **Processing**             |
| `rgb(240, 255, 255)` | **Commit**                 |
| `rgb(255, 217, 217)` | **Stall or failure**       |

- Mirror the same meanings in flowcharts with `subgraph` plus `classDef` fills.
- **Never** let color carry meaning alone; every band and node needs a label that reads in monochrome.
- Instruction and register names in diagram labels follow [assembly.md](assembly.md).

## Review focus

- Does the diagram match the code or text it illustrates, step for step?
- Does it render? With no local `mmdc`, check it by pasting into the Mermaid Live Editor or previewing in VS Code.
- After an `.excalidraw` edit, was the `.png` re-exported?

## Decisions

Reviewers **must not** propose changes that reverse these.

<!-- - YYYY-MM-DD: <decision>. Reason: <why>. -->
