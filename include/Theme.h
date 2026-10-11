#pragma once

#include <SFML/Graphics/Color.hpp>

/// \file Theme.h
/// All the colours of the game live here, in one place.
///
/// Why? Because writing colour values inside the drawing code makes it
/// impossible to add a second look. With a Theme struct the drawing code asks
/// for `theme.text` or `theme.accent`, and the Settings screen can simply hand
/// over a different Theme object to switch between light and dark.
///
/// A `struct` is used because we only want to group the values, not hide them
/// behind getter functions. It is created once and then only read.

struct Theme
{
    // Page background.
    sf::Color background{233, 239, 250};
    // The second stop of the background gradient. See Game::drawBackground().
    sf::Color backgroundTop{245, 248, 254};
    // Slightly lighter panels and cards drawn on top of the background.
    sf::Color surface{255, 255, 255};
    // Line around cards, so they stand out on the background.
    sf::Color border{214, 224, 242};

    // Main text and the dimmer text used for hints and labels.
    sf::Color text{27, 36, 64};
    sf::Color textMuted{107, 122, 153};

    // Buttons in their normal and hovered state.
    sf::Color button{255, 255, 255};
    sf::Color buttonHover{237, 242, 254};

    // Main accent colour, used for highlights, the progress bar and titles.
    sf::Color accent{59, 130, 246};
    // A second accent, used for the glow behind titles and blobs.
    sf::Color accent2{139, 92, 246};
    // Feedback colours.
    sf::Color success{16, 185, 129};
    sf::Color danger{239, 68, 68};
    // Used to warn about a running-out timer.
    sf::Color warning{245, 158, 11};
};

/// The light look. This is the one the game starts with.
inline Theme makeLightTheme()
{
    Theme t;
    t.background    = sf::Color(233, 239, 250);
    t.backgroundTop = sf::Color(246, 249, 255);
    t.surface       = sf::Color(255, 255, 255);
    t.border        = sf::Color(214, 224, 242);
    t.text          = sf::Color(27, 36, 64);
    t.textMuted     = sf::Color(107, 122, 153);
    t.button        = sf::Color(255, 255, 255);
    t.buttonHover   = sf::Color(235, 241, 253);
    t.accent        = sf::Color(59, 130, 246);
    t.accent2       = sf::Color(139, 92, 246);
    t.success       = sf::Color(16, 185, 129);
    t.danger        = sf::Color(239, 68, 68);
    t.warning       = sf::Color(245, 158, 11);
    return t;
}

/// The dark look, chosen on the Settings screen.
inline Theme makeDarkTheme()
{
    Theme t;
    t.background    = sf::Color(22, 25, 40);   // midnight blue
    t.backgroundTop = sf::Color(13, 15, 27);   // darker at the very top
    t.surface       = sf::Color(31, 35, 55);   // cards
    t.border        = sf::Color(56, 63, 92);
    t.text          = sf::Color(240, 243, 250);
    t.textMuted     = sf::Color(146, 154, 184);
    t.button        = sf::Color(44, 50, 76);
    t.buttonHover   = sf::Color(64, 72, 106);
    t.accent        = sf::Color(99, 102, 241);   // indigo
    t.accent2       = sf::Color(168, 85, 247);   // violet
    t.success       = sf::Color(34, 197, 94);    // emerald
    t.danger        = sf::Color(244, 63, 94);    // rose
    t.warning       = sf::Color(251, 191, 36);   // amber
    return t;
}
