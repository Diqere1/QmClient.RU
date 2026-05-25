# Code Review

Use this stance when the user asks for a review or when validating a patch before commit.

## Priority order

1. Correctness
2. Undefined behavior
3. Memory and lifetime safety
4. Thread safety
5. Hot-path performance
6. Protocol, file-format, map, physics, prediction, demo, replay, and rank compatibility
7. API stability
8. Maintainability
9. DDNet style consistency
10. Test and verification coverage

## Output format

List findings first, ordered by severity.

```text
[Critical] Title
path/to/file.cpp:123
Problem description.

Why it matters:
Explain the concrete DDNet/QmClient risk.

Fix:
Suggest the smallest safe correction.
```

Severity:

- `Critical`: likely crash, corruption, protocol/physics compatibility break, data loss, security issue, or severe user-facing regression.
- `Major`: real bug, lifecycle issue, hot-path regression, missing validation, or behavior mismatch.
- `Minor`: low-risk maintainability, localized edge case, or test gap.

After findings, include:

- Overall verdict: `正确`, `需要修复`, or `不安全`.
- Brief summary.
- Optional refactoring suggestions if they are not needed for correctness.

## Review rules

- Focus on real defects, not cosmetic preferences.
- Explain why a problem matters in DDNet/QmClient terms.
- Prefer minimal patch suggestions.
- If a behavior might be historical compatibility but you are not sure, say so.
- Do not suggest broad style rewrites that fight the surrounding code.
