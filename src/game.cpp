#include "game.h"
#include "csv_manager.h"
#include <cstdlib>
#include <cmath>
#include <ctime>

// ================================================
// INICIALIZACION
// ================================================

void initGame(GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    srand((unsigned int)time(nullptr));

    game.playerScore   = 0;
    game.cpuScore      = 0;
    game.elapsedSeconds = 0;
    game.matchStartTime = 0;
    game.state         = STATE_MENU;
    game.running       = true;
    game.difficulty    = DIFF_MEDIUM;
    game.mode          = MODE_PVI;
    game.selectedChar  = CHAR_FREDDY;
    game.menuSelection = 0;
    game.resultMessage = "";

    player.rect       = { 30, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    player.speedY     = PLAYER_SPEED;
    player.movingUp   = false;
    player.movingDown = false;

    cpu.rect       = { WINDOW_WIDTH - 30 - PADDLE_W, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    cpu.speedY     = PLAYER_SPEED;
    cpu.movingUp   = false;
    cpu.movingDown = false;

    resetBall(ball, 1);
}

void startNewGame(GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    game.playerScore    = 0;
    game.cpuScore       = 0;
    game.elapsedSeconds = 0;
    game.matchStartTime = SDL_GetTicks();

    player.rect       = { 30, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    player.movingUp   = false;
    player.movingDown = false;

    cpu.rect      = { WINDOW_WIDTH - 30 - PADDLE_W, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    cpu.movingUp  = false;
    cpu.movingDown = false;

    resetBall(ball, 1);
}

void resetBall(Ball& ball, int direction) {
    ball.posX = (float)(WINDOW_WIDTH  / 2 - BALL_SIZE / 2);
    ball.posY = (float)(WINDOW_HEIGHT / 2 - BALL_SIZE / 2);
    ball.rect = { (int)ball.posX, (int)ball.posY, BALL_SIZE, BALL_SIZE };

    float angle = (float)((rand() % 60) - 30) * 3.14159f / 180.0f;
    ball.velX   = direction * BALL_SPEED_INITIAL * cosf(angle);
    ball.velY   = BALL_SPEED_INITIAL * sinf(angle);

    if (fabsf(ball.velY) < 1.5f)
        ball.velY = (ball.velY >= 0.0f) ? 1.5f : -1.5f;
}

// ================================================
// IA DE LA CPU
// ================================================

static void moveCPU(Paddle& cpu, const Ball& ball, Difficulty diff) {
    int speed;
    switch (diff) {
        case DIFF_EASY:   speed = 3; break;
        case DIFF_MEDIUM: speed = 5; break;
        case DIFF_HARD:   speed = 8; break;
        default:          speed = 5; break;
    }

    int ballCenter = ball.rect.y + ball.rect.h / 2;
    int cpuCenter  = cpu.rect.y  + cpu.rect.h  / 2;
    const int DEADZONE = 5;

    if      (ballCenter < cpuCenter - DEADZONE) cpu.rect.y -= speed;
    else if (ballCenter > cpuCenter + DEADZONE) cpu.rect.y += speed;

    if (cpu.rect.y < 0)                             cpu.rect.y = 0;
    if (cpu.rect.y + cpu.rect.h > WINDOW_HEIGHT)   cpu.rect.y = WINDOW_HEIGHT - cpu.rect.h;
}

// ================================================
// CONDICION DE FIN DE PARTIDA
// ================================================

static void checkEndCondition(GameData& game) {
    bool over = false;

    if (game.playerScore >= MAX_SCORE) {
        game.resultMessage = "GANASTE!";
        over = true;
    } else if (game.cpuScore >= MAX_SCORE) {
        game.resultMessage = (game.mode == MODE_PVP) ? "GANO P2!" : "PERDISTE";
        over = true;
    } else if (game.elapsedSeconds >= MATCH_DURATION) {
        if      (game.playerScore > game.cpuScore)  game.resultMessage = "GANASTE! (tiempo)";
        else if (game.cpuScore > game.playerScore)  game.resultMessage = (game.mode == MODE_PVP) ? "GANO P2! (tiempo)" : "PERDISTE (tiempo)";
        else                                         game.resultMessage = "EMPATE!";
        over = true;
    }

    if (over) {
        saveResult(game);
        game.state = STATE_RESULT;
    }
}

// ================================================
// MANEJO DE INPUT
// ================================================

void handleInput(SDL_Event& e, GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    switch (game.state) {

        // ------ MENU PRINCIPAL ------
        case STATE_MENU:
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        game.menuSelection = (game.menuSelection - 1 + 3) % 3;
                        break;
                    case SDLK_DOWN:
                        game.menuSelection = (game.menuSelection + 1) % 3;
                        break;
                    case SDLK_RETURN:
                        if      (game.menuSelection == 0) { game.state = STATE_MODE_SELECT; game.menuSelection = 0; }
                        else if (game.menuSelection == 2) game.running = false;
                        break;
                    case SDLK_ESCAPE:
                        game.running = false;
                        break;
                    default: break;
                }
            }
            break;

        // ------ SELECCION DE MODO ------
        case STATE_MODE_SELECT:
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        game.menuSelection = (game.menuSelection - 1 + 3) % 3;
                        break;
                    case SDLK_DOWN:
                        game.menuSelection = (game.menuSelection + 1) % 3;
                        break;
                    case SDLK_RETURN:
                        if (game.menuSelection == 0) {
                            game.mode  = MODE_PVP;
                            game.state = STATE_CHAR_SELECT;
                            game.menuSelection = 0;
                        } else if (game.menuSelection == 1) {
                            game.mode  = MODE_PVI;
                            game.state = STATE_CHAR_SELECT;
                            game.menuSelection = 0;
                        } else {
                            game.state = STATE_MENU;
                            game.menuSelection = 0;
                        }
                        break;
                    case SDLK_ESCAPE:
                        game.state = STATE_MENU;
                        game.menuSelection = 0;
                        break;
                    default: break;
                }
            }
            break;

        // ------ SELECCION DE PERSONAJE ------
        case STATE_CHAR_SELECT:
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        game.menuSelection = (game.menuSelection - 1 + NUM_CHARACTERS) % NUM_CHARACTERS;
                        break;
                    case SDLK_RIGHT:
                        game.menuSelection = (game.menuSelection + 1) % NUM_CHARACTERS;
                        break;
                    // Seleccion rapida de dificultad (solo en PvI)
                    case SDLK_1: if (game.mode == MODE_PVI) game.difficulty = DIFF_EASY;   break;
                    case SDLK_2: if (game.mode == MODE_PVI) game.difficulty = DIFF_MEDIUM; break;
                    case SDLK_3: if (game.mode == MODE_PVI) game.difficulty = DIFF_HARD;   break;
                    case SDLK_RETURN:
                        game.selectedChar = (Character)game.menuSelection;
                        game.state = STATE_GAMEPLAY;
                        startNewGame(game, player, cpu, ball);
                        break;
                    case SDLK_ESCAPE:
                        game.state = STATE_MODE_SELECT;
                        game.menuSelection = 0;
                        break;
                    default: break;
                }
            }
            break;

        // ------ GAMEPLAY ------
        case STATE_GAMEPLAY:
            if (e.type == SDL_KEYDOWN) {
                // Jugador 1: W / S
                if (e.key.keysym.sym == SDLK_w) player.movingUp   = true;
                if (e.key.keysym.sym == SDLK_s) player.movingDown = true;
                // Jugador 2 (PvP): flechas arriba/abajo
                if (game.mode == MODE_PVP) {
                    if (e.key.keysym.sym == SDLK_UP)   cpu.movingUp   = true;
                    if (e.key.keysym.sym == SDLK_DOWN)  cpu.movingDown = true;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    game.state = STATE_MENU;
                    game.menuSelection = 0;
                }
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_w) player.movingUp   = false;
                if (e.key.keysym.sym == SDLK_s) player.movingDown = false;
                if (game.mode == MODE_PVP) {
                    if (e.key.keysym.sym == SDLK_UP)   cpu.movingUp   = false;
                    if (e.key.keysym.sym == SDLK_DOWN)  cpu.movingDown = false;
                }
            }
            break;

        // ------ RESULTADO ------
        case STATE_RESULT:
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
                    game.state = STATE_MENU;
                    game.menuSelection = 0;
                }
            }
            break;
    }
}

