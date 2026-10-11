#pragma once

#include "Theme.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <utility>

/// \file Button.h
/// A clickable rectangle with a label, used for every menu item and every
/// answer option.
///
/// Why not draw buttons directly in the Game class?
///   Because then the hover effect would have to be written again for every
///   button. Putting the behaviour in one class means:
///     * one place to change how a button looks,
///     * one place to fix a bug in the click handling,
///     * the Game code stays short:
///           button.update(mouse, m_leftDown, m_delta);
///           if (button.wasClicked()) { ... }
///
/// How a click is detected (the important part):
///   A click is NOT "the mouse is on the button". That would fire the action
///   every single frame while the mouse merely rests there. A real click is:
///       1. the left button goes down while the mouse is on the button, and
///       2. the left button comes back up while the mouse is still on it.
///   `update()` remembers step 1, and checks step 2. `wasClicked()` then
///   reports it for exactly one frame.

/// How a button is coloured.
enum class ButtonStyle
{
    Neutral,  ///< Regular menu button.
    Accent,   ///< The main action of the screen, filled with the accent colour.
    Danger    ///< For destructive actions.
};

class Button
{
public:
    Button() = default;
    Button(sf::FloatRect bounds, std::string label,
           ButtonStyle style = ButtonStyle::Neutral);

    // ---- setup (usually called once, right after construction) ------------
    void setBounds(sf::FloatRect bounds) { m_bounds = bounds; }
    const sf::FloatRect& bounds() const { return m_bounds; }

    void setLabel(std::string label) { m_label = std::move(label); }
    const std::string& label() const { return m_label; }

    /// Small second line under the label, e.g. "30 seconds per question".
    void setSubtitle(std::string subtitle) { m_subtitle = std::move(subtitle); }
    const std::string& subtitle() const { return m_subtitle; }

    void setStyle(ButtonStyle style) { m_style = style; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool enabled() const { return m_enabled; }

    /// A hidden button is neither drawn nor clickable. Used to hide the BACK
    /// button while the answer feedback is showing.
    void setVisible(bool visible) { m_visible = visible; }
    bool visible() const { return m_visible; }

    /// Draw a coloured frame around the button, e.g. green for the correct
    /// answer and red for the wrong one. Pass no colour (default) to remove it.
    void setHighlight(const sf::Color& color) { m_highlight = color; }
    void clearHighlight() { m_highlight = sf::Color::Transparent; }

    /// Turn the hover and press animations on or off (the Settings screen).
    void setAnimationsEnabled(bool enabled) { m_animationsEnabled = enabled; }

    /// A glow behind the button, used for the main action of a screen.
    /// Pass sf::Color::Transparent to switch the glow off.
    void setGlow(const sf::Color& color) { m_glow = color; }

    // ---- per frame --------------------------------------------------------
    /// Update the animations and look at the mouse.
    /// \param mouse     where the mouse is
    /// \param leftDown  true while the left mouse button is held down
    /// \param deltaSeconds seconds since the previous frame
    void update(const sf::Vector2f& mouse, bool leftDown, float deltaSeconds);

    /// \return true on the single frame the player completes a click here.
    bool wasClicked() const { return m_clicked; }

    /// Forget a press that started but never finished. The Game calls this
    /// when it changes screen, so releasing the mouse over a button on the new
    /// screen cannot count as a click on the old one.
    void releasePress() { m_pressStarted = false; m_clicked = false; m_pressed = false; }

    void draw(sf::RenderTarget& target, const sf::Font& font, const Theme& theme) const;

    // ---- for the game logic -----------------------------------------------
    bool isHovered() const { return m_hovered; }
    /// The hover strength, 0..1. The Game can use it for extra effects.
    float hoverAmount() const { return m_hover; }

private:
    /// Blend between two colours, `amount` 0..1.
    static sf::Color mix(const sf::Color& from, const sf::Color& to, float amount);
    /// The colour a disabled button should use.
    bool clickable() const { return m_enabled && m_visible; }

    sf::FloatRect m_bounds;
    std::string m_label;
    std::string m_subtitle;
    ButtonStyle m_style{ButtonStyle::Neutral};
    sf::Color m_glow{sf::Color::Transparent};

    bool m_enabled{true};
    bool m_visible{true};
    bool m_hovered{false};
    bool m_pressed{false};

    /// True while the left button is down and this button was already under the
    /// mouse, i.e. the click has "started" here.
    bool m_pressStarted{false};
    /// Set for exactly one frame when a click finishes on this button.
    bool m_clicked{false};

    /// Smoothed values (0..1) that the drawing code uses, so the button eases
    /// into its new look instead of snapping.
    float m_hover{0.f};
    float m_press{0.f};
    /// The shine that slides across the button while the mouse is on it.
    float m_shine{0.f};
    /// The circle that spreads out from the click point. 1 = finished.
    float m_ripple{1.f};
    sf::Vector2f m_rippleCentre{0.f, 0.f};

    bool m_animationsEnabled{true};

    /// Optional coloured frame (green = correct, red = wrong).
    sf::Color m_highlight{sf::Color::Transparent};
};
