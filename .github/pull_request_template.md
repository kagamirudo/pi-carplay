## What and why

<!-- What changes for the driver or the developer, and why. Link the issue. -->

## How it was tested

<!-- Tests added or changed. For hardware-facing changes, which hardware checklist items you ran (docs/quality.md). -->

## Checklist

- [ ] `make check` passes locally
- [ ] Bug fixes include a test that fails without the fix
- [ ] New behaviour is covered by a test at the lowest level that can catch it
- [ ] No new warnings (compiler, shellcheck, cppcheck, qmllint, QML at run time)
- [ ] `CHANGELOG.md` has a line under `Unreleased` for anything a user would notice
- [ ] Docs updated (README, docs/) if setup, usage or behaviour changed
- [ ] OpenAuto/aasdk changes are in `patches/`, not in the MIT tree
