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

    game.playerScore    = 0;
    game.cpuScore       = 0;
    game.elapsedSeconds = 0;
    game.matchStartTime = 0;
    game.state          = STATE_MENU;
    game.running        = true;
    game.difficulty     = DIFF_MEDIUM;
    game.resultMessage  = "";

    player.rect       = { 30, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    player.speedY     = PLAYER_SPEED;
    player.movingUp   = false;
    player.movingDown = false;

    cpu.rect       = { WINDOW_WIDTH - 30 - PADDLE_W, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    cpu.speedY     = 0;
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

    cpu.rect = { WINDOW_WIDTH - 30 - PADDLE_W, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };

    resetBall(ball, 1);
}

void resetBall(Ball& ball, int direction) {
    ball.posX = (float)(WINDOW_WIDTH  / 2 - BALL_SIZE / 2);
    ball.posY = (float)(WINDOW_HEIGHT / 2 - BALL_SIZE / 2);
    ball.rect = { (int)ball.posX, (int)ball.posY, BALL_SIZE, BALL_SIZE };

    // Angulo aleatorio entre -30 y 30 grados
    float angle = (float)((rand() % 60) - 30) * 3.14159f / 180.0f;
    ball.velX   = direction * BALL_SPEED_INITIAL * cosf(angle);
    ball.velY   = BALL_SPEED_INITIAL * sinf(angle);

    // Velocidad Y minima para evitar pelota perfectamente horizontal
    if (fabsf(ball.velY) < 1.5f) {
        ball.velY = (ball.velY >= 0.0f) ? 1.5f : -1.5f;
    }
}

// ================================================
// LOGICA DE IA
// ================================================

static void moveCPU(Paddle& cpu, const Ball& ball, Difficulty diff) {
    int cpuSpeed;
    switch (diff) {
        case DIFF_EASY:   cpuSpeed = 3; break;
        case DIFF_MEDIUM: cpuSpeed = 5; break;
        case DIFF_HARD:   cpuSpeed = 8; break;
        default:          cpuSpeed = 5; break;
    }

    int ballCenter = ball.rect.y + ball.rect.h / 2;
    int cpuCenter  = cpu.rect.y  + cpu.rect.h  / 2;
    const int DEADZONE = 5;

    if      (ballCenter < cpuCenter - DEADZONE) cpu.rect.y -= cpuSpeed;
    else if (ballCenter > cpuCenter + DEADZONE) cpu.rect.y += cpuSpeed;

    // Mantener la paleta dentro de la pantalla
    if (cpu.rect.y < 0)                              cpu.rect.y = 0;
    if (cpu.rect.y + cpu.rect.h > WINDOW_HEIGHT)    cpu.rect.y = WINDOW_HEIGHT - cpu.rect.h;
}

// ================================================
// VERIFICACION DE CONDICION DE FIN
// ================================================

static void checkEndCondition(GameData& game) {
    bool gameOver = false;

    if (game.playerScore >= MAX_SCORE) {
        game.resultMessage = "GANASTE!";
        gameOver = true;
    } else if (game.cpuScore >= MAX_SCORE) {
        game.resultMessage = "PERDISTE";
        gameOver = true;
    } else if (game.elapsedSeconds >= MATCH_DURATION) {
        if      (game.playerScore > game.cpuScore)  game.resultMessage = "GANASTE! (tiempo)";
        else if (game.cpuScore > game.playerScore)  game.resultMessage = "PERDISTE (tiempo)";
        else                                         game.resultMessage = "EMPATE!";
        gameOver = true;
    }

    if (gameOver) {
        saveResult(game);
        game.state = STATE_RESULT;
    }
}

// ================================================
// MANEJO DE INPUT
// ================================================

void handleInput(SDL_Event& e, GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    switch (game.state) {

        case STATE_MENU:
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN ||
                    e.key.keysym.sym == SDLK_SPACE) {
                    game.state = STATE_DIFFICULTY;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    game.running = false;
                }
            }
            break;

        case STATE_DIFFICULTY:
            if (e.type == SDL_KEYDOWN) {
                bool chosen = true;
                switch (e.key.keysym.sym) {
                    case SDLK_1: game.difficulty = DIFF_EASY;   break;
                    case SDLK_2: game.difficulty = DIFF_MEDIUM; break;
                    case SDLK_3: game.difficulty = DIFF_HARD;   break;
                    default:     chosen = false;                 break;
                }
                if (chosen) {
                    game.state = STATE_GAMEPLAY;
                    startNewGame(game, player, cpu, ball);
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    game.state = STATE_MENU;
                }
            }
            break;

        case STATE_GAMEPLAY:
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP)
                    player.movingUp = true;
                if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN)
                    player.movingDown = true;
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    game.state = STATE_MENU;
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_UP)
                    player.movingUp = false;
                if (e.key.keysym.sym == SDLK_s || e.key.keysym.sym == SDLK_DOWN)
                    player.movingDown = false;
            }
            break;

        case STATE_RESULT:
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN ||
                    e.key.keysym.sym == SDLK_SPACE) {
                    game.state = STATE_MENU;
                }
            }
            break;
    }
}

