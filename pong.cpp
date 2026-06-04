#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <ctime>

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
    STATE_MENU,
    STATE_MODE_SELECT,
    STATE_CHAR_SELECT,
    STATE_GAMEPLAY,
    STATE_RESULT
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
    MODE_PVI = 0,
    MODE_PVP = 1
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
    int         menuSelection;
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

    SDL_Texture*  bgMenu;
    SDL_Texture*  bgModeSelect;
    SDL_Texture*  bgCharSelect;

    SDL_Texture*  bgStage[NUM_CHARACTERS];

    SDL_Texture*  btnPlay;
    SDL_Texture*  btnOptions;
    SDL_Texture*  btnQuit;
    SDL_Texture*  btnPvP;
    SDL_Texture*  btnPvI;
    SDL_Texture*  btnBack;
};

// ================================================
// AUDIO
// ================================================

bool initAudio(SDLContext& sdl) {
    sdl.bgMusic   = nullptr;
    sdl.hitSound  = nullptr;
    sdl.goalSound = nullptr;

    if (Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3) == 0) return false;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        Mix_Quit();
        return false;
    }

    sdl.bgMusic = Mix_LoadMUS("assets/sounds/music.ogg");
    if (!sdl.bgMusic) sdl.bgMusic = Mix_LoadMUS("assets/sounds/music.mp3");

    sdl.hitSound  = Mix_LoadWAV("assets/sounds/hit.wav");
    sdl.goalSound = Mix_LoadWAV("assets/sounds/goal.wav");

    if (sdl.bgMusic) {
        Mix_VolumeMusic(48);
        Mix_PlayMusic(sdl.bgMusic, -1);
    }

    return true;
}

void cleanupAudio(SDLContext& sdl) {
    if (sdl.hitSound)  { Mix_FreeChunk(sdl.hitSound);  sdl.hitSound  = nullptr; }
    if (sdl.goalSound) { Mix_FreeChunk(sdl.goalSound); sdl.goalSound = nullptr; }
    if (sdl.bgMusic)   { Mix_FreeMusic(sdl.bgMusic);   sdl.bgMusic   = nullptr; }
    Mix_CloseAudio();
    Mix_Quit();
}

// ================================================
// CSV - GUARDADO DE RESULTADOS
// ================================================

static std::string getDateTime() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", t);
    return std::string(buf);
}

static const char* getDifficultyName(Difficulty d) {
    switch (d) {
        case DIFF_EASY:   return "Facil";
        case DIFF_MEDIUM: return "Medio";
        case DIFF_HARD:   return "Dificil";
        default:          return "?";
    }
}

static const char* getWinner(const GameData& game) {
    if (game.playerScore > game.cpuScore) return "Jugador";
    if (game.cpuScore > game.playerScore) return "CPU";
    return "Empate";
}

void saveResult(const GameData& game) {
    const char* filename = "resultados.csv";

    bool exists = false;
    {
        std::ifstream check(filename);
        exists = check.is_open();
    }

    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    if (!exists)
        file << "Fecha,Jugador,CPU,Duracion(s),Ganador,Dificultad\n";

    file << getDateTime()                      << ","
         << game.playerScore                   << ","
         << game.cpuScore                      << ","
         << game.elapsedSeconds                << ","
         << getWinner(game)                    << ","
         << getDifficultyName(game.difficulty) << "\n";

    file.close();
}

// ================================================
// LOGICA DEL JUEGO
// ================================================

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

void initGame(GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    srand((unsigned int)time(nullptr));

    game.playerScore    = 0;
    game.cpuScore       = 0;
    game.elapsedSeconds = 0;
    game.matchStartTime = 0;
    game.state          = STATE_MENU;
    game.running        = true;
    game.difficulty     = DIFF_MEDIUM;
    game.mode           = MODE_PVI;
    game.selectedChar   = CHAR_FREDDY;
    game.menuSelection  = 0;
    game.resultMessage  = "";

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

    cpu.rect       = { WINDOW_WIDTH - 30 - PADDLE_W, (WINDOW_HEIGHT - PADDLE_H) / 2, PADDLE_W, PADDLE_H };
    cpu.movingUp   = false;
    cpu.movingDown = false;

    resetBall(ball, 1);
}

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

    if (cpu.rect.y < 0)                           cpu.rect.y = 0;
    if (cpu.rect.y + cpu.rect.h > WINDOW_HEIGHT)  cpu.rect.y = WINDOW_HEIGHT - cpu.rect.h;
}

