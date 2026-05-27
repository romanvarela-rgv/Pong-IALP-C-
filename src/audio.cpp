#include "game.h"

// ================================================
// INICIALIZACION DE AUDIO (SDL_Mixer)
// Los archivos de audio son opcionales; si no
// estan presentes el juego funciona sin sonido.
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

    // Carga de musica de fondo (loop infinito)
    sdl.bgMusic = Mix_LoadMUS("assets/sounds/music.ogg");
    if (!sdl.bgMusic) sdl.bgMusic = Mix_LoadMUS("assets/sounds/music.mp3");

    // Carga de efectos de sonido
    sdl.hitSound  = Mix_LoadWAV("assets/sounds/hit.wav");
    sdl.goalSound = Mix_LoadWAV("assets/sounds/goal.wav");

    if (sdl.bgMusic) {
        Mix_VolumeMusic(48);
        Mix_PlayMusic(sdl.bgMusic, -1);
    }

    return true;
}

// ================================================
// LIMPIEZA DE AUDIO
// ================================================
void cleanupAudio(SDLContext& sdl) {
    if (sdl.hitSound)  { Mix_FreeChunk(sdl.hitSound);  sdl.hitSound  = nullptr; }
    if (sdl.goalSound) { Mix_FreeChunk(sdl.goalSound); sdl.goalSound = nullptr; }
    if (sdl.bgMusic)   { Mix_FreeMusic(sdl.bgMusic);   sdl.bgMusic   = nullptr; }
    Mix_CloseAudio();
    Mix_Quit();
}