// ================================================
// UPDATE
// ================================================

void updateGameplay(GameData& game, Paddle& player, Paddle& cpu, Ball& ball, SDLContext& sdl) {
    game.elapsedSeconds = (int)((SDL_GetTicks() - game.matchStartTime) / 1000);

    // ------ Paleta del jugador 1 ------
    if (player.movingUp)   player.rect.y -= player.speedY;
    if (player.movingDown) player.rect.y += player.speedY;
    if (player.rect.y < 0)                             player.rect.y = 0;
    if (player.rect.y + player.rect.h > WINDOW_HEIGHT) player.rect.y = WINDOW_HEIGHT - player.rect.h;

    // ------ Paleta derecha: IA o Jugador 2 ------
    if (game.mode == MODE_PVI) {
        moveCPU(cpu, ball, game.difficulty);
    } else {
        if (cpu.movingUp)   cpu.rect.y -= cpu.speedY;
        if (cpu.movingDown) cpu.rect.y += cpu.speedY;
        if (cpu.rect.y < 0)                             cpu.rect.y = 0;
        if (cpu.rect.y + cpu.rect.h > WINDOW_HEIGHT)   cpu.rect.y = WINDOW_HEIGHT - cpu.rect.h;
    }

    // ------ Pelota ------
    ball.posX += ball.velX;
    ball.posY += ball.velY;
    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

    // Rebote arriba/abajo
    if (ball.rect.y <= 0) {
        ball.posY = 0.0f;
        ball.velY = fabsf(ball.velY);
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }
    if (ball.rect.y + ball.rect.h >= WINDOW_HEIGHT) {
        ball.posY = (float)(WINDOW_HEIGHT - ball.rect.h);
        ball.velY = -fabsf(ball.velY);
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    // Colision paleta jugador
    if (SDL_HasIntersection(&ball.rect, &player.rect)) {
        ball.posX = (float)(player.rect.x + player.rect.w);
        ball.velX = fabsf(ball.velX);
        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) - (player.rect.y + player.rect.h / 2.0f)) / (player.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;
        if (ball.velX < BALL_SPEED_MAX) ball.velX *= 1.05f;
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    // Colision paleta CPU/P2
    if (SDL_HasIntersection(&ball.rect, &cpu.rect)) {
        ball.posX = (float)(cpu.rect.x - ball.rect.w);
        ball.velX = -fabsf(ball.velX);
        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) - (cpu.rect.y + cpu.rect.h / 2.0f)) / (cpu.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;
        if (fabsf(ball.velX) < BALL_SPEED_MAX) ball.velX *= 1.05f;
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    // Gol CPU / P2
    if (ball.rect.x + ball.rect.w < 0) {
        game.cpuScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, -1);
    }

    // Gol Jugador 1
    if (ball.rect.x > WINDOW_WIDTH) {
        game.playerScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, 1);
    }

    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

    checkEndCondition(game);
}
