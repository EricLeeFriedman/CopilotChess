#!/usr/bin/env python3
"""
validate-pr-review.py

Offline validation for the PR review workflow logic extracted from
.github/workflows/pr-review.yml.

Run this script to verify that the parse_review function and the PR
classification logic behave correctly without requiring a live GitHub
pull request or Copilot CLI invocation.

Usage:
    python3 .github/scripts/validate-pr-review.py

Exit code 0 means all tests passed. Any failure prints the failing case
and exits with code 1.
"""

import json
import sys

# ---------------------------------------------------------------------------
# Logic extracted from pr-review.yml "Post GitHub review" step
# ---------------------------------------------------------------------------

FALLBACK_REVIEW = {
    "event": "REQUEST_CHANGES",
    "body": "@copilot Automated review could not parse structured output. Manual inspection required.",
    "blocking": [],
    "comments": [],
}


def parse_review(text):
    """Parse text as JSON and return a validated review dict.

    Returns a fallback REQUEST_CHANGES dict on any parse or schema error.
    This function mirrors the parse_review logic embedded in pr-review.yml.
    """
    import re

    cleaned = text.replace("\r", "").lstrip("\ufeff")
    cleaned = re.sub(r"\x1b\[[0-9;?]*[ -/]*[@-~]", "", cleaned).strip()

    if cleaned.startswith("```"):
        lines = cleaned.splitlines()
        if len(lines) >= 3:
            cleaned = "\n".join(lines[1:-1]).strip()

    try:
        parsed = json.loads(cleaned)
    except json.JSONDecodeError as exc:
        start = cleaned.find("{")
        end = cleaned.rfind("}")
        if start >= 0 and end > start:
            try:
                parsed = json.loads(cleaned[start : end + 1])
            except json.JSONDecodeError:
                return dict(FALLBACK_REVIEW)
        else:
            return dict(FALLBACK_REVIEW)

    if not isinstance(parsed, dict):
        return dict(FALLBACK_REVIEW)

    if "event" in parsed and not isinstance(parsed["event"], str):
        return dict(FALLBACK_REVIEW)

    for key in ("blocking", "comments"):
        if key not in parsed:
            return dict(FALLBACK_REVIEW)
        if not isinstance(parsed[key], list):
            return dict(FALLBACK_REVIEW)

    for idx, entry in enumerate(parsed["blocking"]):
        if not isinstance(entry, dict):
            return dict(FALLBACK_REVIEW)
        for required_key in ("path", "line", "body"):
            if required_key not in entry:
                return dict(FALLBACK_REVIEW)
        try:
            line_val = int(entry["line"])
        except (ValueError, TypeError):
            return dict(FALLBACK_REVIEW)
        if line_val <= 0:
            return dict(FALLBACK_REVIEW)

    return parsed


# ---------------------------------------------------------------------------
# Logic extracted from pr-review.yml "Classify PR" step
# ---------------------------------------------------------------------------


def classify_pr(changed_paths):
    """Return True if the PR touches only process/docs/workflow files.

    A PR is a 'code PR' when it contains any file under src/ or build.ps1.
    Everything else (docs/, .github/**, AGENTS.md, README.md, etc.) is
    treated as a process-only change that skips the cpp-pr-review agent.
    """
    has_code = any(
        p.startswith("src/") or p == "build.ps1" for p in changed_paths
    )
    return not has_code


# ---------------------------------------------------------------------------
# Test helpers
# ---------------------------------------------------------------------------

_PASS = 0
_FAIL = 0


def _check(label, actual, expected):
    global _PASS, _FAIL
    if actual == expected:
        print(f"  PASS  {label}")
        _PASS += 1
    else:
        print(f"  FAIL  {label}")
        print(f"        expected: {expected!r}")
        print(f"        actual:   {actual!r}")
        _FAIL += 1


# ---------------------------------------------------------------------------
# parse_review tests
# ---------------------------------------------------------------------------


def test_parse_review_valid_approve():
    r = parse_review(
        json.dumps({"event": "APPROVE", "body": "lgtm", "blocking": [], "comments": []})
    )
    _check("valid APPROVE parses cleanly", r["event"], "APPROVE")
    _check("valid APPROVE has empty blocking", r["blocking"], [])


def test_parse_review_valid_request_changes():
    payload = {
        "event": "REQUEST_CHANGES",
        "body": "@copilot fix this",
        "blocking": [{"path": "src/main.cpp", "line": 10, "body": "@copilot missing test"}],
        "comments": [],
    }
    r = parse_review(json.dumps(payload))
    _check("REQUEST_CHANGES with blocking entry", r["event"], "REQUEST_CHANGES")
    _check("blocking entry path preserved", r["blocking"][0]["path"], "src/main.cpp")
    _check("blocking entry line preserved", r["blocking"][0]["line"], 10)


def test_parse_review_invalid_json():
    r = parse_review("not json at all")
    _check("invalid JSON returns fallback event", r["event"], "REQUEST_CHANGES")
    _check("invalid JSON returns empty blocking", r["blocking"], [])


