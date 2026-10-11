#include "Question.h"

#include <algorithm>
#include <random>

namespace
{

/// Fills a Question without repeating the category and difficulty on every line.
Question makeQuestion(std::string category,
                      std::string difficulty,
                      std::string text,
                      std::string a,
                      std::string b,
                      std::string c,
                      std::string d,
                      char correct,
                      std::string explanation)
{
    Question q;
    q.question      = std::move(text);
    q.optionA       = std::move(a);
    q.optionB       = std::move(b);
    q.optionC       = std::move(c);
    q.optionD       = std::move(d);
    q.correctAnswer = correct;
    q.explanation   = std::move(explanation);
    q.category      = std::move(category);
    q.difficulty    = std::move(difficulty);
    return q;
}

/// The question bank. A function-local `static` is created on the first call
/// and then kept in memory, so the 60 questions are not rebuilt on every call.
std::vector<Question> buildBank()
{
    std::vector<Question> bank;

    // =======================================================================
    // C++ - Easy
    // =======================================================================
    bank.push_back(makeQuestion("C++", "Easy",
        "Which extension is used for a C++ source file?",
        ".py", ".cpp", ".java", ".html", 'B',
        "C++ source files use .cpp; .h is normally used for the header file."));
    bank.push_back(makeQuestion("C++", "Easy",
        "Which keyword declares a constant value that cannot be changed?",
        "static", "const", "final", "fixed", 'B',
        "const promises that a variable will not be modified after it is set."));
    bank.push_back(makeQuestion("C++", "Easy",
        "Which symbol is used to read a value from standard input?",
        "std::cout", "std::cin", "std::endl", "std::file", 'B',
        "std::cin is the input stream; std::cout is the output stream."));
    bank.push_back(makeQuestion("C++", "Easy",
        "What does a semicolon ';' at the end of a C++ statement do?",
        "Starts a comment", "Ends the statement", "Opens a block", "Includes a file", 'B',
        "Every C++ statement is finished with a semicolon."));
    bank.push_back(makeQuestion("C++", "Easy",
        "Which of these is a valid C++ comment?",
        "# a comment", "// a comment", "<!-- a comment -->", "' a comment", 'B',
        "C++ uses // for a line comment and /* */ for a block comment."));

    // =======================================================================
    // C++ - Medium
    // =======================================================================
    bank.push_back(makeQuestion("C++", "Medium",
        "What is encapsulation?",
        "Hiding data behind public functions", "Copying a class",
        "Inheriting from two classes", "Overloading an operator", 'A',
        "Encapsulation keeps the data private and exposes it through methods."));
    bank.push_back(makeQuestion("C++", "Medium",
        "Which keyword is used to inherit from a base class?",
        "class", "struct", "public", "virtual", 'C',
        "In C++ a class inherits with the colon and an access specifier."));
    bank.push_back(makeQuestion("C++", "Medium",
        "What does a virtual function guarantee?",
        "The function is fast", "The function can be overridden by a derived class",
        "The function is private", "The function runs only once", 'B',
        "virtual allows dynamic dispatch, so the derived version is called through a base pointer."));
    bank.push_back(makeQuestion("C++", "Medium",
        "What is the difference between std::vector and std::array?",
        "vector is always sorted", "vector can grow, array has a fixed size",
        "array is faster to declare", "There is no difference", 'B',
        "std::array has a size fixed at compile time; std::vector can resize."));
    bank.push_back(makeQuestion("C++", "Medium",
        "What does a reference (&) allow you to do?",
        "Create a copy of a variable", "Access the original variable without copying it",
        "Lock a variable", "Rename a variable", 'B',
        "A reference is another name for the same object, so changes are visible outside."));

    // =======================================================================
    // C++ - Hard
    // =======================================================================
    bank.push_back(makeQuestion("C++", "Hard",
        "What is the difference between a deep copy and a shallow copy?",
        "A deep copy copies nested objects too",
        "A shallow copy copies the top level only, so both objects share nested data",
        "A shallow copy is always faster",
        "There is no difference", 'B',
        "Shallow copying duplicates the outer object but shares the pointed-to data."));
    bank.push_back(makeQuestion("C++", "Hard",
        "What is a dangling pointer?",
        "A pointer that was never initialised",
        "A pointer that points to memory which has been destroyed",
        "A pointer to a const object",
        "A null pointer", 'B',
        "After delete or after a local variable goes out of scope, a pointer to it dangles."));
    bank.push_back(makeQuestion("C++", "Hard",
        "What is the difference between std::string and a C-style string?",
        "std::string knows its own length and manages its memory",
        "std::string cannot store text",
        "C-style strings are faster in every case",
        "They are exactly the same", 'A',
        "std::string stores its size and frees its memory automatically."));
    bank.push_back(makeQuestion("C++", "Hard",
        "What is the rule of three?",
        "If you define a destructor, copy constructor or copy assignment, you usually need all three",
        "Every class needs exactly three methods",
        "Classes may have at most three members",
        "It applies only to structs", 'A',
        "Managing a resource correctly means handling construction, copying and destruction."));
    bank.push_back(makeQuestion("C++", "Hard",
        "What does 'const' after a member function name mean?",
        "The function cannot be called",
        "The function will not modify the object it is called on",
        "The function returns a constant",
        "The function runs only once", 'B',
        "A const member function promises not to change the object's state."));

    // =======================================================================
    // Python - Easy
    // =======================================================================
    bank.push_back(makeQuestion("Python", "Easy",
        "Which extension is used for a Python source file?",
        ".cpp", ".py", ".rs", ".cs", 'B',
        "Python source files use the .py extension."));
    bank.push_back(makeQuestion("Python", "Easy",
        "Which keyword starts a loop that repeats a fixed number of times?",
        "while", "for", "repeat", "loop", 'B',
        "for repeats over a range or a collection; while repeats on a condition."));
    bank.push_back(makeQuestion("Python", "Easy",
        "How do you print something on the screen in Python?",
        "print()", "echo()", "show()", "display()", 'A',
        "print() writes a value to the standard output."));
    bank.push_back(makeQuestion("Python", "Easy",
        "Which of these is used to store a whole number?",
        "int", "str", "list", "tuple", 'A',
        "int holds whole numbers such as 5 or -12."));
    bank.push_back(makeQuestion("Python", "Easy",
        "How do you write a comment in Python?",
        "// comment", "# comment", "/* comment */", "<!-- comment -->", 'B',
        "Python uses # for a comment; a triple-quoted string is also sometimes used."));

    // =======================================================================
    // Python - Medium
    // =======================================================================
    bank.push_back(makeQuestion("Python", "Medium",
        "What is a list in Python?",
        "A collection that keeps order and can be changed",
        "A collection that can only hold one item",
        "A key-value store that is always sorted",
        "A read-only file", 'A',
        "A list is mutable and ordered, so you can add, remove and change items."));
    bank.push_back(makeQuestion("Python", "Medium",
        "What is the difference between a list and a tuple?",
        "A tuple cannot be changed after it is created",
        "A list cannot be changed",
        "They are identical",
        "A tuple only stores numbers", 'A',
        "Tuples are immutable, lists are mutable."));
    bank.push_back(makeQuestion("Python", "Medium",
        "What does a dictionary store?",
        "Key-value pairs", "Only numbers", "A sorted list of characters",
        "A single value", 'A',
        "A dict maps unique keys to values, like {name: 'Asha'}."));
    bank.push_back(makeQuestion("Python", "Medium",
        "What is the purpose of a function in Python?",
        "To store data in a file", "To group reusable code under a name",
        "To draw a window", "To import a library", 'B',
        "A function is a named block of code you can call whenever you need it."));
    bank.push_back(makeQuestion("Python", "Medium",
        "What is the result of 10 / 3 in Python 3?",
        "3", "3.3333333333333335", "1", "It is an error", 'B',
        "Python 3 division of two ints always returns a float; use 10 // 3 for 3."));

    // =======================================================================
    // Python - Hard
    // =======================================================================
    bank.push_back(makeQuestion("Python", "Hard",
        "What is a generator in Python?",
        "A function that yields values one at a time instead of building a list",
        "A function that draws shapes",
        "A class that generates random numbers",
        "A tool that compiles code", 'A',
        "Generators are lazy, so they use far less memory for large sequences."));
    bank.push_back(makeQuestion("Python", "Hard",
        "What is the difference between 'is' and '=='?",
        "'is' checks object identity, '==' checks equal values",
        "They are exactly the same",
        "'is' compares strings, '==' compares numbers",
        "'is' is only valid in Python 2", 'A',
        "'is' asks whether two names point to the same object in memory."));
    bank.push_back(makeQuestion("Python", "Hard",
        "What does the GIL stand for, and what does it do?",
        "Global Interpreter Lock - it allows only one thread of Python code to run at a time",
        "General Input Library - it reads files",
        "Garbage Index List - it frees unused memory",
        "Global Interface Lock - it hides functions", 'A',
        "The GIL makes CPU-bound multithreading hard in pure Python."));
    bank.push_back(makeQuestion("Python", "Hard",
        "What is the difference between a shallow and a deep copy?",
        "A deep copy also duplicates nested objects",
        "A shallow copy is always larger",
        "Deep copying is only for strings",
        "They are the same", 'A',
        "copy.deepcopy walks nested containers; copy.copy does not."));
    bank.push_back(makeQuestion("Python", "Hard",
        "Why would you use a decorator?",
        "To wrap a function and extend its behaviour without changing its body",
        "To store a value in a file",
        "To repeat a loop faster",
        "To import a module automatically", 'A',
        "A decorator takes a function and returns a new, usually extended function."));

    // =======================================================================
    // SQL - Easy
    // =======================================================================
    bank.push_back(makeQuestion("SQL", "Easy",
        "Which command is used to read data from a table?",
        "SELECT", "INSERT", "UPDATE", "DELETE", 'A',
        "SELECT queries rows; INSERT adds, UPDATE changes, DELETE removes."));
    bank.push_back(makeQuestion("SQL", "Easy",
        "Which keyword removes rows from a table?",
        "DROP", "REMOVE", "DELETE", "CLEAR", 'C',
        "DELETE removes rows; DROP removes the whole table."));
    bank.push_back(makeQuestion("SQL", "Easy",
        "Which command adds a new row to a table?",
        "ADD", "INSERT INTO", "CREATE ROW", "NEW", 'B',
        "INSERT INTO table (columns) VALUES (values) adds a row."));
    bank.push_back(makeQuestion("SQL", "Easy",
        "What does the WHERE clause do?",
        "Filters which rows are affected",
        "Sorts the rows",
        "Renames a column",
        "Creates an index", 'A',
        "WHERE keeps only the rows that match the condition."));
    bank.push_back(makeQuestion("SQL", "Easy",
        "Which command changes existing rows?",
        "UPDATE", "ALTER", "MODIFY", "SET", 'A',
        "UPDATE ... SET ... WHERE ... changes the values in matching rows."));

    // =======================================================================
    // SQL - Medium
    // =======================================================================
    bank.push_back(makeQuestion("SQL", "Medium",
        "What is a primary key?",
        "A column that uniquely identifies each row",
        "A column used only for sorting",
        "A column that stores the date",
        "A column that can contain NULL only", 'A',
        "A primary key is unique and cannot be duplicated or NULL in every row."));
    bank.push_back(makeQuestion("SQL", "Medium",
        "What is a foreign key?",
        "A column that links to the primary key of another table",
        "A key used for encryption",
        "A column with a default value",
        "A type of index", 'A',
        "Foreign keys create relationships between tables and keep them consistent."));
    bank.push_back(makeQuestion("SQL", "Medium",
        "What is the difference between DELETE and TRUNCATE?",
        "TRUNCATE removes all rows and cannot be filtered with WHERE",
        "They are identical",
        "DELETE is faster",
        "TRUNCATE only renames rows", 'A',
        "DELETE can filter row by row; TRUNCATE clears the whole table quickly."));
    bank.push_back(makeQuestion("SQL", "Medium",
        "Which join returns rows that match in both tables only?",
        "LEFT JOIN", "INNER JOIN", "FULL JOIN", "CROSS JOIN", 'B',
        "INNER JOIN keeps only the rows that have a match on both sides."));
    bank.push_back(makeQuestion("SQL", "Medium",
        "What does COUNT(*) do?",
        "Counts the rows a query returned",
        "Adds a number to a column",
        "Deletes duplicate rows",
        "Sorts by number", 'A',
        "COUNT(*) returns how many rows the result set contains."));

    // =======================================================================
    // SQL - Hard
    // =======================================================================
    bank.push_back(makeQuestion("SQL", "Hard",
        "What is normalisation mainly for?",
        "Reducing data duplication and update anomalies",
        "Making queries slower on purpose",
        "Encrypting a database",
        "Storing images", 'A',
        "Normalisation splits data into related tables so each fact is stored once."));
    bank.push_back(makeQuestion("SQL", "Hard",
        "What is the difference between a view and a table?",
        "A view stores a saved query, not the data itself",
        "A view cannot be selected from",
        "A table stores a query",
        "They are the same", 'A',
        "A view is a virtual table defined by a SELECT statement."));
    bank.push_back(makeQuestion("SQL", "Hard",
        "What does an index improve most?",
        "The speed of finding rows",
        "The readability of the schema",
        "The number of columns",
        "The size of the database file", 'A',
        "Indexes speed up search, at the cost of slower writes and extra storage."));
    bank.push_back(makeQuestion("SQL", "Hard",
        "In ACID, what does 'I' stand for?",
        "Isolation", "Index", "Integrity", "Identity", 'A',
        "Isolation means a transaction's changes are not visible until it completes."));
    bank.push_back(makeQuestion("SQL", "Hard",
        "What is a recursive common table expression used for?",
        "Querying hierarchies such as an employee-manager tree",
        "Deleting a table quickly",
        "Creating indexes automatically",
        "Backing up a database", 'A',
        "A recursive CTE keeps joining a table to itself until no rows are left."));

    // =======================================================================
    // General Knowledge - Easy
    // =======================================================================
    bank.push_back(makeQuestion("General Knowledge", "Easy",
        "How many continents are there on Earth?",
        "5", "6", "7", "8", 'C',
        "The commonly taught count is seven continents."));
    bank.push_back(makeQuestion("General Knowledge", "Easy",
        "Which planet is known as the Red Planet?",
        "Venus", "Mars", "Jupiter", "Mercury", 'B',
        "Iron oxide dust gives Mars its reddish colour."));
    bank.push_back(makeQuestion("General Knowledge", "Easy",
        "What is the capital of India?",
        "Mumbai", "New Delhi", "Bengaluru", "Chennai", 'B',
        "New Delhi has been the capital of India since 1931."));
    bank.push_back(makeQuestion("General Knowledge", "Easy",
        "How many days are there in a leap year?",
        "365", "366", "364", "367", 'B',
        "February gains one day, so a leap year has 366 days."));
    bank.push_back(makeQuestion("General Knowledge", "Easy",
        "Which gas do plants absorb from the air for photosynthesis?",
        "Oxygen", "Carbon dioxide", "Nitrogen", "Hydrogen", 'B',
        "Plants take in CO2 and release oxygen."));

    // =======================================================================
    // General Knowledge - Medium
    // =======================================================================
    bank.push_back(makeQuestion("General Knowledge", "Medium",
        "Who wrote the play 'Romeo and Juliet'?",
        "Charles Dickens", "William Shakespeare", "Jane Austen", "Mark Twain", 'B',
        "The play was written by William Shakespeare around 1595."));
    bank.push_back(makeQuestion("General Knowledge", "Medium",
        "What is the smallest prime number?",
        "0", "1", "2", "3", 'C',
        "2 is the only even prime number; 0 and 1 are not prime."));
    bank.push_back(makeQuestion("General Knowledge", "Medium",
        "Which organ pumps blood around the human body?",
        "Liver", "Heart", "Lung", "Kidney", 'B',
        "The heart circulates blood through the pulmonary and systemic circuits."));
    bank.push_back(makeQuestion("General Knowledge", "Medium",
        "What is the largest ocean on Earth?",
        "Atlantic", "Indian", "Arctic", "Pacific", 'D',
        "The Pacific Ocean covers more area than all the land combined."));
    bank.push_back(makeQuestion("General Knowledge", "Medium",
        "In which year did the First World War begin?",
        "1912", "1914", "1918", "1920", 'B',
        "The war started in July 1914 and ended in November 1918."));

    // =======================================================================
    // General Knowledge - Hard
    // =======================================================================
    bank.push_back(makeQuestion("General Knowledge", "Hard",
        "What is the value of Pi to two decimal places?",
        "3.12", "3.14", "3.16", "3.18", 'B',
        "Pi is the ratio of a circle's circumference to its diameter: about 3.14159."));
    bank.push_back(makeQuestion("General Knowledge", "Hard",
        "Who was the first woman to win a Nobel Prize?",
        "Marie Curie", "Ada Lovelace", "Rosalind Franklin", "Katherine Johnson", 'A',
        "Marie Curie won the 1903 Nobel Prize in Physics."));
    bank.push_back(makeQuestion("General Knowledge", "Hard",
        "What is the most abundant gas in Earth's atmosphere?",
        "Oxygen", "Carbon dioxide", "Nitrogen", "Argon", 'C',
        "Nitrogen makes up roughly 78 percent of the air."));
    bank.push_back(makeQuestion("General Knowledge", "Hard",
        "Which ancient civilisation built Machu Picchu?",
        "Aztec", "Maya", "Inca", "Roman", 'C',
        "Machu Picchu was built by the Inca in the 15th century."));
    bank.push_back(makeQuestion("General Knowledge", "Hard",
        "What does DNA stand for?",
        "Deoxyribonucleic acid", "Dinucleic acid", "Deoxyribose nucleus acid",
        "Dual nuclear acid", 'A',
        "DNA is the molecule that stores genetic information."));

    return bank;
}

} // namespace

