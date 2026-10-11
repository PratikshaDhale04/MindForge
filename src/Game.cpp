#include "Game.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>


namespace
{
// ---- Window layout --------------------------------------------------------
// The whole game is drawn for a fixed 1280 x 720 screen and centred in the
// window, so the layout is predictable and easy to change.
constexpr float DESIGN_WIDTH  = 1280.f;
constexpr float DESIGN_HEIGHT = 720.f;

constexpr float MARGIN = 80.f;          ///< Left margin for headings.
constexpr float CARD_TOP = 150.f;       ///< Where the content starts.
constexpr float FOOTER_Y = 646.f;       ///< Top of the bottom button row.

constexpr float FADE_DURATION = 0.25f;  ///< Length of the screen fade-in.

constexpr float SLIDE_DURATION = 0.34f; ///< Length of the screen-to-screen slide.

/// How long the "Correct!"/"Wrong!" message stays on screen before the NEXT

/// button becomes available. Feels long enough to read, short enough to keep
/// the quiz moving.
constexpr float FEEDBACK_DELAY = 0.7f;

/// Splits a line "a|b|c" into its parts. Returns false when the line is empty.
/// This is how scores.txt is read.
bool split(const std::string& line, char separator, std::vector<std::string>& parts)
{
    parts.clear();

    if (line.empty())
        return false;

    std::string current;
    for (const char c : line)
    {
        if (c == separator)
        {
            parts.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    parts.push_back(current);

    return true;
}

/// Today's date as YYYY-MM-DD, used to stamp a high score entry.
std::string todayAsString()
{
    const std::time_t now = std::time(nullptr);
    char buffer[16];
    std::tm parts{};
#ifdef _WIN32
    localtime_s(&parts, &now);
#else
    parts = *std::localtime(&now);
#endif
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &parts);
    return std::string(buffer);
}

/// Blend two colours, `amount` from 0 (all of `from`) to 1 (all of `to`).
/// The same little trick the Button class uses for its hover effect.
sf::Color mixColors(const sf::Color& from, const sf::Color& to, float amount)
{
    if (amount <= 0.f)
        return from;
    if (amount >= 1.f)
        return to;

    // Colour components are bytes, so every step has to be converted to float
    // and back again.
    const auto blend = [amount](std::uint8_t a, std::uint8_t b) {
        return static_cast<std::uint8_t>(static_cast<float>(a) +
                                         (static_cast<float>(b) - static_cast<float>(a)) * amount);
    };

    return sf::Color(blend(from.r, to.r), blend(from.g, to.g), blend(from.b, to.b),
                     blend(from.a, to.a));
}

} // namespace


// ===========================================================================
// Setup
// ===========================================================================

Game::Game(std::string programFolder)
    : m_programFolder(std::move(programFolder))
{
    m_theme = makeLightTheme();

}

bool Game::createWindow()
{
    // In SFML 3 the size is given as one sf::Vector2u, not as two numbers.
    m_window.create(sf::VideoMode(sf::Vector2u(static_cast<unsigned int>(DESIGN_WIDTH),
                                               static_cast<unsigned int>(DESIGN_HEIGHT))),
                    "QuizMaster  -  Test Your Knowledge");
    m_window.setFramerateLimit(60);
    m_window.setVerticalSyncEnabled(false);

    // The game is laid out for a fixed 1280 x 720 area. Telling the window
    // that this is its "view" makes SFML scale that area to whatever size the
    // window is and centre it, so the player can resize or maximise the window
    // without anything disappearing off the edge.
    m_window.setView(sf::View(sf::FloatRect(sf::Vector2f(0.f, 0.f),
                                            sf::Vector2f(DESIGN_WIDTH, DESIGN_HEIGHT))));
    return m_window.isOpen();
}


bool Game::loadFont()
{
    // 1. A font placed in assets/fonts is used first, so the game looks the
    //    same on every computer.
    const std::string folder = m_assetsFolder + "/fonts";
    const std::string preferred[] = {"QuizMaster-Regular.ttf", "arial.ttf", "segoeui.ttf"};

    for (const std::string& name : preferred)
    {
        // Check the file exists before asking SFML to open it. SFML prints a
        // long error to the console for every file it cannot open, and this
        // project would print three of them on every single start.
        if (std::filesystem::exists(folder + "/" + name) &&
            m_font.openFromFile(folder + "/" + name))
        {
            m_fontLoaded = true;
            return true;
        }
    }


    // 2. Any other .ttf the user dropped into the folder also works.
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".ttf" &&
            m_font.openFromFile(entry.path().string()))
        {
            m_fontLoaded = true;
            return true;
        }
    }

    // 3. Fall back to a font that ships with Windows. Without a font SFML
    //    cannot draw anything, so this last step keeps the game usable.
    const char* systemFonts[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/Windows/Fonts/tahoma.ttf"
    };

    for (const char* path : systemFonts)
    {
        if (m_font.openFromFile(path))
        {
            m_fontLoaded = true;
            return true;
        }
    }

    std::cerr << "[QuizMaster] No font could be loaded - text will not be visible.\n"
                 "              Put a .ttf file into assets/fonts/ and start again.\n";
    return false;
}

void Game::findFolders()
{
    // The finished program normally lives in build/bin, so the project folder
    // is a couple of levels up. This walks up from the .exe looking for the
    // one folder that has BOTH "data" and "assets" in it.
    //
    // Two details matter here:
    //
    //   * the search starts at the .exe, not at the current folder. If it
    //     started at the current folder the answer would change depending on
    //     where the game was started from - a shortcut, a batch file or an
    //     IDE would each give a different result.
    //
    //   * it insists on BOTH folders. CMake copies "assets" next to the .exe
    //     but not "data", so accepting either one on its own would stop at
    //     build/bin, find no "data" next to the .exe, and then be unable to
    //     save the high scores.

    std::error_code error;

    // The folder the .exe was started from - main() passes this in, taken from
    // argv[0]. Using it instead of the current folder is what makes the answer
    // the same whether the game is started from a shortcut, a batch file or an
    // IDE.
    std::filesystem::path start = m_programFolder;
    if (start.empty())
        start = ".";

    start = std::filesystem::absolute(start, error);

    for (int level = 0; level < 5; ++level)
    {
        const std::filesystem::path data   = start / "data";
        const std::filesystem::path assets = start / "assets";

        if (std::filesystem::is_directory(data, error) &&
            std::filesystem::is_directory(assets, error))
        {
            m_dataFolder   = data.string();
            m_assetsFolder = assets.string();
            return;
        }

        const std::filesystem::path parent = start.parent_path();
        if (parent == start)      // reached the drive root, stop
            break;
        start = parent;
    }

    // Nothing found. Use the folders next to the .exe and make sure they
    // exist, so the game still runs and can still save the high scores.
    const std::filesystem::path beside = std::filesystem::absolute(".", error);
    m_dataFolder   = (beside / "data").string();
    m_assetsFolder = (beside / "assets").string();

    std::filesystem::create_directories(m_dataFolder, error);
    std::filesystem::create_directories(m_assetsFolder, error);
    std::filesystem::create_directories(std::filesystem::path(m_assetsFolder) / "sounds", error);
}

void Game::loadSettings()
{
    // Missing file? Then the defaults below are simply kept. This is why the
    // game works on the very first run.
    std::ifstream file(m_dataFolder + "/settings.txt");
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        // Comments start with '#'. They have to be skipped first, because a
        // comment may well contain an '=' in its text.
        if (line.empty() || line[0] == '#')
            continue;

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos)
            continue;


        const std::string key   = line.substr(0, equals);
        const std::string value = line.substr(equals + 1);

        if (key == "sound")            m_soundOn = (value == "1");
        else if (key == "animations")  m_animationsOn = (value == "1");
        else if (key == "theme")       m_lightTheme = (value == "light");
    }
}

void Game::saveSettings()
{
    // Writing to a temporary file first means an interrupted write cannot
    // destroy the previous settings.
    const std::string temporary = m_dataFolder + "/settings.tmp";
    std::ofstream file(temporary);
    if (!file.is_open())
    {
        std::cerr << "[QuizMaster] Could not save the settings.\n";
        return;
    }

    file << "sound="       << (m_soundOn ? 1 : 0)      << '\n';
    file << "animations="  << (m_animationsOn ? 1 : 0) << '\n';
    file << "theme="       << (m_lightTheme ? "light" : "dark") << '\n';
    file.close();

    if (!file)
    {
        std::cerr << "[QuizMaster] Writing the settings failed.\n";
        std::filesystem::remove(temporary);
        return;
    }

    // The rename is the only step that can still fail, and it is the one that
    // puts the new settings in place.
    //
    // std::filesystem::rename is used rather than std::rename on purpose: the
    // plain C one refuses to replace a file that is already there, so on
    // MinGW the second time the settings were saved the rename would fail and
    // the change would be lost. The std::filesystem version replaces the
    // destination and reports the reason through an error code.
    std::error_code error;
    std::filesystem::rename(temporary, std::filesystem::path(m_dataFolder) / "settings.txt", error);

    if (error)
    {
        std::cerr << "[QuizMaster] Could not replace settings.txt: "
                  << error.message() << '\n';
        std::filesystem::remove(temporary);
    }
}

