# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and run

```bash
make          # build → produces ./tint
make clean    # remove object files and binary
./tint -s -n  # run with shadow and next-piece preview
```

Run the engine unit tests with `make test`; they exercise board initialisation, movement, line clearing, the 7-bag randomiser, shadow tracking, game-over detection, and `str2int`. No external framework required.

Options: `-l <1-9>` starting level, `-n` show next piece, `-d` dotted grid, `-s` shadow, `-b <char>` block character.

## Architecture

The codebase has three layers with clean separation:

**`engine.c/h`** — pure game logic, no I/O. Owns the board (`board_t`, a `int[NUMCOLS][NUMROWS]` array indexed `[x][y]`), all piece state, and the 7-bag randomiser. The main entry points are `engine_init`, `engine_move`, and `engine_evaluate`. `engine_evaluate` drives one game tick and returns `1` (piece moved down), `0` (piece landed, new piece spawned), or `-1` (game over).

**`io.c/h`** — thin ncurses wrapper. All rendering goes through `out_*` functions; all input through `in_*`. Nothing else in the codebase calls ncurses directly.

**`tint.c`** — game loop, rendering, scoring, high scores. Calls `engine_move` on keypress, calls `engine_evaluate` on timeout, then calls `drawboard(engine.board)` to repaint. The score function is passed as a callback to `engine_init`.

## Key non-obvious details

**Board indexing is `[x][y]` (column first).** Row 0 is the top; columns 0, NUMCOLS-2, and NUMCOLS-1, and rows NUMROWS-2 and NUMROWS-1 are sentinel `WALL` cells that bound the playfield. `allowed()` relies on these walls to reject out-of-bounds positions.

**Each board cell renders as two terminal characters side-by-side** (`blockchar blockchar`) so cells appear square in a fixed-width font. All `out_gotoxy` x-coordinates for board cells use `x * 2`.

**Shadow and piece share the same color value on the board.** The renderer distinguishes them using `engine->curx_shadow` / `engine->cury_shadow` coordinates, not board cell values. Every movement function erases both piece and shadow before calling `allowed()`, then redraws both — leaving either on the board would cause it to block its own movement.

**Score is stored at `2×` face value** (`SCOREFACTOR = 2`). Showing next piece and dotted lines each halve the raw score via integer division; doubling the stored value keeps arithmetic exact. `GETSCORE(score)` divides by 2 for display.

**The score file path (`/var/games/tint.scores`) is baked in at compile time** via the `SCOREFILE` macro in the Makefile. The binary writes directly to that path and will fail silently if it is not writable.

**`USE_RAND`** (commented out in `CPPFLAGS`) switches from `random()` to `rand()` in `utils.c`. Leave it commented for better randomness.
