// main.cpp - the raylib front end. All game rules live in core.cpp.
#include "raylib.h"
#include "core.h"
#include "win_util.h"

#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <string>

// ---------------------------------------------------------------- startup log

// The window subsystem has no console, so every raylib message goes to run.log.
// Anything fatal also pops a message box: a black window that disappears is
// useless, an error message is not.
static void append_log(const char* text)
{
    FILE* f = fopen("run.log", "a");
    if (f == nullptr) return;
    fputs(text, f);
    fputc('\n', f);
    fclose(f);
}

static void trace_callback(int logLevel, const char* text, va_list args)
{
    char buf[2048];
    vsnprintf(buf, sizeof(buf), text, args);
    append_log(buf);

    if (logLevel == LOG_FATAL || logLevel == LOG_ERROR)
    {
        show_error_box("Read You Like a Book - startup failed", buf);
    }
}

// ---------------------------------------------------------------- palette

static const Color kBg      = {16, 17, 24, 255};
static const Color kPanel   = {28, 30, 42, 255};
static const Color kPanel2  = {38, 41, 56, 255};
static const Color kAccent  = {96, 200, 255, 255};
static const Color kGood    = {84, 220, 150, 255};
static const Color kBad     = {242, 96, 118, 255};
static const Color kText    = {232, 233, 242, 255};
static const Color kDim     = {122, 126, 148, 255};

static const int kWidth  = 1180;
static const int kHeight = 760;

enum class Screen
{
    Title,
    Name,
    Play,
    Result,
    Board
};

struct App
{
    Screen screen = Screen::Title;
    Profile profile;
    Match match;
    std::string player;
    std::string typed;
    std::string board_path = "data/leaderboard.csv";
    int memory_rounds = 0;   // rounds this machine had already seen before today
    float reveal_timer = 0.0f;
    bool revealing = false;
    bool result_saved = false;
    int rounds_per_match = 60;
};

static std::string profile_path(const std::string& name)
{
    std::string safe = name;
    for (int i = 0; i < (int)safe.size(); i++)
    {
        char c = safe[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || c == ',')
        {
            safe[i] = '_';
        }
    }
    return "data/players/" + safe + ".txt";
}

// ---------------------------------------------------------------- drawing helpers

static void centered(const char* text, int y, int size, Color color)
{
    int w = MeasureText(text, size);
    DrawText(text, (kWidth - w) / 2, y, size, color);
}

// ---------------------------------------------------------------- screens

static void draw_title(App& app)
{
    ClearBackground(kBg);

    centered("READ YOU LIKE A BOOK", 180, 64, kText);
    centered("five models watch you play rock-paper-scissors", 270, 24, kDim);

    centered("press ENTER to start", 420, 28, kAccent);
    centered("R / P / S to play, ESC to quit", 470, 20, kDim);

    DrawRectangle(0, 600, kWidth, 2, kPanel2);
    centered("a machine learning demo with no machine learning in it", 630, 20, kDim);

    if (IsKeyPressed(KEY_ENTER))
    {
        app.screen = Screen::Name;
    }
}

static void draw_name(App& app)
{
    ClearBackground(kBg);

    centered("WHO IS PLAYING?", 140, 44, kText);
    centered("your name is the key to this machine's memory", 210, 20, kDim);

    int box_w = 620;
    int box_x = (kWidth - box_w) / 2;
    Rectangle box = {(float)box_x, 290.0f, (float)box_w, 90.0f};
    DrawRectangleRounded(box, 0.25f, 8, kPanel);
    DrawRectangleLinesEx(box, 2.0f, kAccent);

    std::string shown = app.typed;
    if (shown.empty()) shown = "type your name...";
    Color c = kText;
    if (app.typed.empty()) c = kDim;
    DrawText(shown.c_str(), box_x + 28, 318, 36, c);

    centered("ENTER to continue", 430, 24, kDim);

    int ch = GetCharPressed();
    while (ch > 0)
    {
        if (ch >= 32 && ch < 127 && ch != ',' && app.typed.size() < 18)
        {
            app.typed.push_back((char)ch);
        }
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !app.typed.empty())
    {
        app.typed.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER) && !app.typed.empty())
    {
        app.player = app.typed;

        app.profile.reset();
        app.profile.load(profile_path(app.player));
        app.memory_rounds = app.profile.rounds;

        app.match.new_match(app.rounds_per_match, &app.profile);
        app.match.begin_round();
        app.revealing = false;
        app.reveal_timer = 0.0f;
        app.result_saved = false;
        app.screen = Screen::Play;
    }
}

static Rectangle move_button_rect(int index)
{
    return {(float)60, (float)(170 + index * 150), 420.0f, 120.0f};
}