void Game::loadHighScores()
{
    m_highScores.clear();

    std::ifstream file(m_dataFolder + "/scores.txt");
    if (!file.is_open())
        return;   // First run - the file is created when a quiz is finished.

    std::string line;
    while (std::getline(file, line))
    {
        // A line starting with '#' is a comment. The header of the file is one,
        // so it has to be skipped or it would turn into a bogus score of 0.
        if (line.empty() || line[0] == '#')
            continue;

        // Format: Player|Score|MaxScore|Category|Difficulty|Date
        std::vector<std::string> parts;
        if (!split(line, '|', parts) || parts.size() < 4)
            continue;   // Skip a line we do not understand instead of crashing.


        ScoreEntry entry;
        entry.player     = parts[0];
        entry.score      = std::atoi(parts[1].c_str());
        entry.maxScore   = std::atoi(parts[2].c_str());
        entry.category   = parts[3];
        entry.difficulty = parts.size() > 4 ? parts[4] : "";
        entry.date       = parts.size() > 5 ? parts[5] : "";
        m_highScores.push_back(entry);
    }

    // Highest score first. The lambda keeps the sort stable for equal scores.
    std::stable_sort(m_highScores.begin(), m_highScores.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
}

bool Game::saveHighScores()
{
    const std::string temporary = m_dataFolder + "/scores.tmp";
    std::ofstream file(temporary);
    if (!file.is_open())
    {
        std::cerr << "[QuizMaster] Could not save the high scores.\n";

        return false;
    }

    for (const ScoreEntry& entry : m_highScores)
    {
        file << entry.player << '|' << entry.score << '|' << entry.maxScore << '|'
             << entry.category << '|' << entry.difficulty << '|' << entry.date << '\n';
    }
    file.close();

    // A failed write is reported to the player as a notification, so a broken
    // data folder never goes unnoticed.
    if (!file)
    {
        std::cerr << "[QuizMaster] Writing the high scores failed.\n";
        std::filesystem::remove(temporary);
        return false;
    }

    // std::filesystem::rename rather than std::rename: the plain C version
    // refuses to overwrite an existing file, so every save after the first one
    // would silently fail. See saveSettings() for the longer explanation.
    std::error_code error;
    std::filesystem::rename(temporary, std::filesystem::path(m_dataFolder) / "scores.txt", error);

    if (error)
    {
        std::cerr << "[QuizMaster] Could not replace scores.txt: "
                  << error.message() << '\n';
        std::filesystem::remove(temporary);
        return false;
    }

    return true;
}


// ===========================================================================
// Main loop
// ===========================================================================

int Game::run()
{
    findFolders();
    loadSettings();

    m_theme = m_lightTheme ? makeLightTheme() : makeDarkTheme();
    m_audio.setEnabled(m_soundOn);

    if (!createWindow())
    {
        std::cerr << "[QuizMaster] The window could not be created.\n";
        return 1;
    }

    if (!loadFont())
    {
        // Without a font nothing can be drawn, so say so instead of showing a
        // blank window.
        m_window.close();
        return 1;
    }

    m_audio.load(m_assetsFolder + "/sounds");
    loadHighScores();

    // --- the drifting dots in the background -------------------------------
    // A fixed seed means the same pattern every run, which looks intentional
    // rather than random. srand() keeps std::rand() used in update() in step.
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (int i = 0; i < 46; ++i)
    {
        Spark spark;
        spark.position = sf::Vector2f(static_cast<float>(std::rand() % 1280),
                                      static_cast<float>(std::rand() % 720));
        spark.speed    = 6.f + static_cast<float>(std::rand() % 16);
        spark.radius   = 1.f + static_cast<float>(std::rand() % 25) / 10.f;
        spark.phase    = static_cast<float>(std::rand() % 628) / 100.f;
        m_sparks.push_back(spark);
    }

    // Give every button its size and label. Doing this once here keeps the
    // draw functions free of numbers.
    const float buttonWidth  = 300.f;
    const float buttonHeight = 56.f;
    const float buttonX      = (DESIGN_WIDTH - buttonWidth) / 2.f;

    m_startButton      = Button({{buttonX, 296.f}, {buttonWidth, buttonHeight}}, "START QUIZ", ButtonStyle::Accent);
    m_highScoreButton  = Button({{buttonX, 366.f}, {buttonWidth, buttonHeight}}, "HIGH SCORES");
    m_settingsButton   = Button({{buttonX, 436.f}, {buttonWidth, buttonHeight}}, "SETTINGS");
    m_aboutButton      = Button({{buttonX, 506.f}, {buttonWidth, buttonHeight}}, "ABOUT");
    m_exitButton       = Button({{buttonX, 576.f}, {buttonWidth, buttonHeight}}, "EXIT", ButtonStyle::Danger);

    m_continueButton   = Button({{buttonX, 430.f}, {buttonWidth, buttonHeight}}, "CONTINUE", ButtonStyle::Accent);
    m_backButton       = Button({{MARGIN, FOOTER_Y}, {150.f, 46.f}}, "BACK");
    m_homeScreenButton = Button({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}}, "HOME");
    m_nextButton       = Button({{DESIGN_WIDTH - MARGIN - 200.f, FOOTER_Y}, {200.f, 46.f}},
                                "NEXT", ButtonStyle::Accent);

    // Four buttons in one row on the result screen.
    const float resultButtonWidth = 250.f;
    m_againButton         = Button({{ 80.f, 560.f}, {resultButtonWidth, 52.f}}, "PLAY AGAIN", ButtonStyle::Accent);
    m_againHighScoreButton= Button({{370.f, 560.f}, {resultButtonWidth, 52.f}}, "HIGH SCORES");
    m_homeButton          = Button({{660.f, 560.f}, {resultButtonWidth, 52.f}}, "HOME");
    m_exitResultButton    = Button({{950.f, 560.f}, {resultButtonWidth, 52.f}}, "EXIT", ButtonStyle::Danger);

    m_soundButton      = Button({{MARGIN, 230.f}, {560.f, 64.f}}, "SOUND");
    m_animationsButton = Button({{MARGIN, 310.f}, {560.f, 64.f}}, "ANIMATIONS");
    m_themeButton      = Button({{MARGIN, 390.f}, {560.f, 64.f}}, "THEME");

    // The main action of each screen gets a coloured halo, which is what makes
    // the layout look like a real menu instead of a list of grey boxes.
    m_startButton.setGlow(m_theme.accent);
    m_continueButton.setGlow(m_theme.accent);
    m_nextButton.setGlow(m_theme.accent);
    m_againButton.setGlow(m_theme.accent);


    // Category cards: two columns, two rows.
    const std::vector<std::string> categories = availableCategories();
    m_categoryCount = 0;
    for (std::size_t i = 0; i < categories.size() && i < 4; ++i)
    {
        const int column = static_cast<int>(i) % 2;
        const int row    = static_cast<int>(i) / 2;
        const sf::FloatRect box{{340.f + column * 320.f, 230.f + row * 150.f}, {280.f, 120.f}};
        m_categoryButtons[m_categoryCount] = Button(box, categories[i]);
        m_categoryButtons[m_categoryCount].setSubtitle(
            std::to_string(countQuestions(categories[i], "")) + " questions");
        ++m_categoryCount;
    }

    // Difficulty cards in a single row. The subtitle repeats the two numbers
    // that actually change with the difficulty, so the player can see them.
    // Both come from the constants in Quiz.h, so a label can never disagree
    // with what the quiz really does.
    struct DifficultyCard { const char* name; int questions; int seconds; };
    const DifficultyCard difficultyCards[3] = {
        {"Easy",   EASY_QUESTIONS,   EASY_TIME},
        {"Medium", MEDIUM_QUESTIONS, MEDIUM_TIME},
        {"Hard",   HARD_QUESTIONS,   HARD_TIME}
    };
    for (int i = 0; i < 3; ++i)
    {
        m_difficultyButtons[i] =
            Button({{250.f + i * 270.f, 250.f}, {240.f, 160.f}}, difficultyCards[i].name);
        m_difficultyButtons[i].setSubtitle(
            std::to_string(difficultyCards[i].questions) + " questions  -  " +
            std::to_string(difficultyCards[i].seconds) + " seconds each");
    }

    // Four answer options, stacked vertically. They start below y = 400 so
    // they can never reach the notification strip in the top right corner.
    for (int i = 0; i < 4; ++i)
    {
        m_optionButtons[i] = Button({{290.f, 400.f + i * 58.f}, {700.f, 54.f}}, "");
    }

    // High score filter buttons: ALL + up to four categories.
    m_filterButtons[0] = Button({{MARGIN, 150.f}, {140.f, 42.f}}, "ALL", ButtonStyle::Accent);
    for (int i = 1; i < 5; ++i)
    {
        m_filterButtons[i] = Button({{MARGIN + i * 150.f, 150.f}, {140.f, 42.f}}, "");
        m_filterButtons[i].setVisible(false);
    }
    {
        // Only show filters for categories that really exist.
        const std::vector<std::string> known = availableCategories();
        for (std::size_t i = 0; i < known.size() && i < 4; ++i)
        {
            m_filterButtons[i + 1].setLabel(known[i]);
            m_filterButtons[i + 1].setVisible(true);
        }
    }


    m_fade = 0.f;
    m_previousScreen = Screen::Home;   // no transition on the very first frame
    m_transition = 1.f;

    // ---- say hello ---------------------------------------------------------
    // One welcome message, plus a warning only if something is really missing.
    notify(NotificationKind::Info, "Welcome to QuizMaster",
           std::to_string(questionBank().size()) + " questions are ready. Good luck!",
           3.4f);

    if (!m_audio.hasAnySound())
        notify(NotificationKind::Warning, "Sound files missing",
               "The game runs silently. Add .wav files to assets/sounds.", 4.0f);
    else if (m_highScores.empty())
        notify(NotificationKind::Info, "No scores yet",
               "Finish a quiz to start the high score table", 3.4f);

    sf::Clock clock;

    while (m_window.isOpen())
    {
        // Clamping stops a long pause (dragging the window, loading a file)
        // from making the timer jump and the animations skip.
        m_delta = std::min(clock.restart().asSeconds(), 0.05f);

        handleEvents();
        update(m_delta);
        draw();
    }


    return 0;
}

