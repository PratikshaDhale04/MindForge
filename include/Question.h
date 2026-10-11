#pragma once

#include <string>
#include <vector>

/// \file Question.h
/// Holds the quiz questions and the functions that pick them.
///
/// Why keep the questions in the program instead of a database?
///   Because it is the simplest thing that works, and it keeps the project
///   readable. A `std::vector<Question>` is just a list of Question objects,
///   and everything the quiz needs is already in memory.
///
/// If you ever want to edit questions without recompiling, you can load the
/// same `Question` struct from a text file - nothing else has to change.

/// One multiple-choice question.
struct Question
{
    std::string question;      ///< The text shown to the player.
    std::string optionA;       ///< The four possible answers.
    std::string optionB;
    std::string optionC;
    std::string optionD;
    char correctAnswer{ 'A' }; ///< Which option is right: 'A', 'B', 'C' or 'D'.
    std::string explanation;   ///< Shown after answering, helps the player learn.
    std::string category;      ///< "C++", "Python", "SQL" or "General Knowledge".
    std::string difficulty;    ///< "Easy", "Medium" or "Hard".
};

/// The whole question bank. The function returns the same list every time
/// because the list is built only once, on the first call.
const std::vector<Question>& questionBank();

/// The names of the categories that have at least one question.
/// This is what the category screen shows, so adding a category to the bank
/// is enough to make it appear in the game.
std::vector<std::string> availableCategories();

/// How many questions exist for a category, optionally filtered by difficulty.
/// An empty `difficulty` counts every question in the category.
int countQuestions(const std::string& category, const std::string& difficulty);

/// Pick `howMany` random questions from the given category.
///
/// The questions are returned in a random order and never contain the same
/// question twice, so playing the same category twice does not feel identical.
/// The whole category is shuffled, which is why a category with many questions
/// gives a different quiz every time even at the same difficulty.
///
/// If the category has fewer questions than `howMany`, everything that exists
/// is returned - the caller checks the size and can tell the player.
std::vector<Question> pickQuestions(const std::string& category, int howMany);