static void checkEndCondition(GameData& game) {
    bool over = false;

    if (game.playerScore >= MAX_SCORE) {
        game.resultMessage = "GANASTE!";
        over = true;
    } else if (game.cpuScore >= MAX_SCORE) {
        game.resultMessage = (game.mode == MODE_PVP) ? "GANO P2!" : "PERDISTE";
        over = true;
    } else if (game.elapsedSeconds >= MATCH_DURATION) {
        if      (game.playerScore > game.cpuScore) game.resultMessage = "GANASTE! (tiempo)";
        else if (game.cpuScore > game.playerScore) game.resultMessage = (game.mode == MODE_PVP) ? "GANO P2! (tiempo)" : "PERDISTE (tiempo)";
        else                                        game.resultMessage = "EMPATE!";
        over = true;
    }

    if (over) {
        saveResult(game);
        game.state = STATE_RESULT;
    }
}

void handleInput(SDL_Event& e, GameData& game, Paddle& player, Paddle& cpu, Ball& ball) {
    switch (game.state) {

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

        case STATE_CHAR_SELECT:
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        game.menuSelection = (game.menuSelection - 1 + NUM_CHARACTERS) % NUM_CHARACTERS;
                        break;
                    case SDLK_RIGHT:
                        game.menuSelection = (game.menuSelection + 1) % NUM_CHARACTERS;
                        break;
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

        case STATE_GAMEPLAY:
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w) player.movingUp   = true;
                if (e.key.keysym.sym == SDLK_s) player.movingDown = true;
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

void updateGameplay(GameData& game, Paddle& player, Paddle& cpu, Ball& ball, SDLContext& sdl) {
    game.elapsedSeconds = (int)((SDL_GetTicks() - game.matchStartTime) / 1000);

    if (player.movingUp)   player.rect.y -= player.speedY;
    if (player.movingDown) player.rect.y += player.speedY;
    if (player.rect.y < 0)                              player.rect.y = 0;
    if (player.rect.y + player.rect.h > WINDOW_HEIGHT)  player.rect.y = WINDOW_HEIGHT - player.rect.h;

    if (game.mode == MODE_PVI) {
        moveCPU(cpu, ball, game.difficulty);
    } else {
        if (cpu.movingUp)   cpu.rect.y -= cpu.speedY;
        if (cpu.movingDown) cpu.rect.y += cpu.speedY;
        if (cpu.rect.y < 0)                             cpu.rect.y = 0;
        if (cpu.rect.y + cpu.rect.h > WINDOW_HEIGHT)   cpu.rect.y = WINDOW_HEIGHT - cpu.rect.h;
    }

    ball.posX += ball.velX;
    ball.posY += ball.velY;
    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

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

    if (SDL_HasIntersection(&ball.rect, &player.rect)) {
        ball.posX = (float)(player.rect.x + player.rect.w);
        ball.velX = fabsf(ball.velX);
        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) - (player.rect.y + player.rect.h / 2.0f)) / (player.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;
        if (ball.velX < BALL_SPEED_MAX) ball.velX *= 1.05f;
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    if (SDL_HasIntersection(&ball.rect, &cpu.rect)) {
        ball.posX = (float)(cpu.rect.x - ball.rect.w);
        ball.velX = -fabsf(ball.velX);
        float relHit = ((ball.rect.y + ball.rect.h / 2.0f) - (cpu.rect.y + cpu.rect.h / 2.0f)) / (cpu.rect.h / 2.0f);
        ball.velY = relHit * 5.0f;
        if (fabsf(ball.velX) < BALL_SPEED_MAX) ball.velX *= 1.05f;
        if (sdl.hitSound) Mix_PlayChannel(-1, sdl.hitSound, 0);
    }

    if (ball.rect.x + ball.rect.w < 0) {
        game.cpuScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, -1);
    }

    if (ball.rect.x > WINDOW_WIDTH) {
        game.playerScore++;
        if (sdl.goalSound) Mix_PlayChannel(-1, sdl.goalSound, 0);
        resetBall(ball, 1);
    }

    ball.rect.x = (int)ball.posX;
    ball.rect.y = (int)ball.posY;

    checkEndCondition(game);
}

