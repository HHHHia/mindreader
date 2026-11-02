// core.cpp - game logic only. No raylib in here on purpose.
#include "core.h"

#include <cstdio>
#include <fstream>
#include <sstream>

Move move_beats(Move m)
{
    return (Move)((m + 1) % MOVE_COUNT);
}

bool move_wins(Move a, Move b)
{
    return ((a - b + MOVE_COUNT) % MOVE_COUNT) == 1;
}

const char* move_name(Move m)
{
    if (m == ROCK) return "ROCK";
    if (m == PAPER) return "PAPER";
    return "SCISSORS";
}

bool row_empty(const int* row, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (row[i] != 0) return false;
    }
    return true;
}

int argmax_row(const int* row, int n)
{
    int best = 0;
    for (int i = 1; i < n; i++)
    {
        if (row[i] > row[best]) best = i;
    }
    return best;
}

// ---------------------------------------------------------------- Profile

void Profile::reset()
{
    for (int i = 0; i < MOVE_COUNT; i++) freq[i] = 0;
    for (int i = 0; i < MOVE_COUNT; i++)
        for (int j = 0; j < MOVE_COUNT; j++)
            m1[i][j] = 0;
    for (int i = 0; i < MOVE_COUNT * MOVE_COUNT; i++)
        for (int j = 0; j < MOVE_COUNT; j++)
            m2[i][j] = 0;
    for (int i = 0; i < MOVE_COUNT; i++)
        for (int j = 0; j < MOVE_COUNT; j++)
        {
            after_win[i][j] = 0;
            after_loss[i][j] = 0;
        }
    rounds = 0;
}

void Profile::observe(Move next, int prev1, int prev2, int prev_result)
{
    freq[next] += 1;

    if (prev1 != NO_MOVE)
    {
        m1[prev1][next] += 1;

        // prev_result: +1 the player won that round, 0 draw, -1 lost.
        // A draw is counted on the "lost" side: the player did not get a win,
        // and the two counters must stay aligned.
        if (prev_result > 0) after_win[prev1][next] += 1;
        else after_loss[prev1][next] += 1;
    }

    if (prev1 != NO_MOVE && prev2 != NO_MOVE)
    {
        m2[prev2 * MOVE_COUNT + prev1][next] += 1;
    }

    rounds += 1;
}

bool Profile::save(const std::string& path) const
{
    std::ofstream out(path.c_str());
    if (!out) return false;

    out << "PROFILE 1\n";
    out << rounds << "\n";

    for (int i = 0; i < MOVE_COUNT; i++) out << freq[i] << " ";
    out << "\n";

    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++) out << m1[i][j] << " ";
    }
    out << "\n";

    for (int i = 0; i < MOVE_COUNT * MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++) out << m2[i][j] << " ";
    }
    out << "\n";

    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++) out << after_win[i][j] << " ";
    }
    out << "\n";

    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++) out << after_loss[i][j] << " ";
    }
    out << "\n";

    return true;
}

