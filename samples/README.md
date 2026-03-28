# Samples (aligned with *Compiler Design Project Idea.pdf*)

Use these files to **manually** (or later with a script) run your compiler and check behavior as you implement each feature.

```text
mingw32-make
.\ekker.exe samples\<folder>\sample.txt
```

**Current grammar** (see `parser.y`) allows only: `number id;`, `id = expression;`, `show(id);`, and integer expressions with `+ - * /`.

| Idea PDF theme | Folder | In this repo |
|----------------|--------|----------------|
| Baseline / happy path | `ok/` | Runnable today |
| Uninitialized variables | `uninitialized_variable/` | Sample parses; **warning** needs semantic pass |
| Sensitive data exposure | `sensitive_data_exposure/` | Sample parses; **warning** needs marking vars / names in symtab |
| Division by zero risk | `division_by_zero/` | Parses; **check** needs constant/eval pass |
| Infinite loop detection | `future_from_proposal/` | Syntax not in `parser.y` yet — target program documented there |
| Array index out-of-bounds | `future_from_proposal/` | Same |
| Dead code after return | `future_from_proposal/` | Same |
| Type / mismatch detection | `future_from_proposal/` | Same |
| Undeclared use (baseline errors) | `undeclared_errors/` | Runnable; your parser already reports some errors |

As you add `repeat`, `check`/`otherwise`, `give`, arrays, etc., move or duplicate examples from `future_from_proposal/` into their own folders and mark them runnable in the table above.
