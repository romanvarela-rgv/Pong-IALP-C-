#include "game.h"

// ================================================
// HELPER: carga una textura, retorna nullptr si falla
// (el juego continua sin esa textura)
// ================================================
static SDL_Texture* loadTex(SDL_Renderer* r, const char* path) {
    return IMG_LoadTexture(r, path);
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

    // ------ Contexto SDL ------
    SDLContext sdl = {};
    sdl.window   = window;
    sdl.renderer = renderer;

    // ------ Sprites de entidades ------
    sdl.paddleTex = loadTex(renderer, "assets/images/paddle.png");
    sdl.ballTex   = loadTex(renderer, "assets/images/ball.png");

    // ------ Fondos de menus ------
    sdl.bgMenu       = loadTex(renderer, "assets/images/bg_menu.png");
    sdl.bgModeSelect = loadTex(renderer, "assets/images/bg_mode_select.png");
    sdl.bgCharSelect = loadTex(renderer, "assets/images/bg_char_select.png");

    // ------ Fondos de escenarios (uno por personaje) ------
    sdl.bgStage[CHAR_FREDDY]      = loadTex(renderer, "assets/images/bg_stage_freddy.png");
    sdl.bgStage[CHAR_JASON]       = loadTex(renderer, "assets/images/bg_stage_jason.png");
    sdl.bgStage[CHAR_GHOSTFACE]   = loadTex(renderer, "assets/images/bg_stage_ghostface.png");
    sdl.bgStage[CHAR_LEATHERFACE] = loadTex(renderer, "assets/images/bg_stage_leather.png");

    // ------ Botones ------
    sdl.btnPlay    = loadTex(renderer, "assets/images/PlayButton.png");
    sdl.btnOptions = loadTex(renderer, "assets/images/OptionsButton.png");
    sdl.btnQuit    = loadTex(renderer, "assets/images/QuitButton.png");
    sdl.btnPvP     = loadTex(renderer, "assets/images/PlayerVsPlayerButton.png");
    sdl.btnPvI     = loadTex(renderer, "assets/images/PlayerVsIaButton.png");
    sdl.btnBack    = loadTex(renderer, "assets/images/BackButton.png");

    // ------ Fuentes ------
    sdl.fontLarge  = TTF_OpenFont("assets/fonts/font.ttf", 72);
    sdl.fontMedium = TTF_OpenFont("assets/fonts/font.ttf", 36);
    sdl.fontSmall  = TTF_OpenFont("assets/fonts/font.ttf", 22);

    // ------ Audio ------
    initAudio(sdl);

    // ------ Objetos del juego ------
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

        // 1) INPUT
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) game.running = false;
            handleInput(e, game, player, cpu, ball);
        }

        // 2) UPDATE
        if (game.state == STATE_GAMEPLAY)
            updateGameplay(game, player, cpu, ball, sdl);

        // 3) RENDER
        SDL_SetRenderDrawColor(renderer, 10, 5, 5, 255);  // negro rojizo de fondo
        SDL_RenderClear(renderer);

        switch (game.state) {
            case STATE_MENU:       renderMenu(sdl, game);                          break;
            case STATE_MODE_SELECT: renderModeSelect(sdl, game);                   break;
            case STATE_CHAR_SELECT: renderCharSelect(sdl, game);                   break;
            case STATE_GAMEPLAY:   renderGameplay(sdl, player, cpu, ball, game);   break;
            case STATE_RESULT:     renderResult(sdl, game);                        break;
        }

        SDL_RenderPresent(renderer);

        // 4) CAP FPS
        frameTime = (int)(SDL_GetTicks() - frameStart);
        if (frameTime < FRAME_DELAY)
            SDL_Delay(FRAME_DELAY - frameTime);
    }

    // ================================================
    // LIMPIEZA
    // ================================================
    cleanupAudio(sdl);

    // Sprites
    if (sdl.paddleTex) SDL_DestroyTexture(sdl.paddleTex);
    if (sdl.ballTex)   SDL_DestroyTexture(sdl.ballTex);

    // Fondos de menu
    if (sdl.bgMenu)       SDL_DestroyTexture(sdl.bgMenu);
    if (sdl.bgModeSelect) SDL_DestroyTexture(sdl.bgModeSelect);
    if (sdl.bgCharSelect) SDL_DestroyTexture(sdl.bgCharSelect);

    // Fondos de escenarios
    for (int i = 0; i < NUM_CHARACTERS; i++)
        if (sdl.bgStage[i]) SDL_DestroyTexture(sdl.bgStage[i]);

    // Botones
    if (sdl.btnPlay)    SDL_DestroyTexture(sdl.btnPlay);
    if (sdl.btnOptions) SDL_DestroyTexture(sdl.btnOptions);
    if (sdl.btnQuit)    SDL_DestroyTexture(sdl.btnQuit);
    if (sdl.btnPvP)     SDL_DestroyTexture(sdl.btnPvP);
    if (sdl.btnPvI)     SDL_DestroyTexture(sdl.btnPvI);
    if (sdl.btnBack)    SDL_DestroyTexture(sdl.btnBack);

    // Fuentes
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
