#include <stdio.h>
#include <string.h>

#include "../engine.h"
#include "../utils.h"

#define FILL_COLOR 1  /* arbitrary non-zero, non-WALL value used to fill cells in tests */

static int passed = 0, failed = 0;

#define CHECK(cond, name) \
    do { \
        if (cond) { printf("ok - %s\n", (name)); passed++; } \
        else { printf("FAIL - %s  [%s:%d]\n", (name), __FILE__, __LINE__); failed++; } \
    } while (0)

static void no_score(engine_t *e) { (void)e; }

/* board initialisation */

static void test_board_walls(void)
{
    engine_t e;
    engine_init(&e, no_score);
    int ok;
    ok = 1;
    for (int y = 0; y < NUMROWS; y++) if (e.board[0][y] != WALL) { ok = 0; break; }
    CHECK(ok, "left wall (col 0) is all WALL");
    ok = 1;
    for (int y = 0; y < NUMROWS; y++)
        if (e.board[NUMCOLS-1][y] != WALL || e.board[NUMCOLS-2][y] != WALL) { ok = 0; break; }
    CHECK(ok, "right walls (cols NUMCOLS-1 and NUMCOLS-2) are all WALL");
    ok = 1;
    for (int x = 0; x < NUMCOLS; x++)
        if (e.board[x][NUMROWS-1] != WALL || e.board[x][NUMROWS-2] != WALL) { ok = 0; break; }
    CHECK(ok, "bottom walls (rows NUMROWS-1 and NUMROWS-2) are all WALL");
}

static void test_initial_state(void)
{
    engine_t e;
    engine_init(&e, no_score);
    CHECK(e.score == 0,       "score is 0 after init");
    CHECK(e.curx == 5,        "piece spawns at x=5");
    CHECK(e.cury == 1,        "piece spawns at y=1");
    CHECK(e.shadow == FALSE,  "shadow is disabled by default");
}

static void test_interior_cell_clear(void)
{
    engine_t e;
    engine_init(&e, no_score);
    /* (1,10) is far from the spawn area — must always be empty */
    CHECK(e.board[1][10] == 0, "interior cell (1,10) is empty after init");
}

/* engine_evaluate return values */

static void test_evaluate_falls(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    CHECK(engine_evaluate(&e) == 1, "evaluate returns 1 while piece is in free fall");
}

static void test_evaluate_lands(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    int result, ticks = 0;
    do { result = engine_evaluate(&e); } while (result == 1 && ++ticks < 100);
    CHECK(result == 0, "evaluate returns 0 when piece lands");
}

/* movement */

static void test_move_down(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    int y0 = e.cury;
    engine_move(&e, ACTION_DOWN);
    CHECK(e.cury == y0 + 1,      "ACTION_DOWN increments cury");
    CHECK(e.status.moves == 1,   "ACTION_DOWN increments moves counter");
}

static void test_move_left_bounded(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    for (int i = 0; i < 20; i++) engine_move(&e, ACTION_LEFT);
    CHECK(e.curx >= 1, "repeated ACTION_LEFT stops at left wall");
}

static void test_move_right_bounded(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    for (int i = 0; i < 20; i++) engine_move(&e, ACTION_RIGHT);
    CHECK(e.curx <= NUMCOLS - 3, "repeated ACTION_RIGHT stops at right wall");
}

/* shadow */

static void test_shadow_y_ge_piece_y(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = TRUE;
    engine_move(&e, ACTION_DOWN);   /* first move places shadow correctly */
    CHECK(e.cury_shadow >= e.cury, "shadow y is always >= piece y");
}

static void test_shadow_x_follows_piece(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = TRUE;
    engine_move(&e, ACTION_DOWN);   /* initialise shadow */
    engine_move(&e, ACTION_LEFT);
    CHECK(e.curx_shadow == e.curx, "shadow x matches piece x after left move");
    engine_move(&e, ACTION_RIGHT);
    CHECK(e.curx_shadow == e.curx, "shadow x matches piece x after right move");
}

static void test_drop_teleports_to_shadow(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = TRUE;
    engine_move(&e, ACTION_DOWN);   /* initialise shadow */
    int shadow_y = e.cury_shadow;
    engine_move(&e, ACTION_DROP);
    CHECK(e.cury == shadow_y, "ACTION_DROP moves piece to shadow y position");
}

/* line clearing */

static void test_line_clear(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    /* pre-fill the bottom playfield row (row NUMROWS-3 = 20) */
    for (int x = 1; x <= NUMCOLS - 3; x++)
        e.board[x][NUMROWS - 3] = FILL_COLOR;
    int result, ticks = 0;
    do { result = engine_evaluate(&e); } while (result == 1 && ++ticks < 200);
    CHECK(result == 0,               "piece lands on top of filled row");
    CHECK(e.status.droppedlines >= 1, "filled row was cleared (droppedlines >= 1)");
}

/* 7-bag randomiser */

static void test_7bag_completeness(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    int seen[NUMSHAPES];
    memset(seen, 0, sizeof(seen));
    seen[e.curshape] = 1;
    for (int i = 1; i < NUMSHAPES; i++) {
        engine_move(&e, ACTION_DROP);
        if (engine_evaluate(&e) == -1) break;
        seen[e.curshape] = 1;
    }
    int all = 1;
    for (int i = 0; i < NUMSHAPES; i++) if (!seen[i]) { all = 0; break; }
    CHECK(all, "all 7 shape types appear in the first bag");
}

/* game over */

static void test_game_over(void)
{
    engine_t e;
    engine_init(&e, no_score);
    e.shadow = FALSE;
    int result = 0;
    for (int i = 0; i < 500 && result != -1; i++) {
        engine_move(&e, ACTION_DROP);
        result = engine_evaluate(&e);
    }
    CHECK(result == -1, "game ends with -1 when board fills up");
}

/* str2int */

static void test_str2int(void)
{
    int v;
    CHECK(str2int(&v, "42")  == TRUE  && v == 42, "str2int parses positive integer");
    CHECK(str2int(&v, "-5")  == TRUE  && v == -5, "str2int parses negative integer");
    CHECK(str2int(&v, "0")   == TRUE  && v ==  0, "str2int parses zero");
    CHECK(str2int(&v, "abc") == FALSE,             "str2int rejects non-numeric string");
    CHECK(str2int(&v, "")    == FALSE,             "str2int rejects empty string");
}

/* main */

int main(void)
{
    rand_init();
    printf("=== tint engine tests ===\n");
    test_board_walls();
    test_initial_state();
    test_interior_cell_clear();
    test_evaluate_falls();
    test_evaluate_lands();
    test_move_down();
    test_move_left_bounded();
    test_move_right_bounded();
    test_shadow_y_ge_piece_y();
    test_shadow_x_follows_piece();
    test_drop_teleports_to_shadow();
    test_line_clear();
    test_7bag_completeness();
    test_game_over();
    test_str2int();
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
