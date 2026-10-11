#include "Audio.h"

#include <iostream>

void Audio::load(const std::filesystem::path& folder)
{
    const std::filesystem::path click   = folder / "click.wav";
    const std::filesystem::path correct = folder / "correct.wav";
    const std::filesystem::path wrong   = folder / "wrong.wav";

    // loadInto() returns false for a missing file and simply carries on, so a
    // player without sound files never sees an error window.
    const bool okClick   = loadInto(click, m_clickBuffer, m_click);
    const bool okCorrect = loadInto(correct, m_correctBuffer, m_correct);
    const bool okWrong   = loadInto(wrong, m_wrongBuffer, m_wrong);

    if (!okClick && !okCorrect && !okWrong)
    {
        std::cout << "[QuizMaster] No sound files found - running silently.\n";
    }
}

bool Audio::loadInto(const std::filesystem::path& file,
                     std::optional<sf::SoundBuffer>& buffer,
                     std::optional<sf::Sound>& target)
{
    // Check the file is there before asking SFML to read it. SFML writes a
    // long message to the console for every file it cannot open, and sound is
    // optional here, so a missing file must be completely silent.
    std::error_code error;
    if (!std::filesystem::exists(file, error))
        return false;

    // The buffer is created inside the optional. If the file is missing the
    // optional simply stays empty and the game carries on without the sound.
    buffer.emplace();
    if (!buffer->loadFromFile(file))
    {
        buffer.reset();
        return false;
    }

    // sf::Sound needs a buffer to work with, which is why it is optional too:
    // it only exists when a file was actually loaded.
    target.emplace(*buffer);
    target->setVolume(70.f);
    return true;
}

void Audio::play(SoundEffect effect)
{
    if (!m_enabled)
        return;

    // Restarts a sound that is still playing, so fast clicks do not queue up.
    switch (effect)
    {
    case SoundEffect::Click:   if (m_click)   m_click->play();   break;
    case SoundEffect::Correct: if (m_correct) m_correct->play(); break;
    case SoundEffect::Wrong:   if (m_wrong)   m_wrong->play();   break;
    }
}

bool Audio::hasAnySound() const
{
    return m_click.has_value() || m_correct.has_value() || m_wrong.has_value();
}