def test_parse_review_missing_blocking_key():
    r = parse_review(json.dumps({"event": "COMMENT", "body": "ok", "comments": []}))
    _check("missing 'blocking' key returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_missing_comments_key():
    r = parse_review(json.dumps({"event": "COMMENT", "body": "ok", "blocking": []}))
    _check("missing 'comments' key returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_blocking_not_a_list():
    r = parse_review(
        json.dumps({"event": "COMMENT", "body": "ok", "blocking": "nope", "comments": []})
    )
    _check("blocking not a list returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_non_dict_response():
    r = parse_review(json.dumps([1, 2, 3]))
    _check("non-dict JSON returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_blocking_entry_missing_line():
    payload = {
        "event": "REQUEST_CHANGES",
        "body": "@copilot",
        "blocking": [{"path": "src/x.cpp", "body": "@copilot missing line key"}],
        "comments": [],
    }
    r = parse_review(json.dumps(payload))
    _check("blocking entry missing 'line' returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_blocking_entry_negative_line():
    payload = {
        "event": "REQUEST_CHANGES",
        "body": "@copilot",
        "blocking": [{"path": "src/x.cpp", "line": -1, "body": "@copilot bad line"}],
        "comments": [],
    }
    r = parse_review(json.dumps(payload))
    _check("blocking entry negative line returns fallback", r["event"], "REQUEST_CHANGES")


def test_parse_review_json_in_markdown_fence():
    inner = {"event": "COMMENT", "body": "@copilot ok", "blocking": [], "comments": []}
    fenced = f"```json\n{json.dumps(inner)}\n```"
    r = parse_review(fenced)
    _check("JSON in markdown fence is unwrapped", r["event"], "COMMENT")


def test_parse_review_ansi_escape_codes_stripped():
    inner = json.dumps(
        {"event": "COMMENT", "body": "@copilot ok", "blocking": [], "comments": []}
    )
    # Wrap in ANSI escape sequences as Copilot CLI sometimes produces
    ansi_wrapped = f"\x1b[32m{inner}\x1b[0m"
    r = parse_review(ansi_wrapped)
    _check("ANSI escape codes stripped before parsing", r["event"], "COMMENT")


def test_parse_review_event_not_string():
    payload = {
        "event": 42,
        "body": "@copilot",
        "blocking": [],
        "comments": [],
    }
    r = parse_review(json.dumps(payload))
    _check("non-string 'event' returns fallback", r["event"], "REQUEST_CHANGES")


# ---------------------------------------------------------------------------
# classify_pr tests
# ---------------------------------------------------------------------------


def test_classify_pure_docs_pr():
    paths = ["docs/workflow.md", "docs/architecture.md"]
    _check("pure docs PR is classified as process", classify_pr(paths), True)


def test_classify_pure_github_pr():
    paths = [".github/workflows/pr-review.yml", ".github/ISSUE_TEMPLATE/workflow.yml"]
    _check("pure .github PR is classified as process", classify_pr(paths), True)


def test_classify_agents_md_pr():
    paths = ["AGENTS.md"]
    _check("AGENTS.md-only PR is classified as process", classify_pr(paths), True)


def test_classify_readme_pr():
    paths = ["README.md"]
    _check("README.md-only PR is classified as process", classify_pr(paths), True)


def test_classify_src_pr():
    paths = ["src/chess.cpp", "src/chess.h"]
    _check("src/ PR is classified as code (not process)", classify_pr(paths), False)


def test_classify_build_ps1_pr():
    paths = ["build.ps1"]
    _check("build.ps1 PR is classified as code (not process)", classify_pr(paths), False)


def test_classify_mixed_pr():
    paths = ["src/chess.cpp", "docs/architecture.md"]
    _check("mixed src+docs PR is classified as code (not process)", classify_pr(paths), False)


def test_classify_mixed_github_and_src():
    paths = [".github/workflows/pr-review.yml", "src/main.cpp"]
    _check(
        "mixed .github+src PR is classified as code (not process)",
        classify_pr(paths),
        False,
    )


def test_classify_empty_files():
    _check("empty file list is classified as process", classify_pr([]), True)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    print("Running PR review validation tests...")
    print()

    print("--- parse_review tests ---")
    test_parse_review_valid_approve()
    test_parse_review_valid_request_changes()
    test_parse_review_invalid_json()
    test_parse_review_missing_blocking_key()
    test_parse_review_missing_comments_key()
    test_parse_review_blocking_not_a_list()
    test_parse_review_non_dict_response()
    test_parse_review_blocking_entry_missing_line()
    test_parse_review_blocking_entry_negative_line()
    test_parse_review_json_in_markdown_fence()
    test_parse_review_ansi_escape_codes_stripped()
    test_parse_review_event_not_string()

    print()
    print("--- classify_pr tests ---")
    test_classify_pure_docs_pr()
    test_classify_pure_github_pr()
    test_classify_agents_md_pr()
    test_classify_readme_pr()
    test_classify_src_pr()
    test_classify_build_ps1_pr()
    test_classify_mixed_pr()
    test_classify_mixed_github_and_src()
    test_classify_empty_files()

    print()
    total = _PASS + _FAIL
    print(f"Results: {_PASS}/{total} passed, {_FAIL}/{total} failed.")

    if _FAIL > 0:
        sys.exit(1)
    else:
        print("All tests passed.")
        sys.exit(0)


if __name__ == "__main__":
    main()
