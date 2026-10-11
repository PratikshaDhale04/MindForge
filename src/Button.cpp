#include "Button.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cstdint>

namespace
{
constexpr float HOVER_SPEED = 12.f;   // How fast the hover animation runs.
constexpr float PRESS_SPEED = 22.f;   // Pressing should feel quicker.
constexpr float SHINE_SPEED = 3.2f;   // How often the shine sweeps across.
constexpr float RIPPLE_TIME = 0.45f;  // How long the click ripple lasts.
} // namespace

sf::Color Button::mix(const sf::Color& from, const sf::Color& to, float amount)
{
    if (amount <= 0.f)
        return from;
    if (amount >= 1.f)
        return to;

    // Colour components are unsigned bytes, so every step needs a cast.
    const auto blend = [amount](std::uint8_t a, std::uint8_t b) {
        return static_cast<std::uint8_t>(static_cast<float>(a) +
                                         (static_cast<float>(b) - static_cast<float>(a)) * amount);
    };

    return sf::Color(blend(from.r, to.r), blend(from.g, to.g), blend(from.b, to.b),
                     blend(from.a, to.a));
}

Button::Button(sf::FloatRect bounds, std::string label, ButtonStyle style)
    : m_bounds(bounds)
    , m_label(std::move(label))
    , m_style(style)
{
}

void Button::update(const sf::Vector2f& mouse, bool leftDown, float deltaSeconds)
{
    m_hovered = clickable() && m_bounds.contains(mouse);
    m_pressed = m_hovered && leftDown;

    // --- the two halves of a click ---------------------------------------
    // 1. The press has to START on this button ...
    if (m_pressed)
        m_pressStarted = true;

    // 2. ... and the release has to happen on it as well. This is the frame
    //    the action should run on.
    m_clicked = m_pressStarted && m_hovered && !leftDown;

    if (m_clicked)
    {
        m_pressStarted = false;      // a click can only fire once
        m_ripple       = 0.f;        // start the ripple at the click point
        m_rippleCentre = mouse;
    }
    else if (m_pressStarted && !leftDown)
    {
        // The press started on this button but ended somewhere else - the
        // player dragged away, or let go off the window. Forget it, otherwise
        // moving the mouse back over the button would count as a click.
        m_pressStarted = false;
    }

    // Move the animation a little towards its target. This "exponential
    // smoothing" is the whole trick behind every smooth UI in the game.
    const float hoverTarget   = m_hovered ? 1.f : 0.f;
    const float pressedTarget = m_pressed ? 1.f : 0.f;

    if (!m_animationsEnabled)
    {
        m_hover = hoverTarget;
        m_press = pressedTarget;
        m_shine = hoverTarget;
        if (m_ripple < 1.f)
            m_ripple = 1.f;
        return;
    }

    // clamp() keeps the factor inside 0..1 even if the frame was very long.
    const float hoverFactor  = std::clamp(HOVER_SPEED  * deltaSeconds, 0.f, 1.f);
    const float pressedFactor = std::clamp(PRESS_SPEED * deltaSeconds, 0.f, 1.f);

    m_hover += (hoverTarget   - m_hover)  * hoverFactor;
    m_press += (pressedTarget - m_press) * pressedFactor;

    // The shine sweeps left to right and then jumps back to the start. Because
    // it is a plain counter, the button is busy but nothing needs a timer.
    m_shine += deltaSeconds * SHINE_SPEED;
    if (m_shine > 1.f)
        m_shine -= 1.f;

    // The ripple grows until it covers the whole button, then stops.
    if (m_ripple < 1.f)
        m_ripple = std::min(1.f, m_ripple + deltaSeconds / RIPPLE_TIME);
}