void Game::handleEvents()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
            return;
        }

        // --- the mouse ------------------------------------------------------
        // Only these two events change the Button class. Without them a button
        // could never tell a click from a hover.
        if (const sf::Event::MouseButtonPressed* press = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (press->button == sf::Mouse::Button::Left)
            {
                m_leftDown = true;
                // Remembered for this frame, so a quick click is never lost.
                m_pressLatch = true;
            }
            continue;
        }

        if (const sf::Event::MouseButtonReleased* release = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (release->button == sf::Mouse::Button::Left)
                m_leftDown = false;
            continue;
        }

        // Leaving the window with the button held down must not leave the game
        // thinking the mouse is still pressed.
        if (event->is<sf::Event::MouseLeft>())
        {
            m_leftDown = false;
            m_pressLatch = false;
            continue;
        }

        // --- the keyboard ---------------------------------------------------
        if (const sf::Event::KeyPressed* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Escape)
            {
                m_audio.play(SoundEffect::Click);
                switch (m_screen)
                {
                case Screen::Home:    m_window.close(); break;
                case Screen::Name:    goTo(Screen::Home); break;
                case Screen::Category: goTo(Screen::Home); break;
                case Screen::Difficulty: goTo(Screen::Category); break;
                case Screen::Quiz:    goTo(Screen::Home); break;
                case Screen::Result:  goTo(Screen::Home); break;
                case Screen::HighScores: goTo(Screen::Home); break;
                case Screen::Settings:goTo(Screen::Home); break;
                case Screen::About:   goTo(Screen::Home); break;
                }
                continue;
            }

            // Enter continues from the name screen.
            if (key->code == sf::Keyboard::Key::Enter && m_screen == Screen::Name)
            {
                if (!m_nameInput.empty())
                {
                    m_audio.play(SoundEffect::Click);
                    m_playerName = m_nameInput;
                    goTo(Screen::Category);
                }
                else
                {
                    m_message = "Please enter your name first.";
                    notify(NotificationKind::Warning, "Name required",
                           "Type a name before you continue.");
                }
                continue;
            }

            if (m_screen == Screen::Quiz)
            {
                // A, B, C and D answer the current question, and Enter (or the
                // space bar) moves on once the feedback is showing.
                if (m_quiz.phase() == QuizPhase::Asking)
                {
                    switch (key->code)
                    {
                    case sf::Keyboard::Key::A: m_pendingAnswer = 'A'; break;
                    case sf::Keyboard::Key::B: m_pendingAnswer = 'B'; break;
                    case sf::Keyboard::Key::C: m_pendingAnswer = 'C'; break;
                    case sf::Keyboard::Key::D: m_pendingAnswer = 'D'; break;
                    default: break;
                    }
                }
                else if (key->code == sf::Keyboard::Key::Enter ||
                         key->code == sf::Keyboard::Key::Space)
                {
                    m_pendingNext = true;
                }
                continue;
            }
        }

        // Typing on the name screen. Only ordinary printable characters are
        // accepted, which keeps the stored name simple and safe to show in a
        // text file.
        if (m_screen == Screen::Name)
        {
            if (const sf::Event::TextEntered* typed = event->getIf<sf::Event::TextEntered>())
            {
                const char32_t character = typed->unicode;
                if (character == 8)   // backspace
                {
                    if (!m_nameInput.empty())
                        m_nameInput.pop_back();
                }
                else if (character >= 32 && character < 127 && m_nameInput.size() < 20)
                {
                    m_nameInput += static_cast<char>(character);
                }
                m_message.clear();
            }
        }
    }
}


void Game::update(float deltaSeconds)
{
    // --- the screen-to-screen slide ---------------------------------------
    if (m_previousScreen != m_screen)
    {
        if (!m_animationsOn)
        {
            // With animations switched off the old screen must be dropped at
            // once. If it were kept, it would be drawn on top of the new one
            // for the rest of the game.
            m_previousScreen = m_screen;
            m_transition = 1.f;
        }
        else
        {
            m_transition = std::min(1.f, m_transition + deltaSeconds / SLIDE_DURATION);
            if (m_transition >= 1.f)
                m_previousScreen = m_screen;   // the old screen is gone for good
        }
    }
    else
    {
        m_transition = 1.f;
    }

    // The fade-in runs on every screen.
    if (m_animationsOn)
        m_fade = std::min(1.f, m_fade + deltaSeconds / FADE_DURATION);
    else
        m_fade = 1.f;

    // --- the drifting dots in the background -------------------------------
    for (Spark& spark : m_sparks)
    {
        spark.position.y -= spark.speed * deltaSeconds;
        spark.phase += deltaSeconds;
        if (spark.position.y < -10.f)
        {
            // A dot that leaves the top of the screen reappears at the bottom,
            // at a new random x, so the pattern never repeats exactly.
            spark.position.y = DESIGN_HEIGHT + 10.f;
            spark.position.x = static_cast<float>(std::rand() % static_cast<int>(DESIGN_WIDTH));
        }
    }

    // --- the notifications -------------------------------------------------
    m_notifications.update(deltaSeconds);

    // --- the two progress bars slide towards their target -------------------
    // A bar that jumped straight to its new width would look like a glitch, so
    // the drawn width always "catches up" a little each frame.
    if (m_screen == Screen::Quiz)
    {
        const auto approach = [this, deltaSeconds](float& shown, float target) {
            const float step = std::max(0.15f, std::abs(target - shown) * 6.f) * deltaSeconds;
            if (shown < target) shown = std::min(target, shown + step);
            else if (shown > target) shown = std::max(target, shown - step);
        };

        approach(m_timeBarShown, m_quiz.timeFraction());
        approach(m_quizBarShown, m_quiz.questionCount() == 0 ? 0.f
            : static_cast<float>(m_quiz.currentIndex() +
                                 (m_quiz.phase() == QuizPhase::Asking ? 0 : 1)) /
              static_cast<float>(m_quiz.questionCount()));
    }

    // Timer warning pulse.
    if (m_screen == Screen::Quiz && m_quiz.timeIsLow())
        m_timerFlash += deltaSeconds * 9.f;
    else
        m_timerFlash = 0.f;

    // The result screen bars grow once, when the screen is first entered.
    if (m_screen == Screen::Result)
        m_statBars = m_animationsOn ? std::min(1.f, m_statBars + deltaSeconds / 0.7f) : 1.f;

    switch (m_screen)
    {
    case Screen::Home:       updateHome(); break;
    case Screen::Name:       updateName(); break;
    case Screen::Category:   updateCategory(); break;
    case Screen::Difficulty: updateDifficulty(); break;
    case Screen::Quiz:       updateQuiz(); break;
    case Screen::Result:     updateResult(); break;
    case Screen::HighScores: updateHighScores(); break;
    case Screen::Settings:   updateSettings(); break;
    case Screen::About:      updateAbout(); break;
    }

    // The press latch has done its job for this frame, so it is cleared here.
    // Without this a click could be counted twice.
    m_pressLatch = false;
}

void Game::drawScreen(Screen screen)
{
    switch (screen)
    {
    case Screen::Home:       showHome(); break;
    case Screen::Name:       showNameScreen(); break;
    case Screen::Category:   showCategoryScreen(); break;
    case Screen::Difficulty: showDifficultyScreen(); break;
    case Screen::Quiz:       showQuizScreen(); break;
    case Screen::Result:     showResultScreen(); break;
    case Screen::HighScores: showHighScores(); break;
    case Screen::Settings:   showSettings(); break;
    case Screen::About:      showAbout(); break;
    }
}

void Game::draw()
{
    m_window.clear(m_theme.background);

    // --- the transition between two screens --------------------------------
    // The screen we are leaving stays still, and the screen we are going to
    // slides in from the right on top of it. Drawing the new one last is the
    // important part: every screen starts with an opaque background, so
    // whichever is drawn last is the one the player sees.
    if (m_previousScreen != m_screen)
    {
        // 1-(1-t)^2 is fast at the start and gentle at the end.
        const float t = 1.f - (1.f - m_transition) * (1.f - m_transition);
        const float shift = (1.f - t) * 160.f;

        drawScreen(m_previousScreen);

        // A view is the window onto the drawing area. Moving it sideways moves
        // everything drawn after it - that is the whole slide animation.
        setViewShift(shift);
        drawScreen(m_screen);
        setViewShift(0.f);

        // A panel in the background colour, fading away, so the two screens
        // are not simply lying on top of each other.
        sf::RectangleShape cover(sf::Vector2f(DESIGN_WIDTH + 340.f, DESIGN_HEIGHT));
        cover.setPosition({-170.f, 0.f});
        cover.setFillColor(sf::Color(m_theme.background.r, m_theme.background.g, m_theme.background.b,
                                     static_cast<std::uint8_t>(190.f * (1.f - t))));
        m_window.draw(cover);
    }
    else
    {
        drawScreen(m_screen);
    }

    // --- the notifications are always on top -------------------------------
    m_notifications.draw(m_window, m_font, m_theme);

    // --- the fade-in when a screen first appears ----------------------------
    if (m_fade < 1.f)
    {
        sf::RectangleShape overlay(sf::Vector2f(DESIGN_WIDTH, DESIGN_HEIGHT));
        overlay.setPosition({0.f, 0.f});
        overlay.setFillColor(sf::Color(m_theme.background.r, m_theme.background.g,
                                       m_theme.background.b,
                                       static_cast<std::uint8_t>(255.f * (1.f - m_fade))));
        m_window.draw(overlay);
    }

    m_window.display();
}

