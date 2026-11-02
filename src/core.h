// core.h - game logic, no graphics, no raylib.
// Everything here can be tested from a plain console program.
#pragma once

#include <string>
#include <vector>

enum Move
{
    ROCK = 0,
    PAPER = 1,
    SCISSORS = 2
};

constexpr int MOVE_COUNT = 3;
constexpr int NO_MOVE = -1;
constexpr int PREDICTOR_COUNT = 5;

// "what beats m", e.g. move_beats(ROCK) == PAPER
Move move_beats(Move m);

// true if a beats b
bool move_wins(Move a, Move b);

const char* move_name(Move m);

// How one predictor performed during this session.
struct PredictorInfo
{
    std::string name;
    Move prediction = ROCK;
    int hits = 0;
    int trials = 0;
};

// Everything the models have learned about one player.
// Saved to disk so the machine "remembers" you next time.
struct Profile
{
    int freq[MOVE_COUNT] = {};                              // P(next)
    int m1[MOVE_COUNT][MOVE_COUNT] = {};                    // P(next | prev1)
    int m2[MOVE_COUNT * MOVE_COUNT][MOVE_COUNT] = {};       // P(next | prev2, prev1)
    int after_win[MOVE_COUNT][MOVE_COUNT] = {};             // P(next | prev1, prev round won)
    int after_loss[MOVE_COUNT][MOVE_COUNT] = {};            // P(next | prev1, prev round lost/drawn)
    int rounds = 0;

    void reset();
    void observe(Move next, int prev1, int prev2, int prev_result);

    bool save(const std::string& path) const;
    bool load(const std::string& path);
};

// A single match: N rounds, five models watching, AI plays the counter.
class Match
{
public:
    // profile is borrowed, not owned. It keeps learning across matches.
    void new_match(int total_rounds, Profile* profile);
    void begin_round();
    void submit(Move human);

    int round_no() const { return round_no_; }
    int total_rounds() const { return total_rounds_; }
    bool finished() const { return round_no_ >= total_rounds_; }

    const std::vector<PredictorInfo>& predictions() const { return preds_; }
    int best_index() const { return best_; }
    Move ai_move() const;
    Move last_human() const { return last_human_; }
    int last_result() const { return last_result_; } // +1 player won, 0 draw, -1 player lost

    int player_wins() const { return player_wins_; }
    int ai_wins() const { return ai_wins_; }
    int draws() const { return draws_; }

    // Highest hit rate any model reached, in percent (0..100).
    int best_hit_percent() const;

private:
    Profile* profile_ = nullptr;
    int total_rounds_ = 60;
    int round_no_ = 0;

    int player_wins_ = 0;
    int ai_wins_ = 0;
    int draws_ = 0;

    Move last_human_ = ROCK;
    int last_result_ = 0;
    int best_ = 0;

    std::vector<PredictorInfo> preds_;
    std::vector<Move> human_history_;
    std::vector<int> result_history_;
};

// Leaderboard: who is easiest to read.
struct BoardEntry
{
    std::string name;
    int score = 0;   // best hit percent
    int rounds = 0;
};

std::vector<BoardEntry> load_board(const std::string& path);
void save_board(const std::string& path, std::vector<BoardEntry> entries);

// Small helpers used by both the game and the tests.
bool row_empty(const int* row, int n);
int argmax_row(const int* row, int n);
