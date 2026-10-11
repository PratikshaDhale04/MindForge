#pragma once

#include "Question.h"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#include <string>
#include <vector>

/// \file Quiz.h
/// The quiz itself: which question is showing, the score, the timer and the
/// streak. It contains no drawing code at all - the Game class decides how to
/// show what Quiz reports.
///
/// Separating the two is the most important idea in this project:
///   Quiz  -> "what is the state right now?"  (data and rules)
///   Game  -> "how do I draw that state?"     (graphics and input)
/// Because of that you can change the look completely without breaking the
/// scoring, and you can test the scoring without opening a window.

// ---- How many questions one quiz contains, chosen by the difficulty. -------
// A harder level is a longer quiz *and* a shorter timer, so the number of
// questions and the seconds per question both change with the difficulty.
inline constexpr int EASY_QUESTIONS   = 5;
inline constexpr int MEDIUM_QUESTIONS = 8;
inline constexpr int HARD_QUESTIONS   = 12;

// ---- Points for one correct answer. --------------------------------------
inline constexpr int POINTS_PER_CORRECT = 10;

// ---- Seconds allowed per question, chosen by the difficulty. --------------
// These are plain constants so they are easy to find and change.
inline constexpr int EASY_TIME   = 30;
inline constexpr int MEDIUM_TIME = 20;
inline constexpr int HARD_TIME   = 15;

/// The longest list the high score table keeps.
inline constexpr int MAX_HIGH_SCORES = 10;

/// What the quiz is doing at the moment.
enum class QuizPhase
{
    Asking,      ///< Waiting for the player to answer.
    Feedback,    ///< The answer was given - show the result for a moment.
    Finished     ///< Every question has been played.
};

class Quiz
{
public:
    Quiz() = default;

    /// Start a new quiz. The difficulty decides how many questions there are
    /// and how much time each one gets.
    ///
    /// Returns false (and fills `errorMessage`) when the chosen category has
    /// no questions at all.
    bool start(const std::string& category,
               const std::string& difficulty,
               std::string& errorMessage);

    /// Call once per frame. Detects when the time runs out.
    void update();

    /// The player picked option `choice` ('A'..'D'). Ignored unless the quiz is
    /// currently waiting for an answer.
    void answer(char choice);

    /// Move on to the next question, or finish when this was the last one.
    void advance();

    // ---- reading the current state ---------------------------------------
    /// The question being asked. A quiz that has not been started yet has no
    /// questions, so a blank placeholder is handed back rather than letting
    /// the caller read past the end of an empty list.
    const Question& currentQuestion() const;
    const std::vector<Question>& questions() const { return m_questions; }
    std::size_t questionCount() const { return m_questions.size(); }
    std::size_t currentIndex() const { return m_index; }

    int score() const { return m_score; }
    int correctCount() const { return m_correct; }
    int wrongCount() const { return m_wrong; }
    int unansweredCount() const { return m_unanswered; }
    int streak() const { return m_streak; }
    int bestStreak() const { return m_bestStreak; }
    QuizPhase phase() const { return m_phase; }

    /// The option the player picked, valid during QuizPhase::Feedback.
    char chosenAnswer() const { return m_chosen; }
    /// True when the last answer was correct.
    bool lastAnswerCorrect() const { return m_lastCorrect; }
    /// True when the question ended because the time ran out.
    bool lastAnswerTimedOut() const { return m_timedOut; }

    /// Seconds left for the current question.
    float timeLeft() const;
    /// 0..1 - how much of the time is still left (for the progress bar).
    float timeFraction() const;
    /// True during the last few seconds, so the UI can flash a warning.
    bool timeIsLow() const;

    /// The score a perfect run would have got, e.g. 50 for five questions.
    int maximumScore() const
    {
        return static_cast<int>(m_questions.size()) * POINTS_PER_CORRECT;
    }

    /// Correct answers as a percentage of all questions.
    float accuracy() const;

private:
    /// Seconds allowed for one question, taken from the difficulty.
    int timeForDifficulty(const std::string& difficulty) const;
    /// Number of questions in a quiz of this difficulty.
    int questionsForDifficulty(const std::string& difficulty) const;

    std::vector<Question> m_questions;
    std::size_t m_index{0};

    sf::Clock m_clock;      ///< Restarted for every question.
    float m_timeLimit{20.f};

    int m_score{0};
    int m_correct{0};
    int m_wrong{0};
    int m_unanswered{0};
    int m_streak{0};
    int m_bestStreak{0};

    QuizPhase m_phase{QuizPhase::Finished};
    char m_chosen{'?'};
    bool m_lastCorrect{false};
    bool m_timedOut{false};
};
