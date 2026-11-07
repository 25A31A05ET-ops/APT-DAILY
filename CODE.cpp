#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <chrono>
#include <memory>
#include <iomanip>

// ============================================================================
// DATA STRUCTURES & MODELS
// ============================================================================

struct Question {
    std::string year;     // e.g., "1st Year", "2nd Year", "3rd Year", "4th Year"
    std::string topic;    // e.g., "Percentages", "Time and Work", "Probability"
    std::string level;    // "Beginner", "Intermediate", "Advanced"
    std::string text;
    std::vector<std::string> options;
    int correctOptionIndex; // 0-based index
    std::string explanation;
};

struct AttemptResult {
    std::string date;
    std::string mode; // "daily", "mock", "practice"
    int score;
    int total;
    std::vector<std::pair<Question, int>> userAnswers; // Question and chosen option index
};

struct TopicStats {
    int correct = 0;
    int total = 0;

    double getAccuracy() const {
        return total > 0 ? (static_cast<double>(correct) / total) * 100.0 : 0.0;
    }
};

struct StudentState {
    std::string name = "Student";
    std::string year = "3rd Year";
    std::string goal = "10";
    
    // Hash Maps for O(1) lookups and analytics tracking
    std::unordered_map<std::string, TopicStats> topicStats;
    std::unordered_map<std::string, int> dailyScores; // Format: "YYYY-MM-DD" -> Score
    std::unordered_set<std::string> completedQuestionTexts; // O(1) deduplication set
    
    std::vector<AttemptResult> history;
};

// ============================================================================
// ENGINE / SYSTEM SERVICE
// ============================================================================

class AptiDailyEngine {
private:
    std::vector<Question> questionBank;
    StudentState state;

    void seedQuestionBank() {
        questionBank = {
            // 1st YEAR
            {"1st Year", "Number Systems", "Beginner", "What is the unit digit of (7)^105?", {"1", "3", "7", "9"}, 2, "Cyclicity of 7 is 4. 105 % 4 = 1. So 7^1 = 7."},
            {"1st Year", "Percentages", "Beginner", "If 20% of a number is 50, what is 60% of that number?", {"100", "120", "150", "200"}, 2, "60% = 3 x 20% = 3 x 50 = 150."},
            {"1st Year", "Ratios", "Beginner", "If A : B = 2 : 3 and B : C = 4 : 5, find A : C.", {"8 : 15", "2 : 5", "6 : 15", "8 : 12"}, 0, "A/C = (2/3) * (4/5) = 8/15."},
            
            // 2nd YEAR
            {"2nd Year", "Profit and Loss", "Intermediate", "An article bought for ₹500 is sold for ₹625. Find profit percentage.", {"20%", "25%", "30%", "15%"}, 1, "Profit = 125. (125 / 500) * 100 = 25%."},
            {"2nd Year", "Time and Work", "Intermediate", "A completes work in 10 days, B in 15 days. Together they take?", {"5 days", "6 days", "7.5 days", "8 days"}, 1, "1/10 + 1/15 = 1/6. So 6 days."},
            {"2nd Year", "Speed and Distance", "Intermediate", "Train 120m long crosses pole in 6s. Speed in km/h?", {"60 km/h", "72 km/h", "80 km/h", "90 km/h"}, 1, "Speed = 20 m/s = 20 * (18/5) = 72 km/h."},
            
            // 3rd YEAR
            {"3rd Year", "Permutations & Combinations", "Advanced", "Ways to arrange letters of 'LEADER'?", {"720", "360", "180", "240"}, 1, "6! / 2! = 720 / 2 = 360."},
            {"3rd Year", "Probability", "Advanced", "Two dice thrown. Probability of sum being 7?", {"1/12", "1/6", "1/9", "5/36"}, 1, "6 favorable pairs out of 36. 6/36 = 1/6."},
            {"3rd Year", "Clocks and Calendars", "Advanced", "Angle between clock hands at 3:40?", {"120°", "130°", "140°", "125°"}, 1, "|30(3) - (11/2)(40)| = |90 - 220| = 130°."},
            
            // 4th YEAR
            {"4th Year", "Advanced Quantitative", "Advanced", "A sum amounts to ₹6690 in 3 yrs and ₹10035 in 6 yrs CI. Sum is?", {"₹4460", "₹4500", "₹4680", "₹4800"}, 0, "Ratio (1+r)^3 = 3/2. Sum = 6690 / (3/2) = ₹4460."},
            {"4th Year", "Critical Reasoning", "Advanced", "All engineers know Java. Some know Python. Rajesh doesn't know Python. Therefore?", {"Doesn't know Java", "Not an engineer", "Might still know Java", "Knows C++"}, 2, "Not knowing Python doesn't exclude knowing Java."}
        };
    }

public:
    AptiDailyEngine(const std::string& studentName, const std::string& studyYear) {
        state.name = studentName;
        state.year = studyYear;
        seedQuestionBank();
    }

