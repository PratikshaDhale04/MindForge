#pragma once

#include "Theme.h"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <string>
#include <vector>

/// \file Notification.h
/// The little messages that slide in from the top right corner: "Score saved",
/// "Only 3 questions available", "Sound files missing" and so on.
///
/// Why a class for this?
///   Because a toast has three jobs that always have to happen together:
///     * remember what the message is,
///     * count how long it stays on screen,
///     * draw it, and remove it when its time is up.
///   Doing that in the Game class would mean a second list of variables plus
///   a second drawing function, and the two would easily get out of step.
///
/// A notification is a `struct` because it only holds data - the manager
/// below is the class that does the work.
///
/// IMPORTANT - where these are drawn
///   The messages live in a fixed strip on the right hand side of the screen
///   (see kLeft / kTop below). No screen is allowed to put a button inside
///   that strip, so a message can never cover a button. The layout numbers in
///   Game.cpp are chosen around this: every button either ends before x = 990
///   or starts below y = 245.

/// How important the message is. This picks the colour and the symbol.
enum class NotificationKind
{
    Info,     ///< Something the player should know. Indigo.
    Success,  ///< Something worked. Green.
    Warning,  ///< Something is unusual but the game carries on. Amber.
    Error     ///< Something did not work. Red.
};

/// One message.
///
/// `age` counts up and `life` counts down from `duration` to 0. The game needs
/// both: the age drives the slide-in, the life drives the fade-out.
struct Notification
{
    std::string title;
    std::string message;
    NotificationKind kind{NotificationKind::Info};
    float duration{3.2f};
    float life{0.f};
    float age{0.f};
};

class Notifications
{
public:
    /// Show a message for `seconds`. If more than kMaxVisible are already on
    /// screen the oldest one is dropped, so the list can never grow forever.
    void add(NotificationKind kind, const std::string& title,
             const std::string& message, float seconds = 3.2f);

    /// Count every message down and throw away the ones that reached zero.
    void update(float deltaSeconds);

    /// Draw the messages, newest at the top, sliding in from the right.
    void draw(sf::RenderTarget& target, const sf::Font& font, const Theme& theme) const;

    /// Remove everything at once (used when the theme changes).
    void clear() { m_items.clear(); }

    /// How many messages are currently on screen.
    std::size_t count() const { return m_items.size(); }

    /// The right hand strip the messages use. Screen layouts keep clear of it.
    static constexpr float kLeft  = 992.f;
    static constexpr float kTop   = 20.f;
    static constexpr float kWidth = 264.f;
    static constexpr float kHeight = 68.f;
    static constexpr float kGap    = 8.f;

private:
    static constexpr int kMaxVisible = 3;   ///< How many fit on screen.
    static constexpr float kSlideIn   = 0.22f;  ///< Seconds the slide-in takes.
    static constexpr float kFadeOut   = 0.50f;  ///< Seconds the fade-out takes.
    static constexpr float kScreenW   = 1280.f; ///< The design width of the game.

    /// The colour and the short symbol used for each kind.
    static sf::Color colorFor(NotificationKind kind, const Theme& theme);
    static const char* symbolFor(NotificationKind kind);

    /// Split a long message into lines that fit inside `maxWidth`.
    static std::vector<std::string> wrap(const sf::Font& font, const std::string& text,
                                         unsigned int size, float maxWidth);

    /// Ease a value from 0 to 1, used for the slide-in.
    static float easeOut(float t);

    std::vector<Notification> m_items;
};