void Button::draw(sf::RenderTarget& target, const sf::Font& font, const Theme& theme) const
{
    if (!m_visible)
        return;

    // The press animation shrinks the button slightly, which is what makes a
    // click feel physical.
    const float shrink = m_press * 3.f;
    const sf::FloatRect box{{m_bounds.position.x + shrink, m_bounds.position.y + shrink},
                            {m_bounds.size.x - shrink * 2.f, m_bounds.size.y - shrink * 2.f}};

    // Pick the two colours to blend between.
    sf::Color idleColor   = theme.button;
    sf::Color activeColor = theme.buttonHover;
    sf::Color textColor   = theme.text;

    if (m_style == ButtonStyle::Accent)
    {
        idleColor   = theme.accent;
        activeColor = mix(theme.accent, sf::Color::White, 0.18f);
        textColor   = sf::Color::White;
    }
    else if (m_style == ButtonStyle::Danger)
    {
        idleColor   = theme.danger;
        activeColor = mix(theme.danger, sf::Color::Black, 0.15f);
        textColor   = sf::Color::White;
    }

    const sf::Color fill = m_enabled ? mix(idleColor, activeColor, m_hover) : theme.surface;
    if (!m_enabled)
        textColor = theme.textMuted;

    // --- the glow behind an accent button ---------------------------------
    // Three rectangles, each a bit bigger and more transparent than the last,
    // which looks like a soft halo without needing a texture file.
    if (m_glow != sf::Color::Transparent && m_animationsEnabled)
    {
        for (int i = 3; i >= 1; --i)
        {
            const float grow = static_cast<float>(i) * 4.f;
            sf::RectangleShape halo({box.size.x + grow * 2.f, box.size.y + grow * 2.f});
            halo.setPosition(box.position - sf::Vector2f(grow, grow));
            halo.setFillColor(sf::Color::Transparent);
            halo.setOutlineThickness(2.f);
            halo.setOutlineColor(sf::Color(m_glow.r, m_glow.g, m_glow.b,
                                           static_cast<std::uint8_t>(28.f / static_cast<float>(i))));
            target.draw(halo);
        }
    }

    // --- the background rectangle ---
    sf::RectangleShape shape(box.size);
    shape.setPosition(box.position);
    shape.setFillColor(fill);
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(m_style == ButtonStyle::Neutral ? theme.border
                                                          : mix(fill, sf::Color::White, 0.25f));
    target.draw(shape);

    // --- the shine that sweeps across while the mouse rests on the button --
    if (m_animationsEnabled && m_hover > 0.05f)
    {
        // A narrow vertical band travelling from left to right.
        const float bandWidth = box.size.x * 0.28f;
        const float travel    = box.size.x + bandWidth;
        const float x = box.position.x - bandWidth + travel * m_shine;

        sf::RectangleShape band(sf::Vector2f(bandWidth, box.size.y));
        band.setPosition({x, box.position.y});
        band.setFillColor(sf::Color(255, 255, 255,
                                    static_cast<std::uint8_t>(26.f * m_hover)));
        target.draw(band);
    }

    // --- the ripple that spreads out from the click point -------------------
    if (m_animationsEnabled && m_ripple < 1.f)
    {
        // The circle starts small and grows to cover the whole button.
        const float maxRadius = std::max(box.size.x, box.size.y);
        const float radius = maxRadius * m_ripple;
        const float fade = 1.f - m_ripple;

        sf::CircleShape ripple(radius);
        ripple.setOrigin({radius, radius});
        ripple.setPosition(m_rippleCentre);
        ripple.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(70.f * fade)));
        target.draw(ripple);
    }

    // --- a thin accent strip on the left of an accent button ---
    if (m_style == ButtonStyle::Accent)
    {
        sf::RectangleShape strip(sf::Vector2f(4.f, box.size.y - 14.f));
        strip.setPosition({box.position.x + 12.f, box.position.y + 7.f});
        strip.setFillColor(sf::Color(255, 255, 255, 130));
        target.draw(strip);
    }

    // --- the label, centred in the rectangle ---
    // In SFML 3 a sf::Text has to be created with its font straight away,
    // because there is no "empty" text object any more.
    sf::Text text(font, m_label);
    // A button with a subtitle uses a slightly smaller label so both lines fit.
    const bool hasSubtitle = !m_subtitle.empty();
    text.setCharacterSize(static_cast<unsigned int>(box.size.y * (hasSubtitle ? 0.26f : 0.32f)));
    text.setFillColor(textColor);

    // setOrigin moves the text so that the given point becomes (0,0). Using the
    // centre of the text lets us place it by its middle instead of guessing.
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.getCenter());
    text.setPosition({box.position.x + box.size.x / 2.f,
                      box.position.y + box.size.y / 2.f - (hasSubtitle ? 10.f : 1.f)});
    target.draw(text);

    if (hasSubtitle)
    {
        sf::Text sub(font, m_subtitle);
        sub.setCharacterSize(static_cast<unsigned int>(box.size.y * 0.16f));
        sub.setFillColor(theme.textMuted);
        const sf::FloatRect subBounds = sub.getLocalBounds();
        sub.setOrigin(subBounds.getCenter());
        sub.setPosition({box.position.x + box.size.x / 2.f,
                         box.position.y + box.size.y / 2.f + 16.f});
        target.draw(sub);
    }

    // --- the coloured frame used to mark the right / wrong answer ----------
    if (m_highlight != sf::Color::Transparent)
    {
        // A soft halo first, so the frame does not just look like a thin line.
        for (int i = 2; i >= 1; --i)
        {
            const float grow = static_cast<float>(i) * 3.f;
            sf::RectangleShape halo(box.size + sf::Vector2f(grow * 2.f, grow * 2.f));
            halo.setPosition(box.position - sf::Vector2f(grow, grow));
            halo.setFillColor(sf::Color::Transparent);
            halo.setOutlineThickness(3.f);
            halo.setOutlineColor(sf::Color(m_highlight.r, m_highlight.g, m_highlight.b,
                                           static_cast<std::uint8_t>(70.f / static_cast<float>(i))));
            target.draw(halo);
        }

        sf::RectangleShape frame(box.size + sf::Vector2f(6.f, 6.f));
        frame.setPosition(box.position - sf::Vector2f(3.f, 3.f));
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineThickness(2.f);
        frame.setOutlineColor(m_highlight);
        target.draw(frame);
    }
}