    // O(1) deduplication & randomized shuffling selection algorithm
    std::vector<Question> generateDailyChallenge(size_t limit = 10) {
        std::vector<Question> pool;
        
        // Filter by student's active curriculum year
        for (const auto& q : questionBank) {
            if (q.year == state.year && state.completedQuestionTexts.find(q.text) == state.completedQuestionTexts.end()) {
                pool.push_back(q);
            }
        }

        // Fallback to all uncompleted or entire bank if pool exhausted
        if (pool.empty()) {
            pool = questionBank;
        }

        // Modern random shuffle using std::mt19937
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(pool.begin(), pool.end(), std::default_random_engine(seed));

        if (pool.size() > limit) {
            pool.resize(limit);
        }
        return pool;
    }

    // Processes quiz answers, updates state, and computes statistics
    void submitQuiz(const std::string& mode, const std::string& dateKey, const std::vector<std::pair<Question, int>>& userSelections) {
        AttemptResult result;
        result.date = dateKey;
        result.mode = mode;
        result.total = static_cast<int>(userSelections.size());
        result.score = 0;
        result.userAnswers = userSelections;

        for (const auto& item : userSelections) {
            const Question& q = item.first;
            int chosenOpt = item.second;

            // Track completed question text in set (O(1) insertion)
            state.completedQuestionTexts.insert(q.text);

            bool isCorrect = (chosenOpt == q.correctOptionIndex);
            if (isCorrect) {
                result.score++;
            }

            // Update Topic Map stats
            auto& tStat = state.topicStats[q.topic];
            tStat.total++;
            if (isCorrect) {
                tStat.correct++;
            }
        }

        if (mode == "daily") {
            state.dailyScores[dateKey] = result.score;
        }

        state.history.push_back(result);
    }

    // Displays overall progress analytics
    void printDashboard() const {
        int totalQuestions = 0;
        int totalCorrect = 0;

        for (const auto& attempt : state.history) {
            totalQuestions += attempt.total;
            totalCorrect += attempt.score;
        }

        double overallAccuracy = totalQuestions > 0 ? (static_cast<double>(totalCorrect) / totalQuestions) * 100.0 : 0.0;

        std::cout << "\n==================================================\n";
        std::cout << "          APTIDAILY STUDENT DASHBOARD             \n";
        std::cout << "==================================================\n";
        std::cout << " Student Name    : " << state.name << "\n";
        std::cout << " Active Year     : " << state.year << "\n";
        std::cout << " Total Solved    : " << totalQuestions << " questions\n";
        std::cout << " Total Correct   : " << totalCorrect << "\n";
        std::cout << " Overall Accuracy: " << std::fixed << std::setprecision(1) << overallAccuracy << "%\n";
        std::cout << " Daily Challenges: " << state.dailyScores.size() << " days logged\n";
        std::cout << "--------------------------------------------------\n";
        std::cout << " TOPIC ACCURACY BREAKDOWN:\n";
        for (const auto& [topic, stats] : state.topicStats) {
            std::cout << "  • " << std::left << std::setw(28) << topic 
                      << ": " << stats.correct << "/" << stats.total 
                      << " (" << std::fixed << std::setprecision(1) << stats.getAccuracy() << "%)\n";
        }
        std::cout << "==================================================\n\n";
    }
};

// ============================================================================
// MAIN CLI RUNTIME
// ============================================================================

int main() {
    // Instantiate engine for a 3rd-year student
    AptiDailyEngine engine("Vasamsetti Prudhvi", "3rd Year");

    std::cout << "Initializing AptiDaily C++ Core Placement Engine...\n";

    // 1. Generate daily challenge set
    auto dailySet = engine.generateDailyChallenge(3);

    // 2. Simulate user taking the quiz
    std::vector<std::pair<Question, int>> userAnswers;
    
    std::cout << "\n--- Daily Challenge Started (" << dailySet.size() << " Questions) ---\n";
    for (size_t i = 0; i < dailySet.size(); ++i) {
        const auto& q = dailySet[i];
        std::cout << "\nQ" << (i + 1) << " [" << q.topic << "]: " << q.text << "\n";
        for (size_t optIdx = 0; optIdx < q.options.size(); ++optIdx) {
            std::cout << "  " << static_cast<char>('A' + optIdx) << ") " << q.options[optIdx] << "\n";
        }
        
        // Simulating selecting option index 1 for demo
        int selectedOption = 1; 
        std::cout << "Selected Answer: " << static_cast<char>('A' + selectedOption) << "\n";
        userAnswers.push_back({q, selectedOption});
    }

    // 3. Submit quiz results
    engine.submitQuiz("daily", "2026-11-07", userAnswers);

    // 4. Print updated system dashboard analytics
    engine.printDashboard();

    return 0;
}
