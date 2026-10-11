# Contributing

Issues and pull requests are welcome. Read the [README](README.md) and [Architecture](docs/architecture.md) first.

- Issues should include the commit, board and wiring, SDK and compiler versions, reproduction steps, and relevant logs.
- Pull requests should explain the problem, the resulting behavior, and build results. Include hardware results when a board is available; state explicitly when hardware testing was not performed.
- Before submitting, run the Release build described in the README and `git diff --check`. For performance changes, record measurements before and after the change in this project, using the same lighting, clocks, and model. Testing other repositories is not required.
- Use straightforward C/C++ functions and clear loops. Keep application, driver, and inference responsibilities separate. Avoid unnecessary templates, complex macros, or additional frameworks.
- Preserve original copyright and license notices when modifying third-party sources, and document the changes. Record sources and licenses for new dependencies, models, or assets.
- Original contributions are submitted under Apache-2.0. Third-party code remains subject to its original license.

This repository contains firmware and build/flash entry points, with no standalone test directory. Record the validation steps actually performed in your pull request.