void Game::goTo(Screen screen)
{
    if (screen == m_screen)
        return;

    m_previousScreen = m_screen;
    m_screen         = screen;
    m_message.clear();

    // A mouse press that belonged to the old screen must not also count on the
    // new one, so every button is let go of.
    m_leftDown    = false;
    m_pressLatch  = false;
    m_pendingNext = false;
    for (Button& button : m_optionButtons)          button.releasePress();
    for (Button& button : m_categoryButtons)        button.releasePress();
    for (Button& button : m_difficultyButtons)      button.releasePress();
    for (Button& button : m_filterButtons)          button.releasePress();
    m_startButton.releasePress();
    m_highScoreButton.releasePress();
    m_settingsButton.releasePress();
    m_aboutButton.releasePress();
    m_exitButton.releasePress();
    m_continueButton.releasePress();
    m_backButton.releasePress();
    m_nextButton.releasePress();
    m_againButton.releasePress();
    m_againHighScoreButton.releasePress();
    m_homeButton.releasePress();
    m_exitResultButton.releasePress();
    m_soundButton.releasePress();
    m_animationsButton.releasePress();
    m_themeButton.releasePress();

    m_transition = 0.f;
    m_fade       = m_animationsOn ? 0.f : 1.f;
}

void Game::setViewShift(float x)
{
    // The view is always rebuilt from scratch, never moved from wherever it
    // happened to be. That matters: moving the current view would make every
    // call add up, so the slide animation would push the screen further and
    // further sideways instead of coming back to where it started.
    //
    // Rebuilding it also means setViewShift(0) really does put everything back.
    m_window.setView(sf::View(sf::FloatRect(sf::Vector2f(x, 0.f),
                                            sf::Vector2f(DESIGN_WIDTH, DESIGN_HEIGHT))));
}

sf::Vector2f Game::mousePosition() const
{

    const sf::Vector2i pixel = sf::Mouse::getPosition(m_window);
    const sf::Vector2u size  = m_window.getSize();

    // A zero-sized window would cause a division by zero below.
    if (size.x == 0 || size.y == 0)
        return sf::Vector2f(DESIGN_WIDTH / 2.f, DESIGN_HEIGHT / 2.f);

    // Every button is placed in 1280 x 720 design coordinates, so the real
    // mouse position has to be converted back into those. This is the same
    // scaling and centring that SFML does for drawing (see createWindow()).
    const float scale = std::min(static_cast<float>(size.x) / DESIGN_WIDTH,
                                 static_cast<float>(size.y) / DESIGN_HEIGHT);
    const float offsetX = (static_cast<float>(size.x) - DESIGN_WIDTH * scale) / 2.f;
    const float offsetY = (static_cast<float>(size.y) - DESIGN_HEIGHT * scale) / 2.f;

    return sf::Vector2f((static_cast<float>(pixel.x) - offsetX) / scale,
                        (static_cast<float>(pixel.y) - offsetY) / scale);
}


// ===========================================================================
// Shared drawing helpers
//
// All the repeated drawing code lives in these few functions, so a screen is
// mostly just "which buttons, which numbers".
// ===========================================================================

sf::Text Game::makeText(const std::string& text, unsigned int size, const sf::Color& color,
                        float x, float y) const
{
    // In SFML 3 a text object cannot exist without a font, so an empty
    // sf::Font is passed when nothing could be loaded. Drawing then simply
    // shows nothing instead of crashing.
    sf::Text result(m_font, text);
    result.setCharacterSize(size);
    result.setFillColor(color);
    result.setPosition({x, y});
    return result;
}

sf::Text Game::makeTextCentred(const std::string& text, unsigned int size, const sf::Color& color,
                               float x, float y) const
{
    sf::Text result = makeText(text, size, color, x, y);

    // Only centre when a font is available - getLocalBounds() needs one.
    if (m_fontLoaded)
    {
        const sf::FloatRect bounds = result.getLocalBounds();
        result.setOrigin(bounds.getCenter());
        result.setPosition({x, y});
    }

    return result;
}

sf::Text Game::makeTextRight(const std::string& text, unsigned int size, const sf::Color& color,
                             float rightX, float y) const
{
    sf::Text result = makeText(text, size, color, 0.f, 0.f);

    if (m_fontLoaded)
    {
        // setOrigin(width, 0) means "the given point is the top left corner of
        // the text's own box", so x is now where the text *ends*.
        const sf::FloatRect bounds = result.getLocalBounds();
        result.setOrigin({bounds.size.x, 0.f});
        result.setPosition({rightX, y});
    }


    return result;
}


std::vector<std::string> Game::wrapText(const std::string& text, unsigned int size,

                                        float maxWidth) const
{
    std::vector<std::string> lines;
    if (!m_fontLoaded)
    {
        lines.push_back(text);
        return lines;
    }

    // Measure one character to know roughly how many fit on a line. This is
    // simpler than measuring every word, and close enough for our layout.
    sf::Text probe = makeText("M", size, sf::Color::White, 0.f, 0.f);
    const float characterWidth = std::max(1.f, probe.getLocalBounds().size.x);

    const std::size_t maxCharacters = static_cast<std::size_t>(maxWidth / characterWidth);
    if (maxCharacters == 0)
    {
        lines.push_back(text);
        return lines;
    }

    std::istringstream words(text);
    std::string word;
    std::string current;

    while (words >> word)
    {
        if (current.empty())
        {
            current = word;
        }
        else if (current.size() + 1 + word.size() <= maxCharacters)
        {
            current += ' ';
            current += word;
        }
        else
        {
            lines.push_back(current);
            current = word;
        }
    }

    if (!current.empty())
        lines.push_back(current);

    return lines;
}

void Game::drawBackground()
{
    // ---- 1. a vertical gradient, drawn as thin strips ---------------------
    // A real gradient needs a texture in SFML, but blending between two colours
    // in a loop of 1-pixel rows looks exactly the same and needs no file.
    // 36 rows over 720 pixels is smooth enough and costs almost nothing.
    constexpr int STRIPS = 36;
    const float stripHeight = DESIGN_HEIGHT / static_cast<float>(STRIPS);

    for (int i = 0; i < STRIPS; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(STRIPS - 1);
        sf::RectangleShape strip(sf::Vector2f(DESIGN_WIDTH, stripHeight + 1.f));
        strip.setPosition({0.f, i * stripHeight});
        strip.setFillColor(mixColors(m_theme.backgroundTop, m_theme.background, t));
        m_window.draw(strip);
    }

    // ---- 2. two large soft light blobs ------------------------------------
    // More circles, each more transparent than the last, gives a blurred
    // looking glow without a single image file.
    const float drift = m_animationsOn ? std::sin(m_homeAnim * 0.6f) * 50.f : 0.f;
    const float lift  = m_animationsOn ? std::cos(m_homeAnim * 0.45f) * 30.f : 0.f;

    const auto glowBlob = [this](sf::Vector2f centre, float radius, const sf::Color& color) {
        for (int layer = 6; layer >= 1; --layer)
        {
            const float r = radius * static_cast<float>(layer) / 6.f;
            sf::CircleShape circle(r);
            circle.setOrigin({r, r});
            circle.setPosition(centre);
            circle.setFillColor(sf::Color(color.r, color.g, color.b,
                                          static_cast<std::uint8_t>(7.f / static_cast<float>(layer) * 6.f)));
            m_window.draw(circle);
        }
    };

    glowBlob({240.f + drift, 120.f + lift}, 300.f, m_theme.accent);
    glowBlob({1060.f - drift, 600.f - lift}, 280.f, m_theme.accent2);
    glowBlob({1150.f + lift * 0.5f, 90.f}, 200.f, m_theme.success);

    // ---- 3. the drifting dots ---------------------------------------------
    drawSparkles();
}

void Game::drawSparkles()
{
    if (!m_animationsOn)
        return;

    for (const Spark& spark : m_sparks)
    {
        // Each dot breathes a little, so they do not all look the same.
        const float pulse = 0.65f + 0.35f * std::sin(spark.phase * 1.7f);
        const float radius = spark.radius * pulse;

        sf::CircleShape dot(radius);
        dot.setOrigin({radius, radius});
        dot.setPosition(spark.position);
        dot.setFillColor(sf::Color(m_theme.accent.r, m_theme.accent.g, m_theme.accent.b,
                                   static_cast<std::uint8_t>(70.f * pulse)));
        m_window.draw(dot);
    }
}

void Game::drawGlowTitle(const std::string& text, unsigned int size, float y, float pulse)
{
    // A title is drawn four times: three soft copies behind it and the sharp
    // one on top. Offsetting the copies by a pixel each way is what makes the
    // letters look like they are glowing.
    for (int layer = 3; layer >= 1; --layer)
    {
        const float offset = static_cast<float>(layer) * 1.4f;
        const sf::Color color = mixColors(m_theme.background, m_theme.accent, 0.9f);

        m_window.draw(makeTextCentred(text, size,
                                      sf::Color(color.r, color.g, color.b,
                                                static_cast<std::uint8_t>(70.f / static_cast<float>(layer) *
                                                                            (0.7f + 0.3f * pulse))),
                                      DESIGN_WIDTH / 2.f - offset, y - offset));
        m_window.draw(makeTextCentred(text, size,
                                      sf::Color(color.r, color.g, color.b,
                                                static_cast<std::uint8_t>(70.f / static_cast<float>(layer) *
                                                                            (0.7f + 0.3f * pulse))),
                                      DESIGN_WIDTH / 2.f + offset, y + offset));
    }

    m_window.draw(makeTextCentred(text, size, m_theme.text, DESIGN_WIDTH / 2.f, y));
}