static void draw_play(App& app)
{
    ClearBackground(kBg);

    // header
    DrawText("READ YOU LIKE A BOOK", 40, 28, 28, kText);
    char buf[128];
    int shown_round = app.match.round_no() + 1;
    if (shown_round > app.match.total_rounds()) shown_round = app.match.total_rounds();
    snprintf(buf, sizeof(buf), "ROUND %d / %d", shown_round, app.match.total_rounds());
    int w = MeasureText(buf, 28);
    DrawText(buf, kWidth - 40 - w, 28, 28, kAccent);

    snprintf(buf, sizeof(buf), "you %d   machine %d   draw %d", app.match.player_wins(), app.match.ai_wins(), app.match.draws());
    DrawText(buf, 40, 70, 20, kDim);

    if (app.memory_rounds > 0)
    {
        snprintf(buf, sizeof(buf), "MEMORY: this machine has already watched you for %d rounds", app.memory_rounds);
        DrawText(buf, 40, 98, 20, kGood);
    }

    DrawLine(40, 135, kWidth - 40, 135, kPanel2);

    // move buttons
    const char* names[3] = {"ROCK  [R]", "PAPER  [P]", "SCISSORS  [S]"};
    for (int i = 0; i < 3; i++)
    {
        Rectangle r = move_button_rect(i);
        bool hot = CheckCollisionPointRec(GetMousePosition(), r);
        Color bg = kPanel;
        if (hot || app.revealing) bg = kPanel2;
        DrawRectangleRounded(r, 0.18f, 8, bg);

        Color edge = kPanel2;
        if (app.revealing && (int)app.match.last_human() == i) edge = kAccent;
        DrawRectangleLinesEx(r, 2.0f, edge);

        DrawText(names[i], (int)r.x + 30, (int)r.y + 40, 32, kText);
    }

    // model panel
    int px = 540;
    int py = 170;
    int pw = kWidth - px - 40;
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 430.0f}, 0.06f, 8, kPanel);
    DrawText("MODELS", px + 26, py + 18, 22, kDim);

    const std::vector<PredictorInfo>& preds = app.match.predictions();
    for (int i = 0; i < (int)preds.size(); i++)
    {
        int y = py + 60 + i * 70;
        int pct = 0;
        if (preds[i].trials > 0) pct = preds[i].hits * 100 / preds[i].trials;

        bool is_best = (i == app.match.best_index());
        Color name_color = kText;
        if (is_best) name_color = kAccent;

        DrawText(preds[i].name.c_str(), px + 26, y, 20, name_color);

        snprintf(buf, sizeof(buf), "%3d%%", pct);
        int bw = MeasureText(buf, 20);
        DrawText(buf, px + pw - 26 - bw, y, 20, kText);

        Rectangle track = {(float)(px + 26), (float)(y + 30), (float)(pw - 120), 10.0f};
        DrawRectangleRounded(track, 0.5f, 4, kPanel2);

        Rectangle fill = track;
        fill.width = track.width * (float)pct / 100.0f;
        if (fill.width > 2.0f)
        {
            Color fc = kDim;
            if (is_best) fc = kAccent;
            DrawRectangleRounded(fill, 0.5f, 4, fc);
        }

        snprintf(buf, sizeof(buf), "guess: %s", move_name(preds[i].prediction));
        DrawText(buf, px + 26, y + 44, 16, kDim);
    }

    // footer / result line
    if (app.revealing)
    {
        snprintf(buf, sizeof(buf), "YOU %s   MACHINE %s", move_name(app.match.last_human()), move_name(app.match.ai_move()));
        centered(buf, 640, 32, kText);

        const char* verdict = "DRAW";
        Color vc = kDim;
        if (app.match.last_result() > 0) { verdict = "YOU WIN THIS ROUND"; vc = kGood; }
        if (app.match.last_result() < 0) { verdict = "THE MACHINE CALLED IT"; vc = kBad; }
        centered(verdict, 690, 26, vc);
    }
    else
    {
        centered("pick a move", 680, 26, kDim);
    }

    // input
    if (!app.revealing)
    {
        Move chosen = ROCK;
        bool picked = false;

        if (IsKeyPressed(KEY_R)) { chosen = ROCK; picked = true; }
        if (IsKeyPressed(KEY_P)) { chosen = PAPER; picked = true; }
        if (IsKeyPressed(KEY_S)) { chosen = SCISSORS; picked = true; }

        for (int i = 0; i < 3; i++)
        {
            Rectangle r = move_button_rect(i);
            if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                chosen = (Move)i;
                picked = true;
            }
        }

        if (picked)
        {
            app.match.submit(chosen);
            app.revealing = true;
            app.reveal_timer = 0.85f;
        }
    }
    else
    {
        app.reveal_timer -= GetFrameTime();
        if (app.reveal_timer <= 0.0f)
        {
            app.revealing = false;
            if (app.match.finished())
            {
                app.result_saved = false;
                app.screen = Screen::Result;
            }
            else
            {
                app.match.begin_round();
            }
        }
    }
}