// ================================================
// UPDATE (LOGICA DE GAMEPLAY)
// ================================================

void updateGameplay(GameData& game, Paddle& player, Paddle& cpu, Ball& ball, SDLContext& sdl) {
    // Actualizar tiempo transcurrido
    game.elapsedSeconds = (int)((SDL_GetTicks() - game.matchStartTime) / 1000);

    // ------ Mover paleta del jugador ------
    if (player.movingUp)   player.rect.y -= player.speedY;
    if (player.movingDown) player.rect.y += player.speedY;

    if (player.rect.y < 0)                              player.rect.y = 0;
    if (player.rect.y + player.rect.h > WINDOW_HEIGHT)  player.rect.y = WINDOW_HEIGHT - player.rect.h;

    // ------ Mover paleta de la CPU (IA) ------
    moveCPU(cpu, ball, game.difficulty);

    // ------ Mover pelota ------
    ball.posX += ball.velX;
    ball.posY += ball.velY;
    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

    // ------ Rebote en paredes superior e inferior ------
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

    // ------ Colision con paleta del jugador ------
    if (SDL_HasIntersection(&ball.rect, &player.rect)) {
        ball.posX = (float)(player.rect.x + player.rect.w);
        ball.velX = fabsf(ball.velX);  // rebota hacia la derecha

        // Angulo de rebote segun punto de impacto en la paleta
        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) -
                        (player.rect.y + player.rect.h / 2.0f)) /
                        (player.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;

        // Aceleracion progresiva
        if (ball.velX < BALL_SPEED_MAX) ball.velX *= 1.05f;

        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    // ------ Colision con paleta de la CPU ------
    if (SDL_HasIntersection(&ball.rect, &cpu.rect)) {
        ball.posX = (float)(cpu.rect.x - ball.rect.w);
        ball.velX = -fabsf(ball.velX);  // rebota hacia la izquierda

        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) -
                        (cpu.rect.y + cpu.rect.h / 2.0f)) /
                        (cpu.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;

        if (fabsf(ball.velX) < BALL_SPEED_MAX) ball.velX *= 1.05f;

        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    // ------ Pelota sale por la izquierda: CPU anota ------
    if (ball.rect.x + ball.rect.w < 0) {
        game.cpuScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, -1);  // lanza hacia el jugador (quien perdio el punto)
    }

    // ------ Pelota sale por la derecha: Jugador anota ------
    if (ball.rect.x > WINDOW_WIDTH) {
        game.playerScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, 1);   // lanza hacia la CPU (quien perdio el punto)
    }

    // Actualizar rect desde float
    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

    checkEndCondition(game);
}
