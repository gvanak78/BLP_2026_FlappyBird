#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_PIPES 32




typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;




typedef struct {
    int tick_ms;        /* Játék sebessége: egy tick hossza ms-ban (pl. 60) */
    double gravity;     /* Gravitációs gyorsulás (sor / tick^2, pl. 0.25) */
    double flap_vy;     /* Ugrás utáni felfelé sebesség (sor / tick, pl. -1.6) */
    int pipe_gap;       /* Szabad rés magassága a felső és alsó cső között */
    int pipe_spacing;   /* Vízszintes távolság két csőoszlop bal széle között */
    int pipe_width;     /* Egy csőoszlop szélessége oszlopokban (pl. 4) */
    int bird_x;         /* Madár vízszintes pozíciója a játéktéren belül */
} GameConfig;




typedef struct {
    bool active;        /* Adott tömbelem résztvesz-e a játékban */
    int x;              /* Cső bal szélének oszlopkoordinátája */
    int gap_top;        /* Szabad rés felső sorindexe */
    int gap_bottom;     /* Szabad rés alsó sorindexe (kizárólagos felső határ) */
    bool scored;        /* Igaz, ha a játékos már áthaladt a csövön */
} Pipe;





typedef struct {
    double y;           /* Madár aktuális függőleges pozíciója */
    double vy;          /* Madár függőleges sebessége */
} Bird;





typedef struct {
    GameConfig cfg;
    GameState state;
    int play_width;
    int play_height;

    Bird bird;
    Pipe pipes[MAX_PIPES];  /* Tömbként adjuk át */
    int pipe_count;

    int score;
    int best_score;
    bool score_changed; /* Igaz, ha új pontot kell küldeni az STK3700-nak */
    bool state_changed; /* Igaz, ha új állapotot kell küldeni az STK3700-nak */
} Game;



GameConfig game_default_config(void);
void game_init(Game *g, int play_width, int play_height, const GameConfig *cfg, int best_score);
void game_start_or_flap(Game *g);
void game_restart(Game *g);
void game_step(Game *g);




#endif /* GAME_H */



/*
    A play_width, play_height a linux termináljának méretétől függő változók

    A main.c-ben szükséges függvények:
        game_init(&game, play_w, play_h, &cfg, best_score)
        game_start_or_flap(&game)
        game_restart(&game)
        game_step(&game)
    
*/
