# Ekker Lang 1

A small **Compiler Design** project using **Flex** (lexical analysis) and **Bison** (syntax analysis). This repo is the stripped-down workspace for experimenting with **Ekker-style** keywords and tokens before wiring a full parser and backend.

## Current status

| Piece | Status |
|--------|--------|
| **Flex scanner** | `scanner.l` — tokenizes keywords, operators, literals, identifiers |
| **Bison grammar** | Add `parser.y` (must declare the same tokens Flex returns) |
| **Driver** | Add `main.c` (e.g. open input/output files, set `yyin`, call `yyparse()`) |
| **Makefile** | Optional — run `flex` / `bison` / `gcc` in that dependency order |

Until `parser.y` exists, run **Bison first** so `parser.tab.h` is generated; then Flex can compile `scanner.l`.

## Requirements

- **Flex** (e.g. GnuWin32 Flex 2.5.x)
- **GNU Bison** (e.g. 2.4.x)
- **GCC** (or another C compiler)
- A Unix-style shell for **Make** if you use a Makefile (MSYS2, Git Bash, etc.)

## Language surface (lexer)

The scanner recognizes tokens aligned with an Ekker-like surface syntax:

| Role | Keywords / tokens |
|------|-------------------|
| Types | `number`, `decimal` |
| I/O | `show` |
| Control | `check`, `otherwise` |
| Loops | `repeat` |
| Functions | `start`, `give` |
| Security markers | `secret`, `password`, `key`, `token` |
| Booleans | `true`, `false` |
| Operators | `==`, `!=`, `>=`, `<=`, `>`, `<`, `=`, `+`, `-`, `*`, `/` |
| Symbols | `( ) { } ;` |
| Literals | integer **NUMBER**, **ID** identifiers |

Invalid single characters are reported with **line numbers** (`%option yylineno`). The scanner uses **`%option noyywrap`** so you do not need `-lfl` on Windows in typical setups.

## Suggested invocation

Match your course layout: **input file** → **output file**, for example:

```text
./program sample.txt out.txt
```

In `main.c`, assign `yyin = fopen(argv[1], "r")` and open `argv[2]` for anything you write (IR, messages, etc.).

## Build (manual)

From this directory, after you add `parser.y`:

```text
bison -d -o parser.tab.c parser.y
flex -o lex.yy.c scanner.l
gcc -o program main.c lex.yy.c parser.tab.c
```

Add more `.c` files (AST, symbol table, codegen) to the `gcc` line as the project grows.

## Clean

Remove generated artifacts when troubleshooting:

```text
rm -f program program.exe lex.yy.c parser.tab.c parser.tab.h
```

On plain **cmd.exe**, use `del` instead of `rm` if you are not using a Unix-like shell.

## Repository

Git metadata lives in `.git/`; add a `.gitignore` for `lex.yy.c`, `parser.tab.c`, `parser.tab.h`, and the compiled executable when you are ready.

## Sample programs

The **`samples/`** directory groups test inputs by theme (matching **`compiler Design Project Idea.pdf`**: uninitialized use, sensitive exposure, division by zero, etc.). See **`samples/README.md`** for the index and what the **current** grammar can run vs **future** targets.

## See also

Parent folder **`BEGINNER_PROJECT_GUIDE.md`** — project structure, Makefile template, and rubric alignment with `index.pdf`.
