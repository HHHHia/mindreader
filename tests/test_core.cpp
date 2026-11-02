// test_core.cpp - console test for the model layer. No graphics involved.
#include "../src/core.h"

#include <cstdio>

static unsigned g_seed = 20261102u;
static int rnd(int n)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (int)((g_seed >> 16) % (unsigned)n);
}

static void report(const char* title, Match& m)
{
    printf("\n== %s ==\n", title);
    const std::vector<PredictorInfo>& p = m.predictions();
    for (int i = 0; i < (int)p.size(); i++)
    {
        int pct = 0;
        if (p[i].trials > 0) pct = p[i].hits * 100 / p[i].trials;
        printf("  %-10s hits %4d / %4d  = %3d%%\n", p[i].name.c_str(), p[i].hits, p[i].trials, pct);
    }
    printf("  running best: %d%%\n", m.best_hit_percent());
}

int main()
{
    const int ROUNDS = 300;

    // 1) a player who just cycles ROCK -> PAPER -> SCISSORS
    {
        Profile prof;
        prof.reset();
        Match m;
        m.new_match(ROUNDS, &prof);
        for (int i = 0; i < ROUNDS; i++)
        {
            m.begin_round();
            Move human = (Move)(i % 3);
            m.submit(human);
        }
        report("cycle R->P->S", m);
    }

    // 2) a player who plays uniformly at random
    {
        Profile prof;
        prof.reset();
        Match m;
        m.new_match(ROUNDS, &prof);
        for (int i = 0; i < ROUNDS; i++)
        {
            m.begin_round();
            Move human = (Move)rnd(3);
            m.submit(human);
        }
        report("uniform random", m);
    }

    // 3) a player who repeats a winning move and switches to the
    //    move that beats what just beat them
    {
        Profile prof;
        prof.reset();
        Match m;
        m.new_match(ROUNDS, &prof);
        Move last = ROCK;
        int last_result = 0;
        for (int i = 0; i < ROUNDS; i++)
        {
            m.begin_round();
            Move human = last;
            if (last_result < 0)
            {
                human = move_beats(move_beats(last));
            }
            m.submit(human);
            last_result = m.last_result();
            last = human;
        }
        report("win-stay / lose-shift", m);
    }

    // 4) profile save + load round trip
    {
        Profile a;
        a.reset();
        a.observe(PAPER, ROCK, NO_MOVE, 0);
        a.observe(SCISSORS, PAPER, ROCK, 1);
        a.save("_profile_test.txt");

        Profile b;
        bool ok = b.load("_profile_test.txt");
        bool same = ok && b.rounds == a.rounds && b.freq[1] == 1 && b.freq[2] == 1;
        printf("\n== profile save/load ==\n");
        printf("  loaded=%d rounds=%d freq=%d,%d,%d -> %s\n", ok ? 1 : 0, b.rounds,
               b.freq[0], b.freq[1], b.freq[2], same ? "OK" : "MISMATCH");
        remove("_profile_test.txt");
    }

    // 5) leaderboard ordering
    {
        std::vector<BoardEntry> e;
        e.push_back({"alice", 41, 60});
        e.push_back({"bob", 58, 60});
        e.push_back({"carol", 33, 60});
        save_board("_board_test.csv", e);
        std::vector<BoardEntry> back = load_board("_board_test.csv");
        printf("\n== leaderboard ==\n");
        printf("  rows=%d first=%s(%d)\n", (int)back.size(), back[0].name.c_str(), back[0].score);
        remove("_board_test.csv");
    }

    return 0;
}
