#include "game.h"
#include <fstream>
#include <ctime>

// ================================================
// Devuelve la fecha y hora actual como string
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

// ================================================
// Guarda el resultado de la partida en CSV
// Crea el archivo con encabezado si no existe
// ================================================
void saveResult(const GameData& game) {
    const char* filename = "resultados.csv";

    // Verificar si el archivo ya existe para agregar encabezado
    bool exists = false;
    {
        std::ifstream check(filename);
        exists = check.is_open();
    }

    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    if (!exists) {
        file << "Fecha,Jugador,CPU,Duracion(s),Ganador,Dificultad\n";
    }

    file << getDateTime()                      << ","
         << game.playerScore                   << ","
         << game.cpuScore                      << ","
         << game.elapsedSeconds                << ","
         << getWinner(game)                    << ","
         << getDifficultyName(game.difficulty) << "\n";

    file.close();
}
