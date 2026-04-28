## Summary

-

## Docs Updated

<!-- Consult the doc ownership map in AGENTS.md. List each applicable doc and why it was updated.
     If a row in the ownership map applies but no update was needed, say why (e.g., "docs/architecture.md — no new subsystems added").
     If no rows apply, state that explicitly. -->

-

## Validation

-

## Risks Or Follow-Ups

-

## Automation or Process Changes

<!-- Complete this section if this PR touches .github/**, docs/**, AGENTS.md, or README.md. Delete if not applicable. -->

- **Workflow trigger/event expectations** (which events, paths, or conditions control when this runs):
- **Auth/secrets/permissions affected** (tokens required, scopes needed, behavior when absent):
- **Output/format contract** (expected output schema or side-effects, if applicable):
- **Fallback/error behavior** (what happens on failure or unexpected output):
- **Validation evidence** (how was this verified without relying on a live PR review round):

## Checklist

- [ ] Doc ownership map checked; each applicable row listed in "Docs Updated" above (or N/A with reason)
- [ ] No third-party libraries were introduced
- [ ] Each new or changed behavior is covered by a named test that fails if that specific behavior breaks
- [ ] Failure paths, init/shutdown behavior, and Win32 lifecycle edges verified or stated N/A
- [ ] All affected chess rule surfaces verified for both White and Black (or N/A — not a chess-rules change)
- [ ] The change was self-reviewed for architectural drift