static void draw_result(App& app)
{
    ClearBackground(kBg);

    int pct = app.match.best_hit_percent();

    centered("MATCH OVER", 110, 40, kText);

    char buf[160];
    snprintf(buf, sizeof(buf), "the best model read you %d%% of the time", pct);
    centered(buf, 190, 30, kAccent);

    snprintf(buf, sizeof(buf), "you %d   machine %d   draw %d", app.match.player_wins(), app.match.ai_wins(), app.match.draws());
    centered(buf, 245, 22, kDim);

    const char* verdict = "UNREADABLE. the machine never got a handle on you.";
    Color vc = kGood;
    if (pct >= 50) { verdict = "PREDICTABLE. you have a tell."; vc = kBad; }
    else if (pct >= 40) { verdict = "SOMEWHAT READABLE. a pattern is forming."; vc = kAccent; }
    centered(verdict, 320, 24, vc);

    // save score + memory, exactly once when the result screen is entered
    if (!app.result_saved && app.match.round_no() > 0)
    {
        app.result_saved = true;
        std::vector<BoardEntry> board = load_board(app.board_path);
        bool found = false;
        for (int i = 0; i < (int)board.size(); i++)
        {
            if (board[i].name == app.player)
            {
                if (pct > board[i].score) board[i].score = pct;
                board[i].rounds += app.match.round_no();
                found = true;
            }
        }
        if (!found)
        {
            BoardEntry e;
            e.name = app.player;
            e.score = pct;
            e.rounds = app.match.round_no();
            board.push_back(e);
        }
        save_board(app.board_path, board);
        app.profile.save(profile_path(app.player));
    }

    centered("ENTER: play again      L: leaderboard      ESC: quit", 470, 22, kDim);

    if (IsKeyPressed(KEY_ENTER))
    {
        app.match.new_match(app.rounds_per_match, &app.profile);
        app.match.begin_round();
        app.revealing = false;
        app.reveal_timer = 0.0f;
        app.memory_rounds = app.profile.rounds;
        app.screen = Screen::Play;
    }
    if (IsKeyPressed(KEY_L))
    {
        app.screen = Screen::Board;
    }
}

static void draw_board(App& app)
{
    ClearBackground(kBg);

    centered("EASIEST TO READ", 80, 40, kText);
    centered("highest hit rate wins", 135, 20, kDim);

    std::vector<BoardEntry> board = load_board(app.board_path);

    int y = 200;
    if (board.empty())
    {
        centered("nobody has played yet", y, 26, kDim);
    }

    for (int i = 0; i < (int)board.size() && i < 10; i++)
    {
        char buf[160];
        snprintf(buf, sizeof(buf), "%2d.  %-16s  %3d%%   %d rounds", i + 1, board[i].name.c_str(), board[i].score, board[i].rounds);
        Color c = kText;
        if (i == 0) c = kAccent;
        DrawText(buf, 240, y, 26, c);
        y += 42;
    }

    centered("ESC: back", 690, 20, kDim);

    if (IsKeyPressed(KEY_ESCAPE))
    {
        app.screen = Screen::Result;
    }
}

// ---------------------------------------------------------------- main

int main()
{
    FILE* log = fopen("run.log", "w");
    if (log != nullptr)
    {
        fprintf(log, "started, raylib %s\n", RAYLIB_VERSION);
        fclose(log);
    }

    SetTraceLogCallback(trace_callback);
    SetTraceLogLevel(LOG_ALL);
    append_log("calling InitWindow");

    std::error_code ec;
    std::filesystem::create_directories("data/players", ec);

    InitWindow(kWidth, kHeight, "Read You Like a Book");
    SetTargetFPS(60);

    append_log(TextFormat("InitWindow returned, window ready = %d", IsWindowReady() ? 1 : 0));

    App app;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (app.screen == Screen::Board)
            {
                app.screen = Screen::Result;
            }
            else
            {
                break;
            }
        }

        BeginDrawing();

        if (app.screen == Screen::Title) draw_title(app);
        else if (app.screen == Screen::Name) draw_name(app);
        else if (app.screen == Screen::Play) draw_play(app);
        else if (app.screen == Screen::Result) draw_result(app);
        else if (app.screen == Screen::Board) draw_board(app);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
