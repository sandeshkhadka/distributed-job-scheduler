# DJS Report

To compile:
```bash
cd report
xelatex report.tex   # run twice for cross-references
```

To use the sessions that have been shared from
your co-workers, use
```bash
opencode import .opencode/sessions/latest-session.json

# or open with that session

opencode --session <session_id>
```

To export your current session, run
```bash
opencode export > .opencode/sessions/latest-session.json

```
