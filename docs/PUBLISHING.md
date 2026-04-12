# netPong Publishing Checklist

This file is the local checklist I will use before pushing anything to GitHub.

## Remote Setup

1. Create the GitHub repository manually on my account.
2. Add it as `origin`.
3. Push `main`.
4. Push `phase2`.
5. Push annotated tags.

Commands:

```bash
git remote add origin <REPO_URL>
git push -u origin main
git push -u origin phase2
git push origin v0.1-original-pong
git push origin v0.2-phase1-analysis
```

When the first real Phase 2 milestone exists:

```bash
git push origin v0.3-phase2
```

## Release Standard

Before I publish a release:

- the tagged revision builds
- the release notes describe the tagged revision, not branch head
- the docs say what is present and what is still missing
- no course-owned PDFs, templates, or submission artifacts are tracked
- the writing sounds like an engineering log, not a product page

## Current State

- `v0.1-original-pong` exists locally
- `v0.2-phase1-analysis` exists locally
- `v0.3-phase2` is intentionally not created yet

I am holding the Phase 2 release until the code actually contains the networking work it claims to have.
