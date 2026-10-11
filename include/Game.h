#pragma once

#include "Audio.h"
#include "Button.h"
#include "Notification.h"
#include "Quiz.h"
#include "Theme.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include <string>
#include <vector>

/// \file Game.h
/// Owns the window, keeps track of which screen is showing, and does all the
/// drawing and input handling.
///
/// The screens are *not* separate classes on purpose. For a project this size
/// that would add a lot of files without making anything easier, so every
/// screen is one function here:
///
///     showHome()  showNameScreen()  showCategoryScreen()  ...
///
/// The rules live in Quiz, and this class decides how to show them.
///
/// How the flow works:
///     Game::run()  ->  handleEvents()  ->  update(dt)  ->  draw()
///     goTo(Screen::Quiz) puts the Quiz screen on top; the screen's own
///     update function then calls goTo(Screen::Result) when the quiz is over.

/// The list of screens. A simple `enum class` is all that is needed here.
enum class Screen
{
    Home,
    Name,
    Category,
    Difficulty,
    Quiz,
    Result,
    HighScores,
    Settings,
    About
};

/// How long a notification stays on screen unless the caller says otherwise.
inline constexpr float TOAST_SECONDS = 3.2f;

/// One line of the high score table.
struct ScoreEntry
{
    std::string player;
    int score{0};
    int maxScore{0};
    std::string category;
    std::string difficulty;
    std::string date;
};

/// A small dot that drifts slowly upwards in the background. Only data, so a
/// plain struct is enough.
struct Spark
{
    sf::Vector2f position{0.f, 0.f};
    float speed{10.f};
    float radius{2.f};
    float phase{0.f};
};

class Game
{
public:
    /// \param programFolder the folder the .exe lives in. The game uses it to
    ///        find its "data" and "assets" folders, so the answer is the same
    ///        no matter which folder the game was started from.
    explicit Game(std::string programFolder = std::string());

    /// Create the window, load the font, the settings and the sounds, then run
    /// until the window is closed. \return the process exit code.
    int run();

private:
    // ---- setup ------------------------------------------------------------
    bool createWindow();
    bool loadFont();
    void loadSettings();
    void saveSettings();
    /// Work out which folder holds data/ and assets/ (works from build/bin).
    void findFolders();
    void loadHighScores();
    /// Write the table back to disk. \return true when the file was written.
    bool saveHighScores();

    // ---- the game loop ----------------------------------------------------
    void handleEvents();
    void update(float deltaSeconds);
    void draw();

    sf::Vector2f mousePosition() const;

    /// Switch to another screen and restart the slide / fade animation.
    void goTo(Screen screen);

    /// Draw the screen that is currently active.
    void drawScreen(Screen screen);

    /// Move the drawing view sideways, which is the slide part of a transition.
    void setViewShift(float x);

    /// Feed the mouse position and the frame time to one button.
    /// Every screen does this before asking a button whether it was clicked,
    /// which is why it is worth having in one place.
    void updateButton(Button& button, const sf::Vector2f& mouse);

    /// Show a message in the top right corner. This is the only way the rest
    /// of the code adds a notification, so every message looks the same.
    void notify(NotificationKind kind, const std::string& title,
                const std::string& message, float seconds = TOAST_SECONDS);

    /// Add a finished quiz to the high score table, keep the best 10 and save.
    void recordHighScore();

    /// Left-align several lines of text starting at (x, y) with a fixed line
    /// height. Used by the About screen and the small notes on Settings.
    void drawParagraph(const std::vector<std::string>& lines, unsigned int size,
                       const sf::Color& color, float x, float y, float lineHeight);

    // ---- one draw function per screen ------------------------------------
    void showHome();
    void showNameScreen();
    void showCategoryScreen();
    void showDifficultyScreen();
    void showQuizScreen();
    void showResultScreen();
    void showHighScores();
    void showSettings();
    void showAbout();

    // ---- one update function per screen (input + animation) --------------
    void updateHome();
    void updateName();
    void updateCategory();
    void updateDifficulty();
    void updateQuiz();
    void updateResult();
    void updateHighScores();
    void updateSettings();
    void updateAbout();

    // ---- shared drawing helpers -------------------------------------------
    void drawBackground();
    void drawHeader(const std::string& title, const std::string& subtitle);
    void drawFooterHint(const std::string& text);
    void drawCard(const sf::FloatRect& area);
    void drawProgressBar(const sf::FloatRect& area, float fraction, const sf::Color& color);
    void drawBackButton();
    void drawHomeButton();
    void drawSparkles();

    /// A title with a soft coloured halo behind it (used on the Home screen).
    void drawGlowTitle(const std::string& text, unsigned int size, float y, float pulse);