void Game::drawHeader(const std::string& title, const std::string& subtitle)
{
    m_window.draw(makeText(title, 40, m_theme.text, MARGIN, 56.f));

    // A short coloured line under the title, the simplest possible decoration.
    sf::RectangleShape underline(sf::Vector2f(72.f, 4.f));
    underline.setPosition({MARGIN, 110.f});
    underline.setFillColor(m_theme.accent);
    m_window.draw(underline);

    if (!subtitle.empty())
        m_window.draw(makeText(subtitle, 17, m_theme.textMuted, MARGIN, 124.f));
}


void Game::drawFooterHint(const std::string& text)
{
    // Starts to the right of the BACK button, which always sits at the bottom
    // left corner of the screens that have one.
    m_window.draw(makeText(text, 14, m_theme.textMuted, 250.f, 662.f));
}


void Game::drawCard(const sf::FloatRect& area)
{
    sf::RectangleShape card(area.size);
    card.setPosition(area.position);
    card.setFillColor(m_theme.surface);
    card.setOutlineThickness(1.f);
    card.setOutlineColor(m_theme.border);
    m_window.draw(card);
}

void Game::drawProgressBar(const sf::FloatRect& area, float fraction, const sf::Color& color)
{
    // The empty track.
    sf::RectangleShape track(area.size);
    track.setPosition(area.position);
    track.setFillColor(m_theme.button);
    m_window.draw(track);

    // The filled part, clamped so a value above 1 cannot draw outside.
    const float width = area.size.x * std::clamp(fraction, 0.f, 1.f);
    if (width > 0.f)
    {
        // A glow under the filled part, so the bar looks like it is lit.
        for (int layer = 2; layer >= 1; --layer)
        {
            const float grow = static_cast<float>(layer) * 2.f;
            sf::RectangleShape glow({width + grow * 2.f, area.size.y + grow * 2.f});
            glow.setPosition(area.position - sf::Vector2f(grow, grow));
            glow.setFillColor(sf::Color::Transparent);
            glow.setOutlineThickness(3.f);
            glow.setOutlineColor(sf::Color(color.r, color.g, color.b,
                                           static_cast<std::uint8_t>(60.f / static_cast<float>(layer))));
            m_window.draw(glow);
        }

        sf::RectangleShape fill(sf::Vector2f(width, area.size.y));
        fill.setPosition(area.position);
        fill.setFillColor(color);
        m_window.draw(fill);

        // A bright line along the top of the filled part.
        sf::RectangleShape shine(sf::Vector2f(width, std::max(1.f, area.size.y * 0.22f)));
        shine.setPosition({area.position.x, area.position.y + 1.f});
        shine.setFillColor(sf::Color(255, 255, 255, 55));
        m_window.draw(shine);
    }

    sf::RectangleShape border(area.size);
    border.setPosition(area.position);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(m_theme.border);
    m_window.draw(border);
}


void Game::drawBackButton()
{
    m_backButton.draw(m_window, m_font, m_theme);
}

void Game::drawHomeButton()
{
    m_homeScreenButton.draw(m_window, m_font, m_theme);
}

void Game::drawParagraph(const std::vector<std::string>& lines, unsigned int size,
                         const sf::Color& color, float x, float y, float lineHeight)
{
    // A loop over the lines - the simplest way to print a block of text.
    float lineY = y;
    for (const std::string& line : lines)
    {
        m_window.draw(makeText(line, size, color, x, lineY));
        lineY += lineHeight;
    }
}


// ===========================================================================
// Screen updates - input handling and per-frame animation
// ===========================================================================

void Game::notify(NotificationKind kind, const std::string& title,
                  const std::string& message, float seconds)
{
    // No cutting the text short here. The notification panel wraps a long
    // message onto several lines by itself, so whatever is written is shown.
    m_notifications.add(kind, title, message, seconds);
}

void Game::updateButton(Button& button, const sf::Vector2f& mouse)
{
    button.setAnimationsEnabled(m_animationsOn);

    // m_pressLatch remembers that the mouse was pressed at some point during
    // this frame. Without it a very quick click - press and release between two
    // frames - would be thrown away, because update() only ever sees the mouse
    // in its "up" state and the button would never learn that a press started.
    button.update(mouse, m_leftDown || m_pressLatch, m_delta);
}


// ===========================================================================
// Screen updates - input handling and per-frame animation
//
// A "show..." function draws a screen, the matching "update..." function reacts
// to the player. The game loop calls update() first and draw() after it, so a
// click is always seen before the screen is redrawn.
// ===========================================================================

void Game::updateHome()
{
    m_homeAnim += m_delta;

    const sf::Vector2f mouse = mousePosition();

    updateButton(m_startButton, mouse);
    updateButton(m_highScoreButton, mouse);
    updateButton(m_settingsButton, mouse);
    updateButton(m_aboutButton, mouse);
    updateButton(m_exitButton, mouse);

    if (m_startButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        m_nameInput.clear();
        goTo(Screen::Name);
    }
    else if (m_highScoreButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        loadHighScores();          // re-read, so a score from a previous run shows up
        goTo(Screen::HighScores);
    }
    else if (m_settingsButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Settings);
    }
    else if (m_aboutButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::About);
    }
    else if (m_exitButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        m_window.close();
    }
}

void Game::updateName()
{
    const sf::Vector2f mouse = mousePosition();

    // The blinking cursor in the text box.
    m_caretTimer += m_delta;

    // CONTINUE stays clickable even while the name is empty, so that clicking
    // it can explain *why* nothing happens instead of leaving a dead button.
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_continueButton, mouse);
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    if (m_continueButton.wasClicked())
    {
        // The empty-name check lives here, and also in the Enter key handler,
        // so both ways of continuing behave the same.
        if (m_nameInput.empty())
        {
            m_message = "Please enter your name first.";
            notify(NotificationKind::Warning, "Name required",
                   "Type a name before you continue.");
        }
        else
        {
            m_audio.play(SoundEffect::Click);
            m_playerName = m_nameInput;
            m_notifications.add(NotificationKind::Success, "Welcome, " + m_playerName + "!",
                                "Now pick a category.", 2.6f);
            goTo(Screen::Category);
        }
    }

    else if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

void Game::updateCategory()
{
    const sf::Vector2f mouse = mousePosition();
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    for (int i = 0; i < m_categoryCount; ++i)
    {
        // The category the player picked earlier stays highlighted, so going
        // back from the difficulty screen shows the choice.
        m_categoryButtons[i].setStyle(m_categoryButtons[i].label() == m_selectedCategory
                                         ? ButtonStyle::Accent
                                         : ButtonStyle::Neutral);
        updateButton(m_categoryButtons[i], mouse);

        if (m_categoryButtons[i].wasClicked())
        {
            m_audio.play(SoundEffect::Click);
            m_selectedCategory = m_categoryButtons[i].label();
            m_message.clear();
            goTo(Screen::Difficulty);
            return;
        }
    }

    if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

void Game::updateDifficulty()
{
    const sf::Vector2f mouse = mousePosition();
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    for (int i = 0; i < 3; ++i)
    {
        updateButton(m_difficultyButtons[i], mouse);

        if (m_difficultyButtons[i].wasClicked())
        {
            m_selectedDifficulty = m_difficultyButtons[i].label();

            // Start the quiz straight away. If the category is too small the
            // message explains it instead of starting a broken quiz.
            std::string error;
            if (m_quiz.start(m_selectedCategory, m_selectedDifficulty, error))
            {
                m_audio.play(SoundEffect::Click);
                m_displayScore   = 0.f;
                m_feedbackTimer = 0.f;
                m_pendingAnswer = '\0';
                m_pendingNext   = false;
                m_timeBarShown  = 1.f;
                m_quizBarShown  = 0.f;
                goTo(Screen::Quiz);

                notify(NotificationKind::Info, "Quiz started",
                       m_selectedCategory + "  -  " + m_selectedDifficulty + "  -  " +
                       std::to_string(m_quiz.questionCount()) + " questions");

                // A short warning is useful, but it should not sit on top of
                // the real question for long.
                if (!error.empty())
                    notify(NotificationKind::Warning, "Heads up", error, 2.6f);
            }

            else
            {
                m_message = error;
                notify(NotificationKind::Warning, "Cannot start", error);
            }
            return;
        }
    }


    if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Category);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

void Game::updateQuiz()
{
    const sf::Vector2f mouse = mousePosition();

    // The quiz itself decides when the time is up.
    m_quiz.update();

    const Question& question = m_quiz.currentQuestion();
    const char letters[4] = {'A', 'B', 'C', 'D'};
    const std::string options[4] = {question.optionA, question.optionB,
                                    question.optionC, question.optionD};

    const bool asking = (m_quiz.phase() == QuizPhase::Asking);

    // When the time runs out the Quiz class flips itself to Feedback on its
    // own. The game only notices that one frame later, so the "Time up"
    // message is shown here. The flag stops it showing twice.
    if (!asking && !m_timeUpShown && m_quiz.lastAnswerTimedOut())
    {
        m_timeUpShown = true;
        m_audio.play(SoundEffect::Wrong);
        notify(NotificationKind::Warning, "Time up",
               "Question " + std::to_string(m_quiz.currentIndex() + 1) + " was not answered", 2.2f);
    }
    if (asking)
        m_timeUpShown = false;

    for (int i = 0; i < 4; ++i)
    {
        m_optionButtons[i].setLabel(std::string(1, letters[i]) + ".   " + options[i]);
        m_optionButtons[i].setEnabled(asking);

        // Once the answer is in, the buttons are disabled and the mouse press
        // is reported as "not down" as well. A click can therefore never be
        // counted twice, and no half-finished press is left over for the next
        // question - which matters here, because the quiz does not change
        // screen between questions.
        m_optionButtons[i].update(mouse, asking && (m_leftDown || m_pressLatch), m_delta);
    }


    // BACK is only useful while a question is still open; NEXT appears once
    // the feedback has been shown.
    m_backButton.setVisible(asking);
    updateButton(m_backButton, mouse);

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, 18.f}, {150.f, 46.f}});
    updateButton(m_homeScreenButton, mouse);

    m_nextButton.setVisible(!asking);
    m_nextButton.setBounds({{DESIGN_WIDTH - MARGIN - 200.f, FOOTER_Y}, {200.f, 46.f}});
    m_nextButton.setEnabled(m_feedbackTimer >= FEEDBACK_DELAY);
    updateButton(m_nextButton, mouse);

    if (asking)
    {
        // A click on an answer ...
        char chosen = '\0';
        for (int i = 0; i < 4; ++i)
        {
            if (m_optionButtons[i].wasClicked())
            {
                chosen = letters[i];
                break;
            }
        }

        // ... or the A / B / C / D keys on the keyboard.
        if (chosen == '\0')
            chosen = m_pendingAnswer;

        if (chosen != '\0')
        {
            m_quiz.answer(chosen);
            m_audio.play(m_quiz.lastAnswerCorrect() ? SoundEffect::Correct
                                                    : SoundEffect::Wrong);
            m_feedbackTimer = 0.f;

            // A short message on the right, so the big one in the middle is not
            // the only feedback the player gets.
            if (m_quiz.lastAnswerCorrect())
            {
                if (m_quiz.streak() >= 2)
                    notify(NotificationKind::Success, "Streak!",
                           std::to_string(m_quiz.streak()) + " correct answers in a row", 1.8f);
            }
            else
            {
                notify(NotificationKind::Error, "Wrong answer",
                       std::string("The right one was ") + question.correctAnswer, 1.8f);
            }
        }

    }
    else
    {
        // After answering, the feedback stays up for a moment and then the
        // NEXT button becomes available.
        m_feedbackTimer += m_delta;

        const bool canContinue = m_feedbackTimer >= FEEDBACK_DELAY;
        if (canContinue && (m_nextButton.wasClicked() || m_pendingNext))
        {
            m_audio.play(SoundEffect::Click);
            m_pendingNext = false;
            m_quiz.advance();
            m_feedbackTimer = 0.f;

            if (m_quiz.phase() == QuizPhase::Finished)
            {
                recordHighScore();
                m_displayScore = 0.f;
                m_statBars     = 0.f;
                goTo(Screen::Result);

                // Tell the player plainly what happened, including when the
                // score could not be written to disk.
                if (m_scoreSaved)
                {
                    notify(NotificationKind::Success, "Result saved",
                           std::to_string(m_quiz.score()) + " points for " + m_playerName);
                }
                else
                {
                    notify(NotificationKind::Error, "Could not save",
                           "The score could not be written to disk");
                }
                return;
            }

        }
    }

    m_pendingAnswer = '\0';

    if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

