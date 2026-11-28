# Armito

Armito is a modular project for translating and preparing operations across processor architectures. Its current foundations include an AArch64 decoder, a universal intermediate representation, CPU state models, and a Core lifecycle that coordinates modules.

## Repository layout

- `Boot/` — firmware entry and boot handoff.
- `Core/` — module lifecycle, configuration, dispatch, and coordination.
- `Architecture/AARCH64/` — source-architecture CPU state and architecture-specific behavior.
- `Formats/MachO/` — Mach-O container parsing for architecture identification.
- `Decoder/` — instruction recognition, validation, and operand decoding.
- `IR/` — the universal intermediate representation.
- `Translator/JIT/` — JIT translation area under development.
- `Hardware/` — device identification, descriptions, and capabilities.
- `Platform/` — machine models (Mac14_3, FX8320E).
- `Compatibility/` — CPU and GPU adaptation mechanisms (logic separated from hardware descriptions).
- `Tools/Catalog/` — catalog maintenance and generation tooling.
- `Tests/` — host tests grouped by subsystem; AArch64 fixtures are in `Tests/Fixtures/AARCH64/`.
- `Docs/` — architecture, compatibility, status, decisions, and roadmap documentation.

## Host tests

Run the host suite from the repository root:

```sh
make test
```

The catalog-specific structural and differential checks can also be run with:

```sh
make catalog-test
```

Catalog data remains with the AArch64 decoder (`Decoder/AARCH64/Catalog/`); the generation tool lives in `Tools/Catalog/catalog_tool.py`. Test-only samples are kept under `Tests/Fixtures/`.

## Documentation

- [Documentation index](Docs/README.md)
- [Current status](Docs/STATUS.md)
- [Architecture](Docs/ARCH.md)
- [Compatibility](Docs/COMPATIBILITY.md)
- [Roadmap](Docs/ROADMAP.md)
- [Known failures and limitations](Docs/FAILS.md)
- [Decisions](Docs/DECISIONS.md)

Recognition and architectural validation do not imply that every catalogued instruction has IR translation or an execution backend. See the status and compatibility documents for implemented coverage.

## License

Copyright © 2026 SoulDotPrime.

Armito is licensed under the GNU General Public License v2.0 only (GPL-2.0-only).

Armito is an original project developed by SoulDotPrime. Independent forks are encouraged to clearly identify themselves as unofficial derivatives and preserve the original attribution.

See "LICENSE" [License](LICENSE) for the full license terms.
