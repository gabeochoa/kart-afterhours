# CORRECT — repeat-mistake classes

| Rule | Banned shape | Fix level | Evidence (x2+) |
|---|---|---|---|
| CORRECT-K1 | Raw component pointers batched in `for_each_with`, rendered from `once()` | Architecture already (`after()`, 53fa0e9) + lint guard | 53fa0e9: stale frame + UAF on eliminated kart, `tests/e2e/14` segfaults without; template repo still had the `once()` shape (now fixed there) — recurrence path is copying |
| CORRECT-K2 | e2e exits 0 on failure; e2e reads/writes player save | Behaviour tests already + lint guard | TODO Tier 1 "E2E failures cannot fail anything" (fixed `has_failed()?1:0`); c77151e save-drift period-4 + clobbering player settings (fixed `autosave_enabled=false`+`load_defaults`, `settings.cpp` early-return) |

Why guard-level now: both fixes are already at the highest level (hook choice; exit-code/save-isolation design in `main/settings/e2e_integration`). The missing level was CI naming the fix — an agent copying an old snippet had nothing fail. `scripts/check_correct.py` (`make check`, prerequisite of `make ci`) is that lint.
Commits (local, one per class): `a5376c3` K1 guard+`make check/ci`, `e85bcb7` K2 guards. (Architecture/behaviour fixes themselves: 53fa0e9, c77151e and TODO Tier 1, pre-existing.)
Proof: checker passes HEAD; fails `53fa0e9^` systems.h (K1) and `c77151e^` main.cpp/settings.cpp (K2, all 3 rules) — verified in /tmp/kart-past.
Not re-acted on: untranslated literals etc. remain tracked in TODO.md Tier 1/2 — single-source there, no duplicate rule added here.