void Game::updateResult()
{
    const sf::Vector2f mouse = mousePosition();

    // Count the score up instead of showing it instantly.
    const float target = static_cast<float>(m_quiz.score());
    if (m_displayScore < target)
        m_displayScore = std::min(target, m_displayScore + m_delta * 70.f);
    else
        m_displayScore = target;

    updateButton(m_againButton, mouse);
    updateButton(m_againHighScoreButton, mouse);
    updateButton(m_homeButton, mouse);
    updateButton(m_exitResultButton, mouse);

    if (m_againButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        std::string error;
        if (m_quiz.start(m_selectedCategory, m_selectedDifficulty, error))
        {
            m_displayScore   = 0.f;
            m_feedbackTimer = 0.f;
            m_pendingAnswer = '\0';
            m_pendingNext   = false;
            m_timeBarShown  = 1.f;
            m_quizBarShown  = 0.f;
            goTo(Screen::Quiz);
        }

        else
        {
            m_message = error;
            notify(NotificationKind::Warning, "Cannot restart", error);
            goTo(Screen::Difficulty);
        }
    }
    else if (m_againHighScoreButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::HighScores);
    }
    else if (m_homeButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_exitResultButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        m_window.close();
    }
}

void Game::updateHighScores()
{
    const sf::Vector2f mouse = mousePosition();
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    for (int i = 0; i < 5; ++i)
    {
        if (!m_filterButtons[i].visible())
            continue;

        // The active filter is drawn in the accent colour.
        const bool active = m_filterButtons[i].label() == m_highScoreFilter;
        m_filterButtons[i].setStyle(active ? ButtonStyle::Accent : ButtonStyle::Neutral);
        updateButton(m_filterButtons[i], mouse);

        if (m_filterButtons[i].wasClicked())
        {
            m_audio.play(SoundEffect::Click);
            // "ALL" is stored as an empty string, which means "no filter".
            m_highScoreFilter = (i == 0) ? std::string() : m_filterButtons[i].label();
        }
    }

    if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

void Game::updateSettings()
{
    const sf::Vector2f mouse = mousePosition();

    // The subtitle of each row is the value the switch is on right now.
    m_soundButton.setSubtitle(m_soundOn ? "ON" : "OFF");
    m_animationsButton.setSubtitle(m_animationsOn ? "ON" : "OFF");
    m_themeButton.setSubtitle(m_lightTheme ? "Light" : "Dark");

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_soundButton, mouse);
    updateButton(m_animationsButton, mouse);
    updateButton(m_themeButton, mouse);
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    if (m_soundButton.wasClicked())
    {
        m_soundOn = !m_soundOn;
        m_audio.setEnabled(m_soundOn);
        // Play it straight away so the player can hear that it works.
        m_audio.play(SoundEffect::Click);
        saveSettings();

        if (m_soundOn && !m_audio.hasAnySound())
            notify(NotificationKind::Warning, "No sound files",
                   "Add .wav files into assets/sounds", 3.0f);
        else
            notify(NotificationKind::Success, "Sound",
                   m_soundOn ? "Turned on" : "Turned off", 1.6f);
    }
    else if (m_animationsButton.wasClicked())
    {
        m_animationsOn = !m_animationsOn;
        m_fade = 1.f;      // stop the current fade immediately
        saveSettings();
        notify(NotificationKind::Success, "Animations",
               m_animationsOn ? "Turned on" : "Turned off", 1.6f);
    }
    else if (m_themeButton.wasClicked())
    {
        m_lightTheme = !m_lightTheme;
        m_theme = m_lightTheme ? makeLightTheme() : makeDarkTheme();
        // The buttons keep their glow, which belongs to the old theme.
        m_startButton.setGlow(m_theme.accent);
        m_continueButton.setGlow(m_theme.accent);
        m_nextButton.setGlow(m_theme.accent);
        m_againButton.setGlow(m_theme.accent);
        m_notifications.clear();
        saveSettings();
        notify(NotificationKind::Info, "Theme",
               m_lightTheme ? "Light theme applied" : "Dark theme applied", 1.8f);
    }
    else if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}


void Game::updateAbout()
{
    const sf::Vector2f mouse = mousePosition();
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    updateButton(m_backButton, mouse);
    updateButton(m_homeScreenButton, mouse);

    if (m_backButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
    else if (m_homeScreenButton.wasClicked())
    {
        m_audio.play(SoundEffect::Click);
        goTo(Screen::Home);
    }
}

// ===========================================================================
// Saving a finished quiz
// ===========================================================================

void Game::recordHighScore()
{
    ScoreEntry entry;
    entry.player     = m_playerName;
    entry.score      = m_quiz.score();
    entry.maxScore   = m_quiz.maximumScore();
    entry.category   = m_selectedCategory;
    entry.difficulty = m_selectedDifficulty;
    entry.date       = todayAsString();
    m_highScores.push_back(entry);

    // Highest first. The sort is stable, so two results with the same score
    // keep the order they were played in.
    std::stable_sort(m_highScores.begin(), m_highScores.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });

    // Only the best ten are kept, so scores.txt never grows forever.
    if (m_highScores.size() > static_cast<std::size_t>(MAX_HIGH_SCORES))
        m_highScores.resize(static_cast<std::size_t>(MAX_HIGH_SCORES));

    // saveHighScores() reports back whether the write worked, so the result
    // screen never claims a score was saved when it was not.
    m_scoreSaved = saveHighScores();
}


// ===========================================================================
// Drawing the screens
// ===========================================================================

void Game::showHome()
{
    drawBackground();

    // The title drifts up and down very slowly - the simplest animation there
    // is, and it makes the screen feel alive.
    const float drift  = m_animationsOn ? std::sin(m_homeAnim * 1.2f) * 6.f : 0.f;
    const float pulse  = m_animationsOn ? 0.5f + 0.5f * std::sin(m_homeAnim * 1.8f) : 1.f;

    drawGlowTitle("QUIZMASTER", 72, 150.f + drift, pulse);

    m_window.draw(makeTextCentred("Test Your Knowledge", 24, m_theme.textMuted,
                                  DESIGN_WIDTH / 2.f, 235.f));

    // A two-tone line: the accent fades into the second accent colour.
    for (int i = 0; i < 80; ++i)
    {
        sf::RectangleShape dot(sf::Vector2f(2.f, 4.f));
        dot.setPosition({DESIGN_WIDTH / 2.f - 80.f + i * 2.f, 278.f});
        dot.setFillColor(mixColors(m_theme.accent, m_theme.accent2,
                                   static_cast<float>(i) / 79.f));
        m_window.draw(dot);
    }

    m_startButton.draw(m_window, m_font, m_theme);
    m_highScoreButton.draw(m_window, m_font, m_theme);
    m_settingsButton.draw(m_window, m_font, m_theme);
    m_aboutButton.draw(m_window, m_font, m_theme);
    m_exitButton.draw(m_window, m_font, m_theme);

    m_window.draw(makeTextRight("C++  -  SFML  -  CMake", 14, m_theme.textMuted,
                                DESIGN_WIDTH - MARGIN, 668.f));
}


