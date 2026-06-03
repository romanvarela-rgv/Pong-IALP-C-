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
const int   MAX_SCORE      = 7;
const int   MATCH_DURATION = 120;
const int   TARGET_FPS     = 60;
const int   FRAME_DELAY    = 1000 / TARGET_FPS;

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
// CANTIDAD DE PERSONAJES
// ================================================
const int NUM_CHARACTERS = 4;

// ================================================
// ESTADOS DEL JUEGO
// ================================================
enum GameState {
    STATE_MENU,          // Pantalla principal
    STATE_MODE_SELECT,   // Seleccion de modo (PvP / PvI)
    STATE_CHAR_SELECT,   // Seleccion de personaje/escenario
    STATE_GAMEPLAY,      // Partida en juego
    STATE_RESULT         // Pantalla de resultado
};

// ================================================
// DIFICULTAD (IA)
// ================================================
enum Difficulty {
    DIFF_EASY   = 0,
    DIFF_MEDIUM = 1,
    DIFF_HARD   = 2
};

// ================================================
// MODO DE JUEGO
// ================================================
enum GameMode {
    MODE_PVI = 0,   // Player vs IA
    MODE_PVP = 1    // Player vs Player
};

// ================================================
// PERSONAJES / ESCENARIOS
// ================================================
enum Character {
    CHAR_FREDDY      = 0,
    CHAR_JASON       = 1,
    CHAR_GHOSTFACE   = 2,
    CHAR_LEATHERFACE = 3
};

// ================================================
// STRUCTS
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
    float    posX;
    float    posY;
};

struct GameData {
    int         playerScore;
    int         cpuScore;
    Uint32      matchStartTime;
    int         elapsedSeconds;
    GameState   state;
    Difficulty  difficulty;
    GameMode    mode;
    Character   selectedChar;
    int         menuSelection;   // item activo en el menu actual
    bool        running;
    std::string resultMessage;
};

struct SDLContext {
    SDL_Window*   window;
    SDL_Renderer* renderer;

    // Sprites de entidades
    SDL_Texture*  paddleTex;
    SDL_Texture*  ballTex;

    // Fuentes
    TTF_Font*     fontLarge;
    TTF_Font*     fontMedium;
    TTF_Font*     fontSmall;

    // Audio
    Mix_Music*    bgMusic;
    Mix_Chunk*    hitSound;
    Mix_Chunk*    goalSound;

    // Fondos de pantallas de menu
    SDL_Texture*  bgMenu;
    SDL_Texture*  bgModeSelect;
    SDL_Texture*  bgCharSelect;

    // Fondos de gameplay (uno por personaje)
    SDL_Texture*  bgStage[NUM_CHARACTERS];

    // Botones
    SDL_Texture*  btnPlay;
    SDL_Texture*  btnOptions;
    SDL_Texture*  btnQuit;
    SDL_Texture*  btnPvP;
    SDL_Texture*  btnPvI;
    SDL_Texture*  btnBack;
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
void renderText(SDLContext& sdl, const std::string& text, TTF_Font* font,
                SDL_Color color, int x, int y, bool centered = false);
void renderTextRight(SDLContext& sdl, const std::string& text, TTF_Font* font,
                     SDL_Color color, int rightX, int y);
void renderFullBg(SDLContext& sdl, SDL_Texture* bg);
void renderButton(SDLContext& sdl, SDL_Texture* btn, int centerX, int centerY,
                  int targetW, bool highlighted = false);
void renderMenu(SDLContext& sdl, const GameData& game);
void renderModeSelect(SDLContext& sdl, const GameData& game);
void renderCharSelect(SDLContext& sdl, const GameData& game);
void renderGameplay(SDLContext& sdl, const Paddle& player, const Paddle& cpu,
                    const Ball& ball, const GameData& game);
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
