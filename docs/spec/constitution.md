# insomniaTV constitution

Working principles for changes to this repository. Short on purpose; each one exists because ignoring it already cost us something.

1. **Thin wiring on a maintained base.** Prefer a maintained library or framework with a small amount of project-specific code on top, so an upgrade means swapping the base. Weigh this in every architecture decision (see #80).
2. **Real logic stays testable, and tests don't hide logic.** Decision logic lives behind small interfaces (`src/hal/`, `ISamsungTvTransport`) so native tests exercise the real code. Never wrap logic in `#ifdef ARDUINO` stubs to make tests pass (this caused #65, #67 and #69).
3. **State is read, not assumed.** A command that toggles (Samsung `KEY_POWER`) is sent only after reading the current state. A stub that reports success without acting (the IR driver today) is a known gap, listed in the spec, not a feature.
4. **Everything goes through a PR.** No direct pushes to `master`. CI must pass: lint, format, native unit tests, and the firmware build. The PlatformIO version is pinned (<6.2) because 6.2.0 broke the build.
5. **Verify on the device before calling it done.** Native tests and CI prove compilation and logic; behaviour against the real TV and network is checked on hardware, and PR descriptions say which of the two was done.
6. **No secrets in the repo.** The hardcoded web credentials are a known violation (#51), not a pattern to copy.
7. **Docs describe what exists.** `docs/spec/spec.md` states what is built, stubbed, or planned; update it in the same PR that changes behaviour.