bool Profile::load(const std::string& path)
{
    std::ifstream in(path.c_str());
    if (!in) return false;

    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "PROFILE" || version != 1) return false;

    reset();
    if (!(in >> rounds)) return false;

    for (int i = 0; i < MOVE_COUNT; i++)
    {
        if (!(in >> freq[i])) return false;
    }
    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++)
        {
            if (!(in >> m1[i][j])) return false;
        }
    }
    for (int i = 0; i < MOVE_COUNT * MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++)
        {
            if (!(in >> m2[i][j])) return false;
        }
    }
    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++)
        {
            if (!(in >> after_win[i][j])) return false;
        }
    }
    for (int i = 0; i < MOVE_COUNT; i++)
    {
        for (int j = 0; j < MOVE_COUNT; j++)
        {
            if (!(in >> after_loss[i][j])) return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------- models

static Move predict_freq(const Profile& p)
{
    if (p.rounds == 0) return ROCK;
    return (Move)argmax_row(p.freq, MOVE_COUNT);
}

static Move predict_markov1(const Profile& p, int prev1, Move fallback)
{
    if (prev1 == NO_MOVE) return fallback;
    const int* row = p.m1[prev1];
    if (row_empty(row, MOVE_COUNT)) return fallback;
    return (Move)argmax_row(row, MOVE_COUNT);
}

static Move predict_markov2(const Profile& p, int prev1, int prev2, Move fallback)
{
    if (prev1 == NO_MOVE || prev2 == NO_MOVE) return fallback;
    const int* row = p.m2[prev2 * MOVE_COUNT + prev1];
    if (row_empty(row, MOVE_COUNT)) return fallback;
    return (Move)argmax_row(row, MOVE_COUNT);
}

static Move predict_react(const Profile& p, int prev1, int prev_result, Move fallback)
{
    if (prev1 == NO_MOVE) return fallback;

    const int* row = nullptr;
    if (prev_result > 0) row = p.after_win[prev1];
    else row = p.after_loss[prev1];

    if (row_empty(row, MOVE_COUNT)) return fallback;
    return (Move)argmax_row(row, MOVE_COUNT);
}

// ---------------------------------------------------------------- Match

void Match::new_match(int total_rounds, Profile* profile)
{
    profile_ = profile;
    total_rounds_ = total_rounds;
    round_no_ = 0;
    player_wins_ = 0;
    ai_wins_ = 0;
    draws_ = 0;
    last_human_ = ROCK;
    last_result_ = 0;
    best_ = 0;

    human_history_.clear();
    result_history_.clear();

    preds_.clear();
    preds_.push_back({"BASELINE", ROCK, 0, 0});
    preds_.push_back({"FREQUENCY", ROCK, 0, 0});
    preds_.push_back({"MARKOV-1", ROCK, 0, 0});
    preds_.push_back({"MARKOV-2", ROCK, 0, 0});
    preds_.push_back({"REACT", ROCK, 0, 0});
}

void Match::begin_round()
{
    if (finished() || profile_ == nullptr) return;

    int n = (int)human_history_.size();
    int prev1 = NO_MOVE;
    int prev2 = NO_MOVE;
    if (n >= 1) prev1 = human_history_[n - 1];
    if (n >= 2) prev2 = human_history_[n - 2];

    int prev_result = 0;
    if (!result_history_.empty()) prev_result = result_history_.back();

    Move p_freq = predict_freq(*profile_);
    Move p_m1 = predict_markov1(*profile_, prev1, p_freq);
    Move p_m2 = predict_markov2(*profile_, prev1, prev2, p_m1);
    Move p_react = predict_react(*profile_, prev1, prev_result, p_freq);

    preds_[0].prediction = ROCK;
    preds_[1].prediction = p_freq;
    preds_[2].prediction = p_m1;
    preds_[3].prediction = p_m2;
    preds_[4].prediction = p_react;

    // The model trusted this round is the one that has been right most often.
    best_ = 0;
    int best_hits = -1;
    for (int i = 0; i < (int)preds_.size(); i++)
    {
        if (preds_[i].trials == 0) continue;
        if (preds_[i].hits > best_hits)
        {
            best_hits = preds_[i].hits;
            best_ = i;
        }
    }
}

Move Match::ai_move() const
{
    if (preds_.empty()) return ROCK;
    return move_beats(preds_[best_].prediction);
}

void Match::submit(Move human)
{
    if (finished() || profile_ == nullptr) return;

    int n = (int)human_history_.size();
    int prev1 = NO_MOVE;
    int prev2 = NO_MOVE;
    if (n >= 1) prev1 = human_history_[n - 1];
    if (n >= 2) prev2 = human_history_[n - 2];

    int prev_result = 0;
    if (!result_history_.empty()) prev_result = result_history_.back();

    Move ai = ai_move();

    int result = 0;
    if (move_wins(human, ai)) result = 1;
    else if (move_wins(ai, human)) result = -1;

    for (int i = 0; i < (int)preds_.size(); i++)
    {
        preds_[i].trials += 1;
        if (preds_[i].prediction == human) preds_[i].hits += 1;
    }

    profile_->observe(human, prev1, prev2, prev_result);

    human_history_.push_back(human);
    result_history_.push_back(result);

    last_human_ = human;
    last_result_ = result;

    if (result > 0) player_wins_ += 1;
    else if (result < 0) ai_wins_ += 1;
    else draws_ += 1;

    round_no_ += 1;
}

int Match::best_hit_percent() const
{
    int best = 0;
    for (int i = 0; i < (int)preds_.size(); i++)
    {
        if (preds_[i].trials == 0) continue;
        int pct = preds_[i].hits * 100 / preds_[i].trials;
        if (pct > best) best = pct;
    }
    return best;
}

// ---------------------------------------------------------------- leaderboard

std::vector<BoardEntry> load_board(const std::string& path)
{
    std::vector<BoardEntry> out;
    std::ifstream in(path.c_str());
    if (!in) return out;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string name;
        std::string score;
        std::string rounds;
        if (!std::getline(ss, name, ',')) continue;
        if (!std::getline(ss, score, ',')) continue;
        if (!std::getline(ss, rounds, ',')) continue;

        BoardEntry e;
        e.name = name;
        e.score = atoi(score.c_str());
        e.rounds = atoi(rounds.c_str());
        out.push_back(e);
    }
    return out;
}

void save_board(const std::string& path, std::vector<BoardEntry> entries)
{
    // Highest hit rate first: the person easiest to read wins.
    for (int i = 0; i < (int)entries.size(); i++)
    {
        for (int j = i + 1; j < (int)entries.size(); j++)
        {
            if (entries[j].score > entries[i].score)
            {
                BoardEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    std::ofstream out(path.c_str());
    if (!out) return;
    for (int i = 0; i < (int)entries.size(); i++)
    {
        out << entries[i].name << "," << entries[i].score << "," << entries[i].rounds << "\n";
    }
}
