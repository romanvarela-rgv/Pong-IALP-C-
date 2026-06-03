#include "game.h"
#include <string>
#include <sstream>
#include <iomanip>

// ================================================
// HELPERS DE TEXTO
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
    if (centered) dst = { x - w / 2, y - h / 2, w, h };
    else          dst = { x, y - h / 2, w, h };

    SDL_RenderCopy(sdl.renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

// Texto alineado a la derecha
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

// ================================================
// FONDO DE PANTALLA COMPLETO
// Si no hay textura, usa negro oscuro como fallback
// ================================================
void renderFullBg(SDLContext& sdl, SDL_Texture* bg) {
    SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    if (bg) {
        SDL_RenderCopy(sdl.renderer, bg, nullptr, &full);
    }
    // Si bg == nullptr el renderer ya limpio con el color oscuro del main loop
}

// ================================================
// BOTON CON ESTADO DE SELECCION
// highlighted = true → brillo completo
// highlighted = false → atenuado (70%)
// ================================================
void renderButton(SDLContext& sdl, SDL_Texture* btn, int centerX, int centerY,
                  int targetW, bool highlighted) {
    if (!btn) return;

    int origW, origH;
    SDL_QueryTexture(btn, nullptr, nullptr, &origW, &origH);

    float scale = (float)targetW / origW;
    int   w     = targetW;
    int   h     = (int)(origH * scale);

    SDL_Rect dst = { centerX - w / 2, centerY - h / 2, w, h };

    if (highlighted) {
        SDL_SetTextureColorMod(btn, 255, 255, 255);  // brillo total
    } else {
        SDL_SetTextureColorMod(btn, 130, 130, 130);  // atenuado
    }

    SDL_RenderCopy(sdl.renderer, btn, nullptr, &dst);
    SDL_SetTextureColorMod(btn, 255, 255, 255);  // reset
}

// ================================================
// MENU PRINCIPAL
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

// ================================================
// SELECCION DE MODO
// ================================================
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

// ================================================
// SELECCION DE PERSONAJE / ESCENARIO
// Overlay de resaltado sobre la tarjeta seleccionada
// ================================================
void renderCharSelect(SDLContext& sdl, const GameData& game) {
    renderFullBg(sdl, sdl.bgCharSelect);

    // Rectangulo de resaltado sobre el personaje seleccionado
    // La pantalla se divide en 4 columnas iguales
    int cardW = WINDOW_WIDTH / NUM_CHARACTERS;
    int cardX = game.menuSelection * cardW;

    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);

    // Relleno semitransparente rojo
    SDL_SetRenderDrawColor(sdl.renderer, 180, 10, 10, 60);
    SDL_Rect fill = { cardX + 8, 120, cardW - 16, WINDOW_HEIGHT - 180 };
    SDL_RenderFillRect(sdl.renderer, &fill);

    // Borde rojo solido (triple para grosor)
    SDL_SetRenderDrawColor(sdl.renderer, 220, 20, 20, 255);
    for (int i = 0; i < 3; i++) {
        SDL_Rect border = { fill.x - i, fill.y - i, fill.w + i*2, fill.h + i*2 };
        SDL_RenderDrawRect(sdl.renderer, &border);
    }

    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);

    // Dificultad (solo en modo PvI)
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

// ================================================
// GAMEPLAY
// ================================================
void renderGameplay(SDLContext& sdl, const Paddle& player, const Paddle& cpu,
                    const Ball& ball, const GameData& game) {

    // Fondo del escenario (ya tiene la linea central dibujada)
    renderFullBg(sdl, sdl.bgStage[game.selectedChar]);

    // Si no hay fondo, dibujamos la linea divisoria con SDL
    if (!sdl.bgStage[game.selectedChar]) {
        SDL_SetRenderDrawColor(sdl.renderer, 80, 80, 80, 255);
        for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
            SDL_Rect dash = { WINDOW_WIDTH / 2 - 2, y, 4, 10 };
            SDL_RenderFillRect(sdl.renderer, &dash);
        }
    }

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

    // ------ HUD ------
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color red    = { 210,  20,  20, 255 };
    SDL_Color gray   = { 140, 140, 140, 255 };

    // Puntajes
    renderText(sdl, std::to_string(game.playerScore), sdl.fontLarge, white, WINDOW_WIDTH / 4,     50, true);
    renderText(sdl, std::to_string(game.cpuScore),    sdl.fontLarge, white, 3 * WINDOW_WIDTH / 4, 50, true);

    // Etiquetas P1 / P2 o CPU
    renderText(sdl, "P1",                              sdl.fontSmall, gray, WINDOW_WIDTH / 4,     115, true);
    renderText(sdl, (game.mode == MODE_PVP) ? "P2" : "CPU",
               sdl.fontSmall, gray, 3 * WINDOW_WIDTH / 4, 115, true);

    // Temporizador (cuenta regresiva en rojo)
    int remaining = MATCH_DURATION - game.elapsedSeconds;
    if (remaining < 0) remaining = 0;
    std::ostringstream timer;
    timer << remaining / 60 << ":" << std::setfill('0') << std::setw(2) << remaining % 60;
    renderText(sdl, timer.str(), sdl.fontMedium, red, WINDOW_WIDTH / 2, 40, true);

    // Inferior izquierdo: modo / dificultad
    if (game.mode == MODE_PVI) {
        const char* diff[] = { "FACIL", "MEDIO", "DIFICIL" };
        renderText(sdl, diff[game.difficulty], sdl.fontSmall, gray, 18, WINDOW_HEIGHT - 18);
    } else {
        SDL_Color redLabel = { 200, 60, 60, 255 };
        renderText(sdl, "PvP", sdl.fontSmall, redLabel, 18, WINDOW_HEIGHT - 18);
    }

    // Inferior derecho: controles
    std::string controls = (game.mode == MODE_PVP)
        ? "P1: W/S  |  P2: FLECHAS  |  ESC: MENU"
        : "W/S: MOVER  |  ESC: MENU";
    renderTextRight(sdl, controls, sdl.fontSmall, gray, WINDOW_WIDTH - 18, WINDOW_HEIGHT - 18);
}

// ================================================
// RESULTADO
// ================================================
void renderResult(SDLContext& sdl, const GameData& game) {
    // Usa el fondo del menu como base
    renderFullBg(sdl, sdl.bgMenu);

    // Overlay semitransparente para mejorar legibilidad
    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 160);
    SDL_Rect overlay = { 200, 140, 880, 440 };
    SDL_RenderFillRect(sdl.renderer, &overlay);
    SDL_SetRenderDrawBlendMode(sdl.renderer, SDL_BLENDMODE_NONE);

    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color red    = { 220,  20,  20, 255 };
    SDL_Color gray   = { 170, 170, 170, 255 };

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
