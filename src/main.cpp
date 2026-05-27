#include "game.h"

// ================================================
// PUNTO DE ENTRADA
// Inicializa SDL, crea ventana/renderer,
// carga assets y ejecuta el Game Loop
// ================================================
int main(int /*argc*/, char* /*argv*/[]) {

    // ------ Inicializar subsistemas SDL ------
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)  return 1;
    if (IMG_Init(IMG_INIT_PNG) == 0)         { SDL_Quit(); return 1; }
    if (TTF_Init() != 0)                     { IMG_Quit(); SDL_Quit(); return 1; }

    // ------ Crear ventana ------
    SDL_Window* window = SDL_CreateWindow(
        "PONG - For Your Home TV",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) { TTF_Quit(); IMG_Quit(); SDL_Quit(); return 1; }

    // ------ Crear renderer (acelerado por GPU) ------
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    // ------ Construir contexto SDL ------
    SDLContext sdl = {};
    sdl.window   = window;
    sdl.renderer = renderer;

    // ------ Cargar texturas PNG ------
    sdl.paddleTex = IMG_LoadTexture(renderer, "assets/images/paddle.png");
    sdl.ballTex   = IMG_LoadTexture(renderer, "assets/images/ball.png");

    // ------ Cargar fuentes TTF ------
    sdl.fontLarge  = TTF_OpenFont("assets/fonts/font.ttf", 72);
    sdl.fontMedium = TTF_OpenFont("assets/fonts/font.ttf", 36);
    sdl.fontSmall  = TTF_OpenFont("assets/fonts/font.ttf", 22);

    // ------ Inicializar audio ------
    initAudio(sdl);

    // ------ Inicializar objetos del juego ------
    GameData game   = {};
    Paddle   player = {};
    Paddle   cpu    = {};
    Ball     ball   = {};
    initGame(game, player, cpu, ball);

    // ------ Variables del Game Loop ------
    SDL_Event e;
    Uint32    frameStart;
    int       frameTime;

    // ================================================
    // GAME LOOP PRINCIPAL
    // Patron: Input -> Update -> Render -> Delay
    // ================================================
    while (game.running) {
        frameStart = SDL_GetTicks();

        // 1) CAPTURA DE EVENTOS
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                game.running = false;
            }
            handleInput(e, game, player, cpu, ball);
        }

        // 2) ACTUALIZACION
        if (game.state == STATE_GAMEPLAY) {
            updateGameplay(game, player, cpu, ball, sdl);
        }

        // 3) RENDERIZADO
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        switch (game.state) {
            case STATE_MENU:       renderMenu(sdl);                         break;
            case STATE_DIFFICULTY: renderDifficulty(sdl);                   break;
            case STATE_GAMEPLAY:   renderGameplay(sdl, player, cpu, ball, game); break;
            case STATE_RESULT:     renderResult(sdl, game);                 break;
        }

        SDL_RenderPresent(renderer);

        // 4) CONTROL DE FPS (delay para alcanzar TARGET_FPS)
        frameTime = (int)(SDL_GetTicks() - frameStart);
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }

    // ================================================
    // LIMPIEZA DE RECURSOS
    // ================================================
    cleanupAudio(sdl);
    if (sdl.paddleTex)  SDL_DestroyTexture(sdl.paddleTex);
    if (sdl.ballTex)    SDL_DestroyTexture(sdl.ballTex);
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
