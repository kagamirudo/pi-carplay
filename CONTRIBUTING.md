# Contributing

Thanks for helping. pi-carplay runs in cars, so changes go through the rules in [docs/quality.md](docs/quality.md). In short:

1. Branch from `main` and open a pull request. Direct pushes to `main` are blocked.
2. Run `make check` before pushing. CI runs the same gates and the `CI passed` check must be green to merge.
3. Every bug fix comes with a test that fails without it. See [docs/testing.md](docs/testing.md) for where tests go.
4. Keep coverage from dropping. If you raise it, raise `COVERAGE_MIN` in the Makefile in the same pull request.
5. Add a line under `Unreleased` in `CHANGELOG.md` for anything a user would notice.

## Setting up

```sh
make deps deps-qa   # build and test tools (sudo apt)
make build          # third-party stack and the shell
make check          # lint, analysis, tests, sanitizers, coverage
```

On a Pi with the display attached, `make install` and `make restart` put your build on screen, and `make selftest` checks the hardware.

## Code

- C++20 and Qt 6 for the shell. Match the surrounding style. The build uses `-Wall -Wextra -Wpedantic -Werror`.
- QML must be clean under qmllint, and must not log a warning at run time (the UI tests fail on any).
- Shell scripts must pass shellcheck.
- Changes to OpenAuto or aasdk go in `patches/` (GPL-3.0), never in the MIT tree. Regenerate the patch from your working tree with `git -C ~/src/openauto diff > patches/openauto.patch`.

## Commits and pull requests

- One topic per pull request. Write the description for a reviewer who has not seen the code: what changed, why, and how you tested it.
- Fill in the pull request checklist. For changes that touch the display, touch, Bluetooth, USB or start-up, say which hardware checklist items you ran.
- Squash-merge is the default. The squash title becomes the commit message, so keep it short and in the imperative mood.

## Licensing

The shell and tooling are MIT. `patches/` is GPL-3.0, like the projects it patches. By contributing you agree your contribution is licensed the same way as the files it changes.
