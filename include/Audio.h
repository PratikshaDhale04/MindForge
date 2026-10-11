#pragma once

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <filesystem>
#include <optional>

/// \file Audio.h
/// Plays the three short sound effects: click, correct and wrong.
///
/// The important rule here is that sound is *optional*. If `assets/sounds` is
/// empty, or a file is corrupt, the game still starts and runs - the effect is
/// simply skipped. That is why every sound is stored in a `std::optional`:
/// an empty optional means "no sound loaded".
///
/// No music track is bundled, because music files are large and a repository
/// should stay small. The Music switch on the Settings screen is only enabled
/// if you drop a music*.ogg file into assets/sounds yourself.

/// The effects the game can play.
enum class SoundEffect
{
    Click,
    Correct,
    Wrong
};

class Audio
{
public:
    /// Try to load the effects from `folder`. Missing files are not an error.
    void load(const std::filesystem::path& folder);

    void play(SoundEffect effect);

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool enabled() const { return m_enabled; }

    /// True when at least one sound file was found. The Settings screen uses
    /// this to explain why there is no music.
    bool hasAnySound() const;

private:
    /// Load one file into one optional buffer and sound.
    /// Returns false when the file is missing or cannot be read.
    bool loadInto(const std::filesystem::path& file,
                  std::optional<sf::SoundBuffer>& buffer,
                  std::optional<sf::Sound>& target);

    std::optional<sf::SoundBuffer> m_clickBuffer;
    std::optional<sf::SoundBuffer> m_correctBuffer;
    std::optional<sf::SoundBuffer> m_wrongBuffer;

    std::optional<sf::Sound> m_click;
    std::optional<sf::Sound> m_correct;
    std::optional<sf::Sound> m_wrong;

    bool m_enabled{true};
};
