#include "game.h"
#include <string>
#include <sstream>
#include <iomanip>

// ================================================
// HELPER: Renderizar texto en pantalla
// Usa TTF_RenderText_Blended + SDL_CreateTextureFromSurface
// como indica el Bonus Track
// ================================================
void renderText(SDLContext& sdl, const std::string& text, TTF_Font* font,
                SDL_Color color, int x, int y, bool centered) {
    if (!font || text.empty()) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(sdl.renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

    SDL_Rect dst;
    if (centered) {
        dst = { x - w / 2, y - h / 2, w, h };
    } else {
        dst = { x, y, w, h };
    }

    SDL_RenderCopy(sdl.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

// ================================================
// Linea divisoria central punteada
// ================================================
static void renderDivider(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
        SDL_Rect dash = { WINDOW_WIDTH / 2 - 2, y, 4, 10 };
        SDL_RenderFillRect(renderer, &dash);
    }
}

// ================================================
// MAIN MENU
// ================================================
void renderMenu(SDLContext& sdl) {
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 220,   0, 255 };
    SDL_Color gray   = { 160, 160, 160, 255 };

    renderText(sdl, "PONG",                         sdl.fontLarge,  yellow, WINDOW_WIDTH / 2, 200, true);
    renderText(sdl, "FOR YOUR HOME TV",              sdl.fontSmall,  white,  WINDOW_WIDTH / 2, 295, true);
    renderText(sdl, "Presiona ENTER para comenzar",  sdl.fontMedium, white,  WINDOW_WIDTH / 2, 440, true);
    renderText(sdl, "ESC para salir",                sdl.fontSmall,  gray,   WINDOW_WIDTH / 2, 510, true);
}

// ================================================
// SELECCION DE DIFICULTAD
// ================================================
void renderDifficulty(SDLContext& sdl) {
    SDL_Color yellow = { 255, 220,   0, 255 };
    SDL_Color green  = {  80, 220,  80, 255 };
    SDL_Color orange = { 255, 165,   0, 255 };
    SDL_Color red    = { 255,  80,  80, 255 };
    SDL_Color gray   = { 160, 160, 160, 255 };

    renderText(sdl, "SELECCIONA DIFICULTAD", sdl.fontMedium, yellow, WINDOW_WIDTH / 2, 180, true);

    renderText(sdl, "1   -   FACIL",          sdl.fontMedium, green,  WINDOW_WIDTH / 2, 320, true);
    renderText(sdl, "2   -   MEDIO",          sdl.fontMedium, orange, WINDOW_WIDTH / 2, 400, true);
    renderText(sdl, "3   -   DIFICIL",        sdl.fontMedium, red,    WINDOW_WIDTH / 2, 480, true);

    renderText(sdl, "ESC para volver",        sdl.fontSmall,  gray,   WINDOW_WIDTH / 2, 585, true);
}

// ================================================
// GAMEPLAY (HUD + entidades)
// ================================================
void renderGameplay(SDLContext& sdl, const Paddle& player, const Paddle& cpu,
                    const Ball& ball, const GameData& game) {
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 220,   0, 255 };
    SDL_Color gray   = { 140, 140, 140, 255 };

    // Linea divisoria
    renderDivider(sdl.renderer);

    // ------ Paletas ------
    if (sdl.paddleTex) {
        SDL_RenderCopy(sdl.renderer, sdl.paddleTex, nullptr, &player.rect);
        SDL_RenderCopy(sdl.renderer, sdl.paddleTex, nullptr, &cpu.rect);
    } else {
        SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(sdl.renderer, &player.rect);
        SDL_RenderFillRect(sdl.renderer, &cpu.rect);
    }

    // ------ Pelota ------
    if (sdl.ballTex) {
        SDL_RenderCopy(sdl.renderer, sdl.ballTex, nullptr, &ball.rect);
    } else {
        SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(sdl.renderer, &ball.rect);
    }

    // ------ HUD: Marcadores ------
    renderText(sdl, std::to_string(game.playerScore), sdl.fontLarge, white, WINDOW_WIDTH / 4,      50, true);
    renderText(sdl, std::to_string(game.cpuScore),    sdl.fontLarge, white, 3 * WINDOW_WIDTH / 4,  50, true);

    // ------ HUD: Etiquetas ------
    renderText(sdl, "JUGADOR", sdl.fontSmall, gray, WINDOW_WIDTH / 4,     115, true);
    renderText(sdl, "CPU",     sdl.fontSmall, gray, 3 * WINDOW_WIDTH / 4, 115, true);

    // ------ HUD: Temporizador (cuenta regresiva) ------
    int remaining = MATCH_DURATION - game.elapsedSeconds;
    if (remaining < 0) remaining = 0;
    int mins = remaining / 60;
    int secs = remaining % 60;

    std::ostringstream timer;
    timer << mins << ":" << std::setfill('0') << std::setw(2) << secs;
    renderText(sdl, timer.str(), sdl.fontMedium, yellow, WINDOW_WIDTH / 2, 40, true);

    // ------ HUD: Dificultad (inferior izquierdo) ------
    const char* diffLabel[] = { "FACIL", "MEDIO", "DIFICIL" };
    renderText(sdl, diffLabel[game.difficulty], sdl.fontSmall, gray, 20, WINDOW_HEIGHT - 28);

    // ------ HUD: Controles (inferior derecho) ------
    renderText(sdl, "W/S - MOVER  |  ESC - MENU", sdl.fontSmall, gray,
               WINDOW_WIDTH - 20, WINDOW_HEIGHT - 28);
}

// ================================================
// PANTALLA DE RESULTADO
// ================================================
void renderResult(SDLContext& sdl, const GameData& game) {
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color yellow = { 255, 220,   0, 255 };
    SDL_Color gray   = { 170, 170, 170, 255 };

    renderText(sdl, game.resultMessage, sdl.fontLarge, yellow, WINDOW_WIDTH / 2, 200, true);

    std::string scoreStr = "Jugador  " + std::to_string(game.playerScore) +
                           "  -  " + std::to_string(game.cpuScore) + "  CPU";
    renderText(sdl, scoreStr, sdl.fontMedium, white, WINDOW_WIDTH / 2, 340, true);

    std::string durStr = "Duracion: " + std::to_string(game.elapsedSeconds) + " segundos";
    renderText(sdl, durStr, sdl.fontSmall, gray, WINDOW_WIDTH / 2, 415, true);

    renderText(sdl, "Resultado guardado en resultados.csv", sdl.fontSmall, gray,
               WINDOW_WIDTH / 2, 460, true);

    renderText(sdl, "Presiona ENTER para volver al menu", sdl.fontMedium, white,
               WINDOW_WIDTH / 2, 530, true);
}
