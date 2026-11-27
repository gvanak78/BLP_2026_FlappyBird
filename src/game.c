#include "game.h"

#include <stdlib.h>


// num-ot egy alsó és felső határ közé szorítja
static int clamp_int(int num, int lo, int hi)
{
    if (num < lo) {
        return lo;
    }
    if (num > hi) {
        return hi;
    }
    return num;
}




// A pálya méreteit biztonságos keretek közé szorítja
// Hibakezelés ha irreális értékeket adunk a konfigba
static void sanitize_dimensions(Game *g)
{
    if (g->play_width < 20) {
        g->play_width = 20;
    }
    if (g->play_height < 10) {
        g->play_height = 10;
    }

    if (g->cfg.pipe_width < 1) {
        g->cfg.pipe_width = 1;
    }
    if (g->cfg.pipe_spacing < g->cfg.pipe_width + 4) {
        g->cfg.pipe_spacing = g->cfg.pipe_width + 4;
    }
    if (g->cfg.pipe_gap < 3) {
        g->cfg.pipe_gap = 3;
    }
    if (g->cfg.pipe_gap > g->play_height - 4) {
        g->cfg.pipe_gap = g->play_height - 4;
    }

    g->cfg.bird_x = clamp_int(g->cfg.bird_x, 2, g->play_width / 2);
}




// Kiszámolja a következő csőpár szabad résének a felső Y koordinátáját
static int generate_gap_top(const Game *g)
{
    // margin_top, margin_bottom: biztonságos sávok
    int margin_top = 2;
    int margin_bottom = 2;
    int max_gap_top = g->play_height - margin_bottom - g->cfg.pipe_gap;

    if (max_gap_top <= margin_top) {
        return margin_top;
    }

    int span = max_gap_top - margin_top + 1;
    return margin_top + (rand() % span);
}



// A legjobboldalibb csőnek megnézzük az x koordinátáját
static int rightmost_pipe_x(const Game *g)
{
    int max_x = g->play_width;
    for (int i = 0; i < g->pipe_count; i++) {
        if (g->pipes[i].active && g->pipes[i].x > max_x) {
            max_x = g->pipes[i].x;
        }
    }
    return max_x;
}



// Új cső generálása a megadott pozíción
static void spawn_pipe_at(Game *g, Pipe *p, int start_x)
{
    p->active = true;
    p->x = start_x;
    p->gap_top = generate_gap_top(g);
    p->gap_bottom = p->gap_top + g->cfg.pipe_gap;
    p->scored = false;
}




// Mindent resetel
static void reset_world(Game *g, GameState next_state)
{
    sanitize_dimensions(g);


    // A játék a következő állapotba lép
    // Madár középre kerül
    g->state = next_state;
    g->bird.y = (double)(g->play_height / 2);
    g->bird.vy = 0.0;
    g->score = 0;

    // Flagek beállítása, hogy a Gecko lássa a változást
    g->score_changed = true;
    g->state_changed = true;


    // Csövek nullázása
    for (int i = 0; i < MAX_PIPES; ++i) {
        g->pipes[i].active = false;
        g->pipes[i].x = 0;
        g->pipes[i].gap_top = 0;
        g->pipes[i].gap_bottom = 0;
        g->pipes[i].scored = false;
    }

    // Kiszámolja hány csőre van szükség
    // "+3": tartalék, hogy a játék széle mögött is legyen már cső legenerálva
    int needed = (g->play_width / g->cfg.pipe_spacing) + 3;
    if (needed > MAX_PIPES) {
        needed = MAX_PIPES;
    }
    g->pipe_count = needed;

    // Az első cső a "play_width + 4" oszlopból indul
    int first_x = g->play_width + 4;
    if (first_x < g->cfg.bird_x + 8) {
        first_x = g->cfg.bird_x + 8;
    }

    // Csövek legenerálása
    // "first_x + i * pipe_spacing" re sorakoznak a csövek egymástól
    for (int i = 0; i < g->pipe_count; i++) {
        spawn_pipe_at(g, &g->pipes[i], first_x + i * g->cfg.pipe_spacing);
    }
}




