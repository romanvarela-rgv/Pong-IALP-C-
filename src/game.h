#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <string>

// ================================================
// CONSTANTES DE VENTANA
// ================================================
const int WINDOW_WIDTH  = 1280;
const int WINDOW_HEIGHT = 720;

// ================================================
// REGLAS DE JUEGO
// ================================================
const int   MAX_SCORE        = 7;
const int   MATCH_DURATION   = 120;   // segundos
const int   TARGET_FPS       = 60;
const int   FRAME_DELAY      = 1000 / TARGET_FPS;

// ================================================
// DIMENSIONES DE ENTIDADES
// ================================================
const int PADDLE_W  = 15;
const int PADDLE_H  = 100;
const int BALL_SIZE = 15;

// ================================================
// VELOCIDADES
// ================================================
const int   PLAYER_SPEED       = 7;
const float BALL_SPEED_INITIAL = 5.0f;
const float BALL_SPEED_MAX     = 14.0f;

// ================================================
// ESTADOS DEL JUEGO
// ================================================
enum GameState {
    STATE_MENU,
    STATE_DIFFICULTY,
    STATE_GAMEPLAY,
    STATE_RESULT
};

// ================================================
// DIFICULTAD
// ================================================
enum Difficulty {
    DIFF_EASY   = 0,
    DIFF_MEDIUM = 1,
    DIFF_HARD   = 2
};

// ================================================
// ESTRUCTURAS
// ================================================

struct Paddle {
    SDL_Rect rect;
    int      speedY;
    bool     movingUp;
    bool     movingDown;
};

struct Ball {
    SDL_Rect rect;
    float    velX;
    float    velY;
    float    posX;   // posicion en float para movimiento suave
    float    posY;
};

struct GameData {
    int         playerScore;
    int         cpuScore;
    Uint32      matchStartTime;
    int         elapsedSeconds;
    GameState   state;
    Difficulty  difficulty;
    bool        running;
    std::string resultMessage;
};

struct SDLContext {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  paddleTex;
    SDL_Texture*  ballTex;
    TTF_Font*     fontLarge;
    TTF_Font*     fontMedium;
    TTF_Font*     fontSmall;
    Mix_Music*    bgMusic;
    Mix_Chunk*    hitSound;
    Mix_Chunk*    goalSound;
};

// ================================================
// DECLARACIONES - game.cpp
// ================================================
void initGame(GameData& game, Paddle& player, Paddle& cpu, Ball& ball);
void startNewGame(GameData& game, Paddle& player, Paddle& cpu, Ball& ball);
void resetBall(Ball& ball, int direction);
void handleInput(SDL_Event& e, GameData& game, Paddle& player, Paddle& cpu, Ball& ball);
void updateGameplay(GameData& game, Paddle& player, Paddle& cpu, Ball& ball, SDLContext& sdl);

// ================================================
// DECLARACIONES - render.cpp
// ================================================
void renderText(SDLContext& sdl, const std::string& text, TTF_Font* font, SDL_Color color, int x, int y, bool centered = false);
void renderMenu(SDLContext& sdl);
void renderDifficulty(SDLContext& sdl);
void renderGameplay(SDLContext& sdl, const Paddle& player, const Paddle& cpu, const Ball& ball, const GameData& game);
void renderResult(SDLContext& sdl, const GameData& game);

// ================================================
// DECLARACIONES - audio.cpp
// ================================================
bool initAudio(SDLContext& sdl);
void cleanupAudio(SDLContext& sdl);

// ================================================
// DECLARACIONES - csv_manager.cpp
// ================================================
void saveResult(const GameData& game);
