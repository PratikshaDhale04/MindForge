#include "Notification.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <utility>

void Notifications::add(NotificationKind kind, const std::string& title,
                        const std::string& message, float seconds)
{
    Notification item;
    item.title    = title;
    item.message  = message;
    item.kind     = kind;
    item.duration = seconds;
    item.life     = seconds;   // update() counts this down
    item.age      = 0.f;       // ...and this counts up

    // Newest first, so drawing is a simple loop from the top.
    m_items.insert(m_items.begin(), std::move(item));

    // Drop anything that no longer fits. The oldest ones are at the end.
    if (m_items.size() > static_cast<std::size_t>(kMaxVisible))
        m_items.resize(static_cast<std::size_t>(kMaxVisible));
}

void Notifications::update(float deltaSeconds)
{
    for (Notification& item : m_items)
    {
        item.life -= deltaSeconds;
        item.age  += deltaSeconds;
    }

    // erase-remove is the standard way to delete everything that matches a
    // condition. Here the condition is "the time is up".
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
                                 [](const Notification& n) { return n.life <= 0.f; }),
                  m_items.end());
}

sf::Color Notifications::colorFor(NotificationKind kind, const Theme& theme)
{
    switch (kind)
    {
    case NotificationKind::Success: return theme.success;
    case NotificationKind::Warning: return theme.warning;
    case NotificationKind::Error:   return theme.danger;
    case NotificationKind::Info:    break;
    }
    return theme.accent;
}

const char* Notifications::symbolFor(NotificationKind kind)
{
    switch (kind)
    {
    case NotificationKind::Success: return "+";
    case NotificationKind::Warning: return "!";
    case NotificationKind::Error:   return "x";
    case NotificationKind::Info:    break;
    }
    return "i";
}

float Notifications::easeOut(float t)
{
    // clamp() keeps the result inside 0..1 even if the frame was long.
    t = std::clamp(t, 0.f, 1.f);
    // 1 - (1-t)^3: fast at the start, slow at the end. This one line is what
    // makes a panel glide instead of jerk.
    return 1.f - (1.f - t) * (1.f - t) * (1.f - t);
}

std::vector<std::string> Notifications::wrap(const sf::Font& font, const std::string& text,
                                             unsigned int size, float maxWidth)
{
    // Measure one character to work out roughly how many fit on a line. This
    // is simpler than measuring every word and is accurate enough here.
    sf::Text probe(font, "M");
    probe.setCharacterSize(size);
    const float characterWidth = std::max(1.f, probe.getLocalBounds().size.x);
    const std::size_t maxCharacters = static_cast<std::size_t>(maxWidth / characterWidth);

    std::vector<std::string> lines;
    if (maxCharacters == 0)
    {
        lines.push_back(text);
        return lines;
    }

    std::istringstream words(text);
    std::string word, current;

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

void Notifications::draw(sf::RenderTarget& target, const sf::Font& font,
                         const Theme& theme) const
{
    // The panel has a fixed place on the screen. The rest of the game is laid
    // out so that no button ever ends up underneath it.
    const float left = kScreenW - kLeft - kWidth;

    for (std::size_t i = 0; i < m_items.size(); ++i)
    {
        const Notification& item = m_items[i];
        const sf::Color color = colorFor(item.kind, theme);

        // --- how visible it is right now ------------------------------------
        // 1. it slides in during its first kSlideIn seconds ...
        const float slide = easeOut(item.age / kSlideIn);
        // 2. ... and fades out during the last kFadeOut seconds of its life.
        const float fade = std::clamp(item.life / kFadeOut, 0.f, 1.f);

        // One single number that every drawing call below uses as "how solid
        // am I", so nothing can end up at a different transparency.
        const auto withAlpha = [fade](std::uint8_t r, std::uint8_t g, std::uint8_t b, float a) {
            return sf::Color(r, g, b, static_cast<std::uint8_t>(a * fade));
        };

        // --- where it sits ---------------------------------------------------
        // Newest at the top, every following message one row lower.
        const float y = kTop + static_cast<float>(i) * (kHeight + kGap);
        const float slideBy = (1.f - slide) * (kWidth + 24.f);
        const sf::FloatRect box(sf::Vector2f(left + slideBy, y),
                                sf::Vector2f(kWidth, kHeight));

        // --- a soft shadow, so the panel lifts off the background ------------
        sf::RectangleShape shadow(sf::Vector2f(kWidth, kHeight));
        shadow.setPosition(box.position + sf::Vector2f(0.f, 3.f));
        shadow.setFillColor(withAlpha(0, 0, 0, 110.f));
        target.draw(shadow);

        // --- the panel --------------------------------------------------------
        sf::RectangleShape panel(box.size);
        panel.setPosition(box.position);
        panel.setFillColor(withAlpha(theme.surface.r, theme.surface.g, theme.surface.b, 248.f));
        panel.setOutlineThickness(1.f);
        panel.setOutlineColor(withAlpha(color.r, color.g, color.b, 140.f));
        target.draw(panel);

        // --- a coloured bar down the left edge --------------------------------
        sf::RectangleShape bar(sf::Vector2f(4.f, kHeight));
        bar.setPosition(box.position);
        bar.setFillColor(withAlpha(color.r, color.g, color.b, 255.f));
        target.draw(bar);

        // --- the round symbol ---------------------------------------------------
        const float badgeY = box.position.y + 11.f;
        sf::CircleShape badge(10.f);
        badge.setOrigin({10.f, 10.f});
        badge.setPosition({box.position.x + 24.f, badgeY});
        badge.setFillColor(withAlpha(color.r, color.g, color.b, 255.f));
        target.draw(badge);

        sf::Text symbol(font, symbolFor(item.kind));
        symbol.setCharacterSize(14);
        symbol.setFillColor(withAlpha(theme.background.r, theme.background.g,
                                      theme.background.b, 255.f));
        const sf::FloatRect symbolBounds = symbol.getLocalBounds();
        symbol.setOrigin(symbolBounds.getCenter());
        symbol.setPosition({badge.getPosition().x + 10.f, badge.getPosition().y + 10.f});
        target.draw(symbol);

        // --- the title ------------------------------------------------------------
        sf::Text titleText(font, item.title);
        titleText.setCharacterSize(16);
        titleText.setFillColor(withAlpha(theme.text.r, theme.text.g, theme.text.b, 255.f));
        titleText.setPosition({box.position.x + 42.f, box.position.y + 9.f});
        target.draw(titleText);

        // --- the message, broken into as many lines as it needs -------------------
        const std::vector<std::string> lines = wrap(font, item.message, 13, kWidth - 54.f);
        float lineY = box.position.y + 31.f;
        for (const std::string& line : lines)
        {
            if (lineY > box.position.y + kHeight - 4.f)
                break;      // never draw outside the panel
            sf::Text bodyText(font, line);
            bodyText.setCharacterSize(13);
            bodyText.setFillColor(withAlpha(theme.textMuted.r, theme.textMuted.g,
                                            theme.textMuted.b, 255.f));
            bodyText.setPosition({box.position.x + 42.f, lineY});
            target.draw(bodyText);
            lineY += 16.f;
        }

        // --- the shrinking line that shows how much time is left ------------------
        const float left2 = std::clamp(item.life / item.duration, 0.f, 1.f);
        sf::RectangleShape timer(sf::Vector2f((kWidth - 42.f) * left2, 2.f));
        timer.setPosition({box.position.x + 42.f, box.position.y + kHeight - 5.f});
        timer.setFillColor(withAlpha(color.r, color.g, color.b, 170.f));
        target.draw(timer);
    }
}
