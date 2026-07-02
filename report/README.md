# DJS Report

## Prerequisites

- XeLaTeX (for Times New Roman support)
- Node.js (for Mermaid diagram generation)
- `@mermaid-js/mermaid-cli` (mmdc)

Install mmdc:
```bash
npm install -g @mermaid-js/mermaid-cli
```

## Compiling the Report

```bash
cd report

# First, generate/update diagrams from Mermaid sources:
make -C diagrams

# Then compile LaTeX (run twice for cross-references):
xelatex report.tex
xelatex report.tex
```

## Adding or Updating Diagrams

1. Edit the `.mmd` files in `diagrams/`
2. Run `make -C diagrams` to regenerate PNGs into `figures/`
3. Recompile the report with `xelatex report.tex`

### Diagram Source Files

| File | Diagram Type | Description |
|------|-------------|-------------|
| `diagrams/use-case.mmd` | flowchart | Use case diagram |
| `diagrams/class-analysis.mmd` | classDiagram | Analysis-level class diagram |
| `diagrams/sequence-job-lifecycle.mmd` | sequenceDiagram | Complete job lifecycle |
| `diagrams/sequence-worker-registration.mmd` | sequenceDiagram | Worker registration flow |
| `diagrams/sequence-metrics-reporting.mmd` | sequenceDiagram | Metrics reporting flow |
| `diagrams/state-job-states.mmd` | stateDiagram-v2 | Job state machine |
| `diagrams/activity-workflow.mmd` | flowchart | Scheduling/execution workflow |
| `diagrams/class-design.mmd` | classDiagram | Refined design-level class diagram |
| `diagrams/component-diagram.mmd` | flowchart | System components |
| `diagrams/deployment-diagram.mmd` | flowchart | Deployment architecture |
| `diagrams/er-diagram.mmd` | erDiagram | Entity-relationship diagram |

## File Structure

```
report/
├── report.tex         # Main LaTeX file
├── figures/           # Generated PNGs (from make)
├── diagrams/          # Mermaid source files (version-tracked)
│   ├── *.mmd
│   └── Makefile
├── chapters/          # Chapter .tex files
│   ├── cover.tex
│   ├── certificate.tex
│   ├── acknowledgement.tex
│   ├── abstract.tex
│   ├── chapter1_introduction.tex
│   ├── chapter2_background.tex
│   ├── chapter3_analysis.tex │   ├── chapter4_design.tex
│   ├── chapter5_implementation.tex
│   ├── chapter6_conclusion.tex
│   ├── references.tex
│   └── appendix.tex
└── formats/           # Original format specification files
```

# using opencode

exporting session:
```bash
opencode export > .opencode/sessions/latest-session.json
```

importing and using sessions:
```bash
opencode import .opencode/sessions/latest-session.json

opencode --session <session_id>
```
