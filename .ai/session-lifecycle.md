# Session Lifecycle

Use this lifecycle for QmClient agent sessions. It keeps long-running work resumable and prevents scope drift.

## Start

1. Read `AGENTS.md` / `CLAUDE.md`.
2. Read `feature_list.json`.
3. Read `progress.md`.
4. Read `session-handoff.md`.
5. Run `./init.sh` when practical. If not, run `python qmclient_scripts/gate/check_workflow_docs.py`.
6. Check `git status --short` and treat unrelated changes as user work.

## Select

- Work on exactly one feature or one user-requested fix at a time.
- Prefer an existing `in-progress` feature from `feature_list.json`.
- If no active feature matches the user's request, update the state files only after the task is clearly defined.
- If the request is ambiguous, ask before implementation.

## Execute

- Read nearby source, call sites, config variables, translations, and tests before editing.
- Keep the patch within the smallest safe surface.
- Do not change high-risk DDNet behavior without explicit approval.
- Record important decisions in `progress.md` as they become durable.

## Verify

Use `.ai/verification.md` to choose the appropriate checks.

Evidence should include:

- Exact command.
- Result.
- Build/test warnings if any.
- Known unverified areas, especially visual checks.

## Wrap Up

Before ending a substantial session:

1. Update `progress.md` with completed work, evidence, blockers, and next action.
2. Update `feature_list.json` if a feature status or evidence changed.
3. Update `session-handoff.md` with current objective, files changed, known risks, and the exact resume command.
4. Leave the worktree understandable. Do not revert unrelated user changes.
5. If the feature is complete, ensure version metadata follows the MMP rule unless the user explicitly scoped the task away from code changes.

## Clean state expectation

A task is not complete just because code was edited. Completion requires implemented behavior plus relevant verification evidence. If verification cannot be run, state that clearly in `progress.md` and in the final response.