void Game::showNameScreen()
{
    drawBackground();
    drawHeader("Enter Your Name", "Type your name with the keyboard, then press Enter");

    // The box the name appears in.
    const sf::FloatRect box{{340.f, 210.f}, {600.f, 110.f}};
    drawCard(box);

    if (m_nameInput.empty())
    {
        m_window.draw(makeText("your name here", 34, m_theme.textMuted, box.position.x + 32.f,
                               box.position.y + 34.f));
    }
    else
    {
        m_window.draw(makeText(m_nameInput, 34, m_theme.text,
                               box.position.x + 32.f, box.position.y + 34.f));

        // A "_" that blinks on and off, so it is obvious this box is typing.
        if (m_caretTimer < 0.6f)
        {
            sf::Text caret = makeText("_", 34, m_theme.accent, box.position.x + 32.f,
                                      box.position.y + 34.f);
            // Put the cursor just after the last typed character.
            if (m_fontLoaded)
            {
                sf::Text probe = makeText(m_nameInput, 34, m_theme.text, 0.f, 0.f);
                caret.setPosition({box.position.x + 32.f + probe.getLocalBounds().size.x + 6.f,
                                   box.position.y + 34.f});

            }
            m_window.draw(caret);
        }
    }

    // The error message only appears when there really is a problem.
    if (!m_message.empty())
        m_window.draw(makeTextCentred(m_message, 19, m_theme.warning, DESIGN_WIDTH / 2.f, 350.f));

    m_continueButton.draw(m_window, m_font, m_theme);
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Press Enter to continue. Esc goes back to the menu.");
}

void Game::showCategoryScreen()
{
    drawBackground();
    drawHeader("Choose a Category", "Step 1 of 2  -  what would you like to be asked about?");

    for (int i = 0; i < m_categoryCount; ++i)
        m_categoryButtons[i].draw(m_window, m_font, m_theme);

    if (!m_message.empty())
        m_window.draw(makeTextCentred(m_message, 19, m_theme.warning, DESIGN_WIDTH / 2.f, 540.f));

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Pick one card. Esc goes back.");
}

void Game::showDifficultyScreen()
{
    drawBackground();
    drawHeader("Choose a Difficulty",
               "Step 2 of 2  -  the difficulty sets the timer and how long the quiz is");

    for (int i = 0; i < 3; ++i)
        m_difficultyButtons[i].draw(m_window, m_font, m_theme);

    if (!m_message.empty())
        m_window.draw(makeTextCentred(m_message, 19, m_theme.warning, DESIGN_WIDTH / 2.f, 450.f));

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Pick one card to start the quiz. Esc goes back.");
}

void Game::showQuizScreen()
{
    drawBackground();

    const Question& question = m_quiz.currentQuestion();
    const bool asking = (m_quiz.phase() == QuizPhase::Asking);

    // ---- the top bar -----------------------------------------------------
    // Everything in the top area stays on the LEFT of the screen. The strip on
    // the right is reserved for notifications, so a message can never cover
    // the question number or the timer.
    m_window.draw(makeText("QUIZMASTER", 22, m_theme.accent, MARGIN, 22.f));
    m_window.draw(makeText(m_selectedCategory + "  |  " + m_selectedDifficulty,
                           15, m_theme.textMuted, MARGIN, 50.f));
    m_window.draw(makeText("Question " + std::to_string(m_quiz.currentIndex() + 1) +
                           " / " + std::to_string(m_quiz.questionCount()),
                           22, m_theme.text, MARGIN, 76.f));

    // ---- the timer -------------------------------------------------------
    const float fraction = m_quiz.timeFraction();
    sf::Color timerColor = fraction > 0.5f ? m_theme.success
                        : (fraction > 0.25f ? m_theme.warning : m_theme.danger);

    // The last seconds blink so a running-out timer is easy to notice.
    if (m_animationsOn && m_quiz.timeIsLow())
    {
        const float pulse = 0.5f + 0.5f * std::sin(m_timerFlash);
        timerColor = mixColors(m_theme.text, m_theme.danger, pulse);
    }

    m_window.draw(makeText("Time left", 15, m_theme.textMuted, MARGIN, 110.f));
    m_window.draw(makeText(std::to_string(static_cast<int>(std::ceil(m_quiz.timeLeft()))) + " s",
                           26, timerColor, MARGIN + 78.f, 102.f));

    // ---- the two bars ----------------------------------------------------
    // Both are 780 wide instead of the full 1120, so they also stay clear of
    // the notification strip. They slide to their new width instead of
    // jumping (see Game::update).
    const float barWidth = 780.f;

    drawProgressBar({{MARGIN, 112.f}, {barWidth, 8.f}}, m_quizBarShown, m_theme.accent);
    drawProgressBar({{MARGIN, 150.f}, {barWidth, 12.f}}, m_timeBarShown, timerColor);

    // ---- the question ----------------------------------------------------
    // Long questions are broken into several lines so they always fit.
    const std::vector<std::string> lines = wrapText(question.question, 24, 980.f);
    float lineY = 190.f;
    for (const std::string& line : lines)
    {
        m_window.draw(makeTextCentred(line, 24, m_theme.text, DESIGN_WIDTH / 2.f, lineY));
        lineY += 32.f;
    }

    // ---- the middle line --------------------------------------------------
    // While a question is open this shows the streak. After answering it
    // shows whether the answer was right - and it pops in with a small scale.
    if (asking)
    {
        if (m_quiz.streak() >= 2)
        {
            m_window.draw(makeTextCentred(std::to_string(m_quiz.streak()) +
                                          " question streak!", 26,
                                          m_theme.warning, DESIGN_WIDTH / 2.f, 320.f));
        }
    }
    else
    {
        std::string headline;
        sf::Color color = m_theme.success;

        if (m_quiz.lastAnswerTimedOut())
        {
            headline = "Time Up!";
            color = m_theme.warning;
        }
        else if (m_quiz.lastAnswerCorrect())
        {
            headline = "Correct!";
            color = m_theme.success;
            if (m_quiz.streak() >= 2)
                headline += "   " + std::to_string(m_quiz.streak()) + " question streak!";
        }
        else
        {
            headline = "Wrong!";
            color = m_theme.danger;
        }

        // A quick pop: the text starts a little too small and springs to its
        // real size in the first quarter of a second. (1-t)^3 makes it fast
        // at the start and slow at the end, which is what a pop looks like.
        const float pop = std::clamp(m_feedbackTimer / 0.22f, 0.f, 1.f);
        const auto grow = static_cast<unsigned int>(24.f + 6.f * (1.f - (1.f - pop) * (1.f - pop) * (1.f - pop)));

        m_window.draw(makeTextCentred(headline, grow, color, DESIGN_WIDTH / 2.f, 318.f));

        if (!m_quiz.lastAnswerCorrect())
        {
            m_window.draw(makeTextCentred("The correct answer is " +
                                          std::string(1, question.correctAnswer) + ".",
                                          18, m_theme.text, DESIGN_WIDTH / 2.f, 352.f));
        }

        if (!question.explanation.empty())
            m_window.draw(makeTextCentred(question.explanation, 15, m_theme.textMuted,
                                          DESIGN_WIDTH / 2.f, 376.f));
    }

    // ---- the four answers ------------------------------------------------
    const char letters[4] = {'A', 'B', 'C', 'D'};

    for (int i = 0; i < 4; ++i)
    {
        // After answering, the right option gets a green frame and the wrong
        // one the player chose gets a red frame.
        m_optionButtons[i].clearHighlight();
        if (!asking)
        {
            if (letters[i] == question.correctAnswer)
                m_optionButtons[i].setHighlight(m_theme.success);
            else if (letters[i] == m_quiz.chosenAnswer())
                m_optionButtons[i].setHighlight(m_theme.danger);
        }
        m_optionButtons[i].draw(m_window, m_font, m_theme);

        // A small key cap on the right showing the letter, so it is obvious
        // that A, B, C and D on the keyboard work as well.
        const sf::FloatRect bounds = m_optionButtons[i].bounds();
        const sf::FloatRect cap{{bounds.position.x + bounds.size.x - 46.f,
                                 bounds.position.y + (bounds.size.y - 30.f) / 2.f},
                                {30.f, 30.f}};
        const bool lit = m_optionButtons[i].isHovered();

        sf::RectangleShape keyCap(cap.size);
        keyCap.setPosition(cap.position);
        keyCap.setFillColor(lit ? mixColors(m_theme.accent, sf::Color::White, 0.25f) : m_theme.button);
        keyCap.setOutlineThickness(1.f);
        keyCap.setOutlineColor(lit ? m_theme.accent : m_theme.border);
        m_window.draw(keyCap);

        m_window.draw(makeTextCentred(std::string(1, letters[i]), 15,
                                      lit ? sf::Color::White : m_theme.textMuted,
                                      cap.position.x + cap.size.x / 2.f,
                                      cap.position.y + cap.size.y / 2.f));
    }


    drawBackButton();
    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, 18.f}, {150.f, 46.f}});
    drawHomeButton();
    m_nextButton.setBounds({{DESIGN_WIDTH - MARGIN - 200.f, FOOTER_Y}, {200.f, 46.f}});
    m_nextButton.draw(m_window, m_font, m_theme);

    if (m_feedbackTimer < FEEDBACK_DELAY && !asking)
    {
        // A shrinking line above NEXT, showing how long until it can be used.
        const float remaining = 1.f - m_feedbackTimer / FEEDBACK_DELAY;
        drawProgressBar({{DESIGN_WIDTH - MARGIN - 200.f, 632.f}, {200.f, 4.f}},
                        remaining, m_theme.textMuted);
    }
}
void Game::showResultScreen()
{
    drawBackground();
    drawHeader("QUIZ COMPLETED!", "Here is how it went");

    m_window.draw(makeTextCentred("Player: " + m_playerName, 30, m_theme.text,
                                  DESIGN_WIDTH / 2.f, 176.f));

    // ---- the score, counting up ------------------------------------------
    // The number pops in a little as it grows, which makes the moment feel
    // like a moment instead of a line of text.
    const int shown = static_cast<int>(m_displayScore);
    const bool perfect = (m_quiz.score() == m_quiz.maximumScore());
    const float grow = m_animationsOn
                     ? 1.f + 0.10f * (1.f - std::clamp(m_displayScore /
                                                     static_cast<float>(std::max(1, m_quiz.score())), 0.f, 1.f))
                     : 1.f;

    m_window.draw(makeTextCentred("SCORE", 15, m_theme.textMuted,
                                  DESIGN_WIDTH / 2.f, 224.f));
    m_window.draw(makeTextCentred(std::to_string(shown),
                                  static_cast<unsigned int>(72.f * grow),
                                  perfect ? m_theme.success : m_theme.accent,
                                  DESIGN_WIDTH / 2.f, 292.f));
    m_window.draw(makeTextCentred("out of " + std::to_string(m_quiz.maximumScore()),
                                  20, m_theme.textMuted, DESIGN_WIDTH / 2.f, 356.f));

    // ---- four small statistic cards --------------------------------------
    const struct { const char* label; std::string value; sf::Color color; int count; } stats[4] = {
        {"CORRECT",    std::to_string(m_quiz.correctCount()),    m_theme.success,  m_quiz.correctCount()},
        {"WRONG",      std::to_string(m_quiz.wrongCount()),      m_theme.danger,   m_quiz.wrongCount()},
        {"UNANSWERED", std::to_string(m_quiz.unansweredCount()), m_theme.textMuted, m_quiz.unansweredCount()},
        {"ACCURACY",   std::to_string(static_cast<int>(m_quiz.accuracy())) + "%",
                       m_theme.accent, static_cast<int>(m_quiz.accuracy())}
    };

    const float cardsWidth = 250.f;
    for (int i = 0; i < 4; ++i)
    {
        const sf::FloatRect card{{80.f + i * 290.f, 396.f}, {cardsWidth, 104.f}};
        drawCard(card);

        m_window.draw(makeTextCentred(stats[i].label, 14, m_theme.textMuted,
                                      card.position.x + card.size.x / 2.f, 410.f));
        m_window.draw(makeTextCentred(stats[i].value, 32, stats[i].color,
                                      card.position.x + card.size.x / 2.f, 432.f));

        // A small bar showing the share of the quiz, which grows from nothing
        // when the screen is entered. The accuracy bar is 0..100 instead of
        // 0..questionCount, so it gets its own scale.
        const float scale = (i == 3) ? 100.f
                                      : static_cast<float>(std::max<std::size_t>(1, m_quiz.questionCount()));
        const float fraction = std::clamp(static_cast<float>(stats[i].count) / scale, 0.f, 1.f) *
                               m_statBars;

        sf::RectangleShape track({card.size.x - 40.f, 5.f});
        track.setPosition({card.position.x + 20.f, card.position.y + card.size.y - 20.f});
        track.setFillColor(m_theme.button);
        m_window.draw(track);

        sf::RectangleShape fill({(card.size.x - 40.f) * fraction, 5.f});
        fill.setPosition(track.getPosition());
        fill.setFillColor(stats[i].color);
        m_window.draw(fill);
    }

    // ---- the best streak, with a little flame-ish badge -------------------
    const std::string streakText = "Best streak: " + std::to_string(m_quiz.bestStreak());
    m_window.draw(makeTextCentred(streakText, 22, m_theme.text, DESIGN_WIDTH / 2.f, 522.f));

    m_againButton.draw(m_window, m_font, m_theme);
    m_againHighScoreButton.draw(m_window, m_font, m_theme);
    m_homeButton.draw(m_window, m_font, m_theme);
    m_exitResultButton.draw(m_window, m_font, m_theme);

    // The footer tells the truth: a failed write is never reported as a save.
    m_window.draw(makeTextRight(
        m_scoreSaved ? "Saved to data/scores.txt" : "Could not write scores.txt",
        14, m_scoreSaved ? m_theme.textMuted : m_theme.danger,
        DESIGN_WIDTH - MARGIN, 668.f));
}