    sf::Text makeText(const std::string& text, unsigned int size, const sf::Color& color,
                      float x, float y) const;
    sf::Text makeTextCentred(const std::string& text, unsigned int size, const sf::Color& color,
                             float x, float y) const;

    /// Place text so that its *right* edge is at `rightX`.
    /// Used for the values that line up in a column, like the timer and the
    /// high score table.
    sf::Text makeTextRight(const std::string& text, unsigned int size, const sf::Color& color,
                          float rightX, float y) const;

    /// Break a long string into lines that fit inside `maxWidth`.
    std::vector<std::string> wrapText(const std::string& text, unsigned int size,
                                      float maxWidth) const;

    // ---- data -------------------------------------------------------------
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_fontLoaded{false};

    Quiz m_quiz;
    Audio m_audio;
    Theme m_theme;
    Notifications m_notifications;

    Screen m_screen{Screen::Home};

    /// The screen that is being left. It is drawn on top of the new one and
    /// fades away, which is what makes the change look like a real transition
    /// instead of a jump cut.
    Screen m_previousScreen{Screen::Home};
    float m_transition{1.f};          ///< 0 = old screen on top, 1 = finished.

    /// Seconds since the last frame. Stored so every update function can use it.
    float m_delta{0.f};

    /// True while the left mouse button is held down. handleEvents() keeps this
    /// up to date and every Button needs it to tell a real click from a hover.
    bool m_leftDown{false};

    /// True from the moment the mouse is pressed until the end of that frame.
    /// A click that starts and ends between two frames would otherwise be lost,
    /// because update() would only ever see the mouse in its "up" state.
    bool m_pressLatch{false};

    // The player's choices, filled in as they move through the menus.
    std::string m_playerName;
    std::string m_selectedCategory;
    std::string m_selectedDifficulty;

    // Text the player is typing on the name screen.
    std::string m_nameInput;
    std::string m_message;          ///< Short hint, e.g. "Please enter your name."

    /// Set by handleEvents() when the player presses A/B/C/D, and by Enter on
    /// the feedback screen. updateQuiz() picks them up and does the work, so
    /// the keyboard and the mouse go through exactly the same code path.
    char m_pendingAnswer{'\0'};
    bool m_pendingNext{false};

    // High score table (read from and written to data/scores.txt).
    std::vector<ScoreEntry> m_highScores;
    std::string m_highScoreFilter;  ///< Empty string means "show every category".

    // Settings (read from and written to data/settings.txt).
    bool m_soundOn{true};
    bool m_animationsOn{true};
    bool m_lightTheme{true};      ///< Light is the look the game starts with.

    // Animation state.
    float m_fade{1.f};               ///< 0 -> 1 fade-in when a screen appears.
    float m_feedbackTimer{0.f};      ///< Seconds the answer feedback stays up.
    float m_displayScore{0.f};       ///< Counts up to the real score on the result screen.
    float m_timerFlash{0.f};         ///< Pulses while the timer is nearly over.
    float m_homeAnim{0.f};           ///< Slow drift used for the home title.
    float m_caretTimer{0.f};         ///< Makes the "_" in the name box blink.
    float m_timeBarShown{1.f};       ///< Smoothed width of the timer bar.
    float m_quizBarShown{0.f};       ///< Smoothed width of the question bar.
    float m_statBars{0.f};           ///< Grows from 0 to 1 to animate the result bars.
    bool m_timeUpShown{false};       ///< Stops the "Time up" message repeating.

    /// Drifting dots in the background. They are created once and then simply
    /// move, so the background is never completely still.
    std::vector<Spark> m_sparks;

    /// True once a score was written to disk, so the result screen can say so.
    bool m_scoreSaved{false};

    // Paths.
    std::string m_dataFolder;
    std::string m_assetsFolder;

    /// The folder the .exe lives in, handed over by main().
    std::string m_programFolder;

    // Buttons are members, not locals, so they keep their animation values
    // between frames while the mouse moves over them.
    Button m_startButton, m_highScoreButton, m_settingsButton, m_aboutButton, m_exitButton;
    Button m_continueButton, m_backButton, m_homeScreenButton, m_nextButton;
    Button m_againButton, m_homeButton, m_againHighScoreButton, m_exitResultButton;
    Button m_optionButtons[4];
    Button m_soundButton, m_animationsButton, m_themeButton;
    Button m_categoryButtons[4];
    Button m_difficultyButtons[3];
    Button m_filterButtons[5];

    /// How many category buttons the bank actually provides (max 4).
    int m_categoryCount{0};
};