// Gameover
static void set_game_over(Game *g)
{
    if (g->state != STATE_GAME_OVER) {
        g->state = STATE_GAME_OVER;
        g->state_changed = true;
        if (g->score > g->best_score) {
            g->best_score = g->score;
        }
    }
}





// Ütközésvizsgálat a madár "x" és "y" koordinátáji alapján
static bool check_collision(const Game *g)
{

    // double-t kasztolunk intté, a kerekítés miatt "0.0 ? 0.5 : -0.5"
    int by = (int)(g->bird.y + (g->bird.y >= 0.0 ? 0.5 : -0.5));
    int bx = g->cfg.bird_x;

    // A madár a földre esett
    if (by >= g->play_height - 1) {
        return true;
    }

    // A madár kirepült a játéktérből
    if (by < 0) {
        return true;
    }

    // Csővel való ütközés
    for (int i = 0; i < g->pipe_count; i++) {
        const Pipe *p = &g->pipes[i];
        if (!p->active) {
            continue;
        }

        if (bx >= p->x && bx < (p->x + g->cfg.pipe_width)) {
            if (by < p->gap_top || by >= p->gap_bottom) {
                return true;
            }
        }
    }

    return false;
}




// Egy alap játék konfiguráció
GameConfig game_default_config(void)
{
    GameConfig cfg = {
        .tick_ms = 65,
        .gravity = 0.22,
        .flap_vy = -1.55,
        .pipe_gap = 7,
        .pipe_spacing = 22,
        .pipe_width = 4,
        .bird_x = 10
    };
    return cfg;
}






void game_init(Game *g, int play_width, int play_height, const GameConfig *cfg, int best_score)
{
    if (!g) {
        return;
    }

    g->play_width = play_width;
    g->play_height = play_height;
    g->cfg = cfg ? *cfg : game_default_config();
    g->best_score = best_score > 0 ? best_score : 0;

    reset_world(g, STATE_MENU);
}




// Indítás/ ugrás eseménykezelője
void game_start_or_flap(Game *g)
{
    if (!g) {
        return;
    }

    if (g->state == STATE_MENU) {
        reset_world(g, STATE_PLAYING);
        g->bird.vy = g->cfg.flap_vy;

    } else if (g->state == STATE_PLAYING) {
        g->bird.vy = g->cfg.flap_vy;

    } else if (g->state == STATE_GAME_OVER) {
        reset_world(g, STATE_PLAYING);
        g->bird.vy = g->cfg.flap_vy;
    }
}





void game_restart(Game *g)
{
    if (!g) {
        return;
    }
    reset_world(g, STATE_MENU);
}



// Maga a játék előre léptetése
// Minden ticknél egyszer fut le
void game_step(Game *g)
{
    // Ha rossz pointert adtunk át, vagy nem vagyunk játékban return
    if (!g || g->state != STATE_PLAYING) {
        return;
    }

    // Függőleges sebesség nő a gravitációval
    g->bird.vy += g->cfg.gravity;
    g->bird.y += g->bird.vy;

    // Csövek mozgatása és újragenerálása
    // Ez a for ciklus átláthatóság miatt van
    for (int i = 0; i < g->pipe_count; i++) {
        Pipe *p = &g->pipes[i];
        if (!p->active) {
            continue;
        }

        // Csövek eltolása balra
        p->x -= 1;

        // Pont jár, ha a madár "x" pozíciója túlhaladt a cső jobb széle mellett ÉS
        // az aktuális csőre még nem számoltuk be a pontot
        if (!p->scored && (p->x + g->cfg.pipe_width - 1) < g->cfg.bird_x) {
            p->scored = true;
            g->score += 1;
            g->score_changed = true;

            // A legtöbb pontot frissítjük ha kell
            if (g->score > g->best_score) {
                g->best_score = g->score;
            }
        }

        // Ha teljesen kiment egy cső képernyő bal szélére, 
        // megnézzük a legjobboldali cső pozícióját, és újat generálunk mellé
        if ((p->x + g->cfg.pipe_width) <= 0) {
            int next_x = rightmost_pipe_x(g) + g->cfg.pipe_spacing;
            spawn_pipe_at(g, p, next_x);
        }
    }

    // Ütközés -> gameover
    if (check_collision(g)) {
        g->bird.y = (double)(g->play_height - 1);
        set_game_over(g);
    }
}