void Game::showHighScores()
{
    drawBackground();
    drawHeader("High Scores", "The best " + std::to_string(MAX_HIGH_SCORES) +
                              " results are kept in data/scores.txt");

    for (int i = 0; i < 5; ++i)
    {
        if (m_filterButtons[i].visible())
            m_filterButtons[i].draw(m_window, m_font, m_theme);
    }

    // The table starts at y = 222 so it stays clear of the notification strip.
    const sf::FloatRect table{{MARGIN, 222.f}, {DESIGN_WIDTH - MARGIN * 2.f, 375.f}};
    drawCard(table);

    m_window.draw(makeTextRight("#", 15, m_theme.textMuted, 150.f, 228.f));
    m_window.draw(makeText("PLAYER", 15, m_theme.textMuted, 180.f, 228.f));
    m_window.draw(makeText("SCORE", 15, m_theme.textMuted, 620.f, 228.f));
    m_window.draw(makeText("CATEGORY", 15, m_theme.textMuted, 760.f, 228.f));
    m_window.draw(makeText("LEVEL", 15, m_theme.textMuted, 940.f, 228.f));
    m_window.draw(makeText("DATE", 15, m_theme.textMuted, 1080.f, 228.f));

    // Only the rows that pass the filter are shown. An empty filter string
    // means "show everything", which is why this one condition does it all.
    int row = 0;
    for (const ScoreEntry& entry : m_highScores)
    {
        if (!m_highScoreFilter.empty() && entry.category != m_highScoreFilter)
            continue;

        if (row >= MAX_HIGH_SCORES)
            break;

        const float y = 262.f + row * 33.f;

        // The best three results get a gold / silver / bronze number.
        sf::Color rankColor = m_theme.text;
        if (row == 0)      rankColor = m_theme.warning;
        else if (row == 1) rankColor = sf::Color(198, 203, 216);
        else if (row == 2) rankColor = sf::Color(198, 132, 74);

        m_window.draw(makeTextRight(std::to_string(row + 1), 16, rankColor, 150.f, y));
        m_window.draw(makeText(entry.player, 16, m_theme.text, 180.f, y));
        m_window.draw(makeText(std::to_string(entry.score) + " / " + std::to_string(entry.maxScore),
                               16, m_theme.text, 620.f, y));
        m_window.draw(makeText(entry.category, 16, m_theme.textMuted, 760.f, y));
        m_window.draw(makeText(entry.difficulty, 16, m_theme.textMuted, 940.f, y));
        m_window.draw(makeText(entry.date, 16, m_theme.textMuted, 1080.f, y));

        ++row;
    }

    if (row == 0)
    {
        m_window.draw(makeTextCentred("No scores yet.", 26, m_theme.text,
                                      DESIGN_WIDTH / 2.f, 320.f));
        m_window.draw(makeTextCentred("Finish a quiz and your name will appear here.",
                                      17, m_theme.textMuted, DESIGN_WIDTH / 2.f, 356.f));
    }

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Click a filter to see only one category.");
}

void Game::showSettings()
{
    drawBackground();
    drawHeader("Settings", "Click a row to change it. Your choices are saved automatically.");

    m_soundButton.draw(m_window, m_font, m_theme);
    m_animationsButton.draw(m_window, m_font, m_theme);
    m_themeButton.draw(m_window, m_font, m_theme);

    // A few honest notes, so the player knows why something may be missing.
    const std::vector<std::string> notes = m_audio.hasAnySound()
        ? std::vector<std::string>{
              "Sound files found in assets/sounds.",
              "Wanted: click.wav, correct.wav, wrong.wav (any short .wav works)."}
        : std::vector<std::string>{
              "No sound files were found, so the game runs silently.",
              "To add sounds, copy click.wav, correct.wav and wrong.wav into assets/sounds."};

    drawParagraph(notes, 16, m_theme.textMuted, MARGIN, 500.f, 24.f);
    drawParagraph({"Settings are stored in data/settings.txt"},
                  16, m_theme.textMuted, MARGIN, 560.f, 24.f);

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Esc goes back to the menu.");
}

void Game::showAbout()
{
    drawBackground();
    drawHeader("QUIZMASTER", "Test Your Knowledge");

    // The card is 800 wide so its right edge stops before the notification strip.
    drawCard({{160.f, 180.f}, {800.f, 430.f}});

    const std::vector<std::string> lines = {
        "A simple C++ desktop quiz application, written as a college project.",
        "",
        "Built with",
        "    C++17      the game logic, the questions and the score",
        "    SFML 3     the window, the text, the mouse and the sounds",
        "    CMake      building the project on Windows",
        "",
        "Features",
        "    " + std::to_string(availableCategories().size()) + " categories and " +
            std::to_string(questionBank().size()) + " questions in total",
        "    " + std::to_string(3) + " difficulties - each one has a shorter timer and a longer quiz",
        "    countdown timer, score, accuracy and answer streaks",
        "    correct / wrong feedback with a short explanation",
        "    a high score table stored in data/scores.txt",
        "    dark and light theme, sound and animation switches"
    };


    drawParagraph(lines, 17, m_theme.text, 200.f, 205.f, 25.f);

    m_homeScreenButton.setBounds({{DESIGN_WIDTH - MARGIN - 150.f, FOOTER_Y}, {150.f, 46.f}});
    drawBackButton();
    drawHomeButton();
    drawFooterHint("Thanks for playing!");
}
