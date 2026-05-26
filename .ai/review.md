# 代码审查

当用户要求 review，或者我们在提交前验证补丁时，采用这套审查立场。

## 优先级顺序

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

## 输出格式

先列 findings，并按严重度排序。

```text
[Critical] Title
path/to/file.cpp:123
问题描述。

Why it matters:
解释它在 DDNet/QmClient 里的具体风险。

Fix:
给出最小且安全的修正建议。
```

严重度：

- `Critical`: likely crash, corruption, protocol/physics compatibility break, data loss, security issue, or severe user-facing regression.
- `Major`: real bug, lifecycle issue, hot-path regression, missing validation, or behavior mismatch.
- `Minor`: low-risk maintainability, localized edge case, or test gap.

列完 findings 后，再补：

- Overall verdict: `正确`, `需要修复`, or `不安全`.
- Brief summary.
- Optional refactoring suggestions if they are not needed for correctness.

## 审查规则

- Focus on real defects, not cosmetic preferences.
- Explain why a problem matters in DDNet/QmClient terms.
- Prefer minimal patch suggestions.
- If a behavior might be historical compatibility but you are not sure, say so.
- Do not suggest broad style rewrites that fight the surrounding code.
