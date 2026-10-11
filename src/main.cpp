// ===========================================================================
//  QuizMaster  -  Test Your Knowledge
//
//  This is the only file that talks to main().
//
//  It does three things and nothing more:
//      1. creates the Game object,
//      2. runs it,
//      3. gives the number that run() returned back to Windows.
//
//  Everything else - the window, the screens, the questions, the score - lives
//  inside Game, so main() stays short enough to understand at a glance.
// ===========================================================================

#include "Game.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
    // argv[0] is the full path of the running .exe. The Game needs it so it can
    // find its "data" and "assets" folders no matter which folder the player
    // happened to start it from. If it is not available, "." is used instead.
    std::filesystem::path programFolder = ".";
    if (argc > 0 && argv[0] != nullptr)
        programFolder = std::filesystem::absolute(argv[0]).parent_path();

    // The object is created on the stack. C++ destroys it automatically when
    // main() returns, so no delete is needed anywhere in this project.
    Game game(programFolder.string());

    // run() opens the window and only returns once the player has closed it.
    // It returns 0 on a normal exit and 1 if something was missing (no font,
    // no window), which is the number Windows sees as "failed".
    const int exitCode = game.run();

    if (exitCode != 0)
        std::cerr << "[QuizMaster] The game stopped with an error.\n";

    return exitCode;
}