const std::vector<Question>& questionBank()
{
    static const std::vector<Question> bank = buildBank();
    return bank;
}

std::vector<std::string> availableCategories()
{
    std::vector<std::string> names;

    for (const Question& q : questionBank())
    {
        const bool alreadyListed =
            std::find(names.begin(), names.end(), q.category) != names.end();
        if (!alreadyListed)
            names.push_back(q.category);
    }

    return names;
}

int countQuestions(const std::string& category, const std::string& difficulty)
{
    int total = 0;

    for (const Question& q : questionBank())
    {
        if (q.category == category && (difficulty.empty() || q.difficulty == difficulty))
            ++total;
    }

    return total;
}

std::vector<Question> pickQuestions(const std::string& category, int howMany)
{
    // Step 1: collect every question of the chosen category.
    //
    // Note that the difficulty is *not* used to filter here. Each category
    // holds 15 questions but a quiz is only 5 to 12 long, so filtering by
    // difficulty would hand out exactly the same questions every single time.
    // The difficulty already makes a difference in two other ways: it decides
    // how many questions the quiz has and how much time each one gets.
    std::vector<Question> matching;
    for (const Question& q : questionBank())
    {
        if (q.category == category)
            matching.push_back(q);
    }

    // Step 2: shuffle them. A fixed seed would repeat the same order every
    // time, so the generator is seeded from the system clock.
    std::random_device seed;
    std::mt19937 generator(seed());
    std::shuffle(matching.begin(), matching.end(), generator);

    // Step 3: keep only as many as the player asked for. Shrinking the vector
    // like this is safe - the extra Question objects are simply destroyed.
    if (howMany > 0 && static_cast<int>(matching.size()) > howMany)
        matching.resize(static_cast<std::size_t>(howMany));

    return matching;
}
