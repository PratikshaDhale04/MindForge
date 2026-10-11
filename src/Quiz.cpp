#include "Quiz.h"

#include <algorithm>

int Quiz::timeForDifficulty(const std::string& difficulty) const
{
    if (difficulty == "Easy")
        return EASY_TIME;
    if (difficulty == "Hard")
        return HARD_TIME;
    return MEDIUM_TIME;   // "Medium" and anything unexpected
}

int Quiz::questionsForDifficulty(const std::string& difficulty) const
{
    if (difficulty == "Easy")
        return EASY_QUESTIONS;
    if (difficulty == "Hard")
        return HARD_QUESTIONS;
    return MEDIUM_QUESTIONS;   // "Medium" and anything unexpected
}

bool Quiz::start(const std::string& category,
                 const std::string& difficulty,
                 std::string& errorMessage)
{
    // How long and how many are both decided by the difficulty.
    const int wanted = questionsForDifficulty(difficulty);

    m_questions = pickQuestions(category, wanted);

    if (m_questions.empty())
    {
        errorMessage = "There are no questions for " + category + " yet.";
        return false;
    }

    if (static_cast<int>(m_questions.size()) < wanted)
    {
        // Not a failure - the player simply gets everything that exists.
        errorMessage = "Only " + std::to_string(m_questions.size()) +
                       " questions available for " + category + ".";
    }
    else
    {
        errorMessage.clear();
    }

    m_index       = 0;
    m_score       = 0;
    m_correct     = 0;
    m_wrong       = 0;
    m_unanswered  = 0;
    m_streak      = 0;
    m_bestStreak  = 0;
    m_chosen      = '?';
    m_lastCorrect = false;
    m_timedOut    = false;
    m_phase       = QuizPhase::Asking;
    m_timeLimit   = static_cast<float>(timeForDifficulty(difficulty));

    m_clock.restart();
    return true;
}

void Quiz::update()
{
    if (m_phase != QuizPhase::Asking)
        return;

    if (timeLeft() <= 0.f)
    {
        // The player ran out of time: count it as unanswered and show the
        // feedback so the correct answer is still revealed.
        ++m_unanswered;
        m_chosen      = '?';
        m_lastCorrect = false;
        m_timedOut    = true;
        m_streak      = 0;          // a streak must not survive a timeout
        m_phase       = QuizPhase::Feedback;
    }
}

void Quiz::answer(char choice)
{
    if (m_phase != QuizPhase::Asking || m_questions.empty())
        return;

    m_chosen = choice;
    m_timedOut = false;

    if (choice == m_questions[m_index].correctAnswer)
    {
        ++m_correct;
        m_score += POINTS_PER_CORRECT;

        ++m_streak;
        if (m_streak > m_bestStreak)
            m_bestStreak = m_streak;

        m_lastCorrect = true;
    }
    else
    {
        ++m_wrong;
        m_streak = 0;              // a wrong answer breaks the streak
        m_lastCorrect = false;
    }

    m_phase = QuizPhase::Feedback;
}

const Question& Quiz::currentQuestion() const
{
    if (m_index < m_questions.size())
        return m_questions[m_index];

    // Only reachable before the first start() or after a failed one. Handing
    // back an empty question keeps the drawing code free of checks.
    static const Question empty;
    return empty;
}

void Quiz::advance()
{
    if (m_phase != QuizPhase::Feedback)
        return;

    // Nothing to advance through.
    if (m_questions.empty())
    {
        m_phase = QuizPhase::Finished;
        return;
    }

    ++m_index;

    if (m_index >= m_questions.size())
    {
        m_index = m_questions.size() - 1;
        m_phase = QuizPhase::Finished;
        return;
    }

    m_phase = QuizPhase::Asking;
    m_clock.restart();
}

float Quiz::timeLeft() const
{
    if (m_phase == QuizPhase::Finished)
        return 0.f;

    const float elapsed = m_clock.getElapsedTime().asSeconds();
    return std::max(0.f, m_timeLimit - elapsed);
}

float Quiz::timeFraction() const
{
    if (m_timeLimit <= 0.f)
        return 0.f;
    return timeLeft() / m_timeLimit;
}

bool Quiz::timeIsLow() const
{
    // The last 25% of the available time, with a minimum of 3 seconds so the
    // warning is not missed on a short Hard timer.
    return m_phase == QuizPhase::Asking && timeLeft() <= std::max(3.f, m_timeLimit * 0.25f);
}

float Quiz::accuracy() const
{
    if (m_questions.empty())
        return 0.f;
    return 100.f * static_cast<float>(m_correct) / static_cast<float>(m_questions.size());
}
