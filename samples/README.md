# Samples

```text
mingw32-make
.\ekker.exe samples\<folder>\<file>.txt
```

Optional log file: `.\ekker.exe samples\ok\minimal.txt build.log`

## Pipeline (see `doc/INDEX_PDF_CHECKLIST.md`)

1. Token dump (Flex)  
2. Parse → AST  
3. Symbol table collection  
4. Type checking + security warnings/errors  
5. Dead-code warnings  
6. **Constant folding** (AST)  
7. **3-address IR** for all `define` functions + `start`  
8. **Stack-machine code generation**  
9. Symbol table print  

## Constant folding (optimization) demo

Run:

`.\ekker.exe samples\ok\constant_folding.txt`

- Step **1c** prints the **AST before** optimization (you still see nested `BINOP` trees for `1+2+3`, etc.).
- Step **3c** runs **constant folding** on that AST (see `optimize.c`).
- Steps **4** and **4b** show **IR** and **stack code** after folding: pure constant expressions become a **single** literal (e.g. `t? = 10` for `2*3+4`, not three temps and two adds).

Details: `doc/OPTIMIZATION.md` (short explanation).

## Folders

| Folder | Purpose |
|--------|---------|
| `ok/` | Happy-path programs (includes `constant_folding.txt`) |
| `loop/` | `repeat` loops |
| `division_by_zero/` | Parse-time `/ 0` |
| `sensitive_data_exposure/` | Sensitive `show` warning |
| `uninitialized_variable/` | Uninit use warning |
| `undeclared_errors/` | Undeclared id errors |
| `error/` | OOB arrays, dead code, type mismatch, infinite loop hints, etc. |

Invalid source (e.g. `@`) triggers a **lexical error** and `INVALID_CHAR` so the parser fails with a syntax error.
