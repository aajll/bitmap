# Security Policy

## Reporting a Vulnerability

Use GitHub's Security Advisories feature to report security concerns privately.

Expect a response within 7 days. If the issue is confirmed, a fix will be released as a patch version.

## Scope

`bitmap` is a small library with no network stack, no external dependencies, and no dynamic memory allocation. The primary attack surface is an out-of-range bit index, an undersized backing array or snapshot destination, and integer or shift behaviour at the capacity boundaries. The library bounds-checks indices against the descriptor's declared `bit_count`, but it cannot discover the real length of an arbitrary pointer. A forged descriptor or a misdeclared storage capacity is caller misuse.