// ================================================
// RENDER - HELPERS
// ================================================

void renderText(SDLContext& sdl, const std::string& text, TTF_Font* font,
                SDL_Color color, int x, int y, bool centered = false) {
    if (!font || text.empty()) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdl.renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst;
    if (centered) dst = { x - w / 2, y - h / 2, w, h };
    else          dst = { x, y - h / 2, w, h };

    SDL_RenderCopy(sdl.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void renderTextRight(SDLContext& sdl, const std::string& text, TTF_Font* font,
                     SDL_Color color, int rightX, int y) {
    if (!font || text.empty()) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdl.renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst = { rightX - w, y - h / 2, w, h };
    SDL_RenderCopy(sdl.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void renderFullBg(SDLContext& sdl, SDL_Texture* bg) {
    SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    if (bg)
        SDL_RenderCopy(sdl.renderer, bg, nullptr, &full);
}

void renderButton(SDLContext& sdl, SDL_Texture* btn, int centerX, int centerY,
                  int targetW, bool highlighted = false) {
    if (!btn) return;

    int origW, origH;
    SDL_QueryTexture(btn, nullptr, nullptr, &origW, &origH);

    float scale = (float)targetW / origW;
    int   w     = targetW;
    int   h     = (int)(origH * scale);

    SDL_Rect dst = { centerX - w / 2, centerY - h / 2, w, h };

    SDL_SetTextureColorMod(btn, highlighted ? 255 : 130,
                                highlighted ? 255 : 130,
                                highlighted ? 255 : 130);
    SDL_RenderCopy(sdl.renderer, btn, nullptr, &dst);
    SDL_SetTextureColorMod(btn, 255, 255, 255);
}

// ================================================
// RENDER - PANTALLAS
// ================================================

void renderMenu(SDLContext& sdl, const GameData& game) {
    renderFullBg(sdl, sdl.bgMenu);

    int cx   = WINDOW_WIDTH / 2;
    int btnW = 500;

    renderButton(sdl, sdl.btnPlay,    cx, 365, btnW, game.menuSelection == 0);
    renderButton(sdl, sdl.btnOptions, cx, 470, btnW, game.menuSelection == 1);
    renderButton(sdl, sdl.btnQuit,    cx, 568, btnW, game.menuSelection == 2);

    SDL_Color gray = { 140, 140, 140, 255 };
    renderText(sdl, "FLECHAS: NAVEGAR  |  ENTER: CONFIRMAR  |  ESC: SALIR",
               sdl.fontSmall, gray, cx, WINDOW_HEIGHT - 18, true);
}

void renderModeSelect(SDLContext& sdl, const GameData& game) {
    renderFullBg(sdl, sdl.bgModeSelect);

    int cx   = WINDOW_WIDTH / 2;
    int btnW = 630;

    renderButton(sdl, sdl.btnPvP,  cx, 345, btnW, game.menuSelection == 0);
    renderButton(sdl, sdl.btnPvI,  cx, 455, btnW, game.menuSelection == 1);
    renderButton(sdl, sdl.btnBack, cx, 555, 380,  game.menuSelection == 2);

    SDL_Color gray = { 140, 140, 140, 255 };
    renderText(sdl, "FLECHAS: NAVEGAR  |  ENTER: CONFIRMAR  |  ESC: VOLVER",
               sdl.fontSmall, gray, cx, WINDOW_HEIGHT - 18, true);
}

void renderCharSelect(SDLContext& sdl, const GameData& game) {
    renderFullBg(sdl, sdl.bgCharSelect);

    int cardW = WINDOW_WIDTH / NUM_CHARACTERS;
    int cardX = game.menuSelection * cardW;

    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(sdl.renderer, 180, 10, 10, 60);
    SDL_Rect fill = { cardX + 8, 120, cardW - 16, WINDOW_HEIGHT - 180 };
    SDL_RenderFillRect(sdl.renderer, &fill);

    SDL_SetRenderDrawColor(sdl.renderer, 220, 20, 20, 255);
    for (int i = 0; i < 3; i++) {
        SDL_Rect border = { fill.x - i, fill.y - i, fill.w + i*2, fill.h + i*2 };
        SDL_RenderDrawRect(sdl.renderer, &border);
    }

    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);

    SDL_Color yellow = { 255, 210, 0, 255 };
    SDL_Color gray   = { 150, 150, 150, 255 };

    if (game.mode == MODE_PVI) {
        const char* diffNames[] = { "FACIL", "MEDIO", "DIFICIL" };
        std::string diffStr = "DIFICULTAD: ";
        diffStr += diffNames[game.difficulty];
        diffStr += "  (1/2/3)";
        renderText(sdl, diffStr, sdl.fontSmall, yellow, WINDOW_WIDTH / 2, WINDOW_HEIGHT - 42, true);
    }

    renderText(sdl, "< > ELEGIR KILLER  |  ENTER: CONFIRMAR  |  ESC: VOLVER",
               sdl.fontSmall, gray, WINDOW_WIDTH / 2, WINDOW_HEIGHT - 18, true);
}

void renderGameplay(SDLContext& sdl, const Paddle& player, const Paddle& cpu,
                    const Ball& ball, const GameData& game) {

    renderFullBg(sdl, sdl.bgStage[game.selectedChar]);

    if (!sdl.bgStage[game.selectedChar]) {
        SDL_SetRenderDrawColor(sdl.renderer, 80, 80, 80, 255);
        for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
            SDL_Rect dash = { WINDOW_WIDTH / 2 - 2, y, 4, 10 };
            SDL_RenderFillRect(sdl.renderer, &dash);
        }
    }

    if (sdl.paddleTex) {
        SDL_RenderCopy(sdl.renderer, sdl.paddleTex, nullptr, &player.rect);
        SDL_RenderCopy(sdl.renderer, sdl.paddleTex, nullptr, &cpu.rect);
    } else {
        SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(sdl.renderer, &player.rect);
        SDL_RenderFillRect(sdl.renderer, &cpu.rect);
    }

    if (sdl.ballTex) {
        SDL_RenderCopy(sdl.renderer, sdl.ballTex, nullptr, &ball.rect);
    } else {
        SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(sdl.renderer, &ball.rect);
    }

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color red   = { 210,  20,  20, 255 };
    SDL_Color gray  = { 140, 140, 140, 255 };

    renderText(sdl, std::to_string(game.playerScore), sdl.fontLarge, white, WINDOW_WIDTH / 4,     50, true);
    renderText(sdl, std::to_string(game.cpuScore),    sdl.fontLarge, white, 3 * WINDOW_WIDTH / 4, 50, true);

    renderText(sdl, "P1",                                  sdl.fontSmall, gray, WINDOW_WIDTH / 4,     115, true);
    renderText(sdl, (game.mode == MODE_PVP) ? "P2" : "CPU",
               sdl.fontSmall, gray, 3 * WINDOW_WIDTH / 4, 115, true);

    int remaining = MATCH_DURATION - game.elapsedSeconds;
    if (remaining < 0) remaining = 0;
    std::ostringstream timer;
    timer << remaining / 60 << ":" << std::setfill('0') << std::setw(2) << remaining % 60;
    renderText(sdl, timer.str(), sdl.fontMedium, red, WINDOW_WIDTH / 2, 40, true);

    if (game.mode == MODE_PVI) {
        const char* diff[] = { "FACIL", "MEDIO", "DIFICIL" };
        renderText(sdl, diff[game.difficulty], sdl.fontSmall, gray, 18, WINDOW_HEIGHT - 18);
    } else {
        SDL_Color redLabel = { 200, 60, 60, 255 };
        renderText(sdl, "PvP", sdl.fontSmall, redLabel, 18, WINDOW_HEIGHT - 18);
    }

    std::string controls = (game.mode == MODE_PVP)
        ? "P1: W/S  |  P2: FLECHAS  |  ESC: MENU"
        : "W/S: MOVER  |  ESC: MENU";
    renderTextRight(sdl, controls, sdl.fontSmall, gray, WINDOW_WIDTH - 18, WINDOW_HEIGHT - 18);
}

void renderResult(SDLContext& sdl, const GameData& game) {
    renderFullBg(sdl, sdl.bgMenu);

    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 160);
    SDL_Rect overlay = { 200, 140, 880, 440 };
    SDL_RenderFillRect(sdl.renderer, &overlay);
    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color red   = { 220,  20,  20, 255 };
    SDL_Color gray  = { 170, 170, 170, 255 };

    int cx = WINDOW_WIDTH / 2;

    renderText(sdl, game.resultMessage, sdl.fontLarge, red, cx, 220, true);

    std::string scoreStr = "P1  " + std::to_string(game.playerScore) +
                           "  -  " + std::to_string(game.cpuScore) +
                           ((game.mode == MODE_PVP) ? "  P2" : "  CPU");
    renderText(sdl, scoreStr, sdl.fontMedium, white, cx, 350, true);

    std::string durStr = "Duracion: " + std::to_string(game.elapsedSeconds) + " segundos";
    renderText(sdl, durStr, sdl.fontSmall, gray, cx, 425, true);

    renderText(sdl, "Resultado guardado en resultados.csv", sdl.fontSmall, gray, cx, 468, true);
    renderText(sdl, "ENTER para volver al menu", sdl.fontMedium, white, cx, 530, true);
}

// ================================================
// PUNTO DE ENTRADA
// ================================================
int main(int /*argc*/, char* /*argv*/[]) {

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)  return 1;
    if (IMG_Init(IMG_INIT_PNG) == 0)         { SDL_Quit(); return 1; }
    if (TTF_Init() != 0)                     { IMG_Quit(); SDL_Quit(); return 1; }

    SDL_Window* window = SDL_CreateWindow(
        "SLAYER PONG",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) { TTF_Quit(); IMG_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { SDL_DestroyWindow(window); TTF_Quit(); IMG_Quit(); SDL_Quit(); return 1; }

    SDLContext sdl = {};
    sdl.window   = window;
    sdl.renderer = renderer;

    // Sprites
    sdl.paddleTex = IMG_LoadTexture(renderer, "assets/images/paddle.png");
    sdl.ballTex   = IMG_LoadTexture(renderer, "assets/images/ball.png");

    // Fondos de menus
    sdl.bgMenu       = IMG_LoadTexture(renderer, "assets/images/bg_menu.png");
    sdl.bgModeSelect = IMG_LoadTexture(renderer, "assets/images/bg_mode_select.png");
    sdl.bgCharSelect = IMG_LoadTexture(renderer, "assets/images/bg_char_select.png");

    // Fondos de escenarios
    sdl.bgStage[CHAR_FREDDY]      = IMG_LoadTexture(renderer, "assets/images/bg_stage_freddy.png");
    sdl.bgStage[CHAR_JASON]       = IMG_LoadTexture(renderer, "assets/images/bg_stage_jason.png");
    sdl.bgStage[CHAR_GHOSTFACE]   = IMG_LoadTexture(renderer, "assets/images/bg_stage_ghostface.png");
    sdl.bgStage[CHAR_LEATHERFACE] = IMG_LoadTexture(renderer, "assets/images/bg_stage_leather.png");

    // Botones
    sdl.btnPlay    = IMG_LoadTexture(renderer, "assets/images/PlayButton.png");
    sdl.btnOptions = IMG_LoadTexture(renderer, "assets/images/OptionsButton.png");
    sdl.btnQuit    = IMG_LoadTexture(renderer, "assets/images/QuitButton.png");
    sdl.btnPvP     = IMG_LoadTexture(renderer, "assets/images/PlayerVsPlayerButton.png");
    sdl.btnPvI     = IMG_LoadTexture(renderer, "assets/images/PlayerVsIaButton.png");
    sdl.btnBack    = IMG_LoadTexture(renderer, "assets/images/BackButton.png");

    // Fuentes
    sdl.fontLarge  = TTF_OpenFont("assets/fonts/font.ttf", 72);
    sdl.fontMedium = TTF_OpenFont("assets/fonts/font.ttf", 36);
    sdl.fontSmall  = TTF_OpenFont("assets/fonts/font.ttf", 22);

    // Audio
    initAudio(sdl);

    // Objetos del juego
    GameData game   = {};
    Paddle   player = {};
    Paddle   cpu    = {};
    Ball     ball   = {};
    initGame(game, player, cpu, ball);

    // ================================================
    // GAME LOOP  (Input -> Update -> Render -> Delay)
    // ================================================
    SDL_Event e;
    Uint32    frameStart;
    int       frameTime;

    while (game.running) {
        frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) game.running = false;
            handleInput(e, game, player, cpu, ball);
        }

        if (game.state == STATE_GAMEPLAY)
            updateGameplay(game, player, cpu, ball, sdl);

        SDL_SetRenderDrawColor(renderer, 10, 5, 5, 255);
        SDL_RenderClear(renderer);

        switch (game.state) {
            case STATE_MENU:        renderMenu(sdl, game);                         break;
            case STATE_MODE_SELECT: renderModeSelect(sdl, game);                   break;
            case STATE_CHAR_SELECT: renderCharSelect(sdl, game);                   break;
            case STATE_GAMEPLAY:    renderGameplay(sdl, player, cpu, ball, game);  break;
            case STATE_RESULT:      renderResult(sdl, game);                       break;
        }

        SDL_RenderPresent(renderer);

        frameTime = (int)(SDL_GetTicks() - frameStart);
        if (frameTime < FRAME_DELAY)
            SDL_Delay(FRAME_DELAY - frameTime);
    }

    // ================================================
    // LIMPIEZA
    // ================================================
    cleanupAudio(sdl);

    if (sdl.paddleTex) SDL_DestroyTexture(sdl.paddleTex);
    if (sdl.ballTex)   SDL_DestroyTexture(sdl.ballTex);

    if (sdl.bgMenu)       SDL_DestroyTexture(sdl.bgMenu);
    if (sdl.bgModeSelect) SDL_DestroyTexture(sdl.bgModeSelect);
    if (sdl.bgCharSelect) SDL_DestroyTexture(sdl.bgCharSelect);

    for (int i = 0; i < NUM_CHARACTERS; i++)
        if (sdl.bgStage[i]) SDL_DestroyTexture(sdl.bgStage[i]);

    if (sdl.btnPlay)    SDL_DestroyTexture(sdl.btnPlay);
    if (sdl.btnOptions) SDL_DestroyTexture(sdl.btnOptions);
    if (sdl.btnQuit)    SDL_DestroyTexture(sdl.btnQuit);
    if (sdl.btnPvP)     SDL_DestroyTexture(sdl.btnPvP);
    if (sdl.btnPvI)     SDL_DestroyTexture(sdl.btnPvI);
    if (sdl.btnBack)    SDL_DestroyTexture(sdl.btnBack);

    if (sdl.fontLarge)  TTF_CloseFont(sdl.fontLarge);
    if (sdl.fontMedium) TTF_CloseFont(sdl.fontMedium);
    if (sdl.fontSmall)  TTF_CloseFont(sdl.fontSmall);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
