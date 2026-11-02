# Read You Like a Book

A demo about how a machine learns to read you: you play rock-paper-scissors against the computer while
**five prediction models watch every move**. A match is 60 rounds. At the end it tells you how often the
best model read you correctly, and files your readability onto a leaderboard — the easier you are to
predict, the higher you rank.

中文版：[README.md](README.md)

There is no neural network in here, only counting: what you play, what you play after a given move,
what you do after winning and after losing. Yet over 300 rounds it reads a fixed pattern (an R→P→S cycle,
"win and repeat") at 95–99%, while a truly random player stays around 30%.
**The gap is not in the machine, it is in the player being read.**

## How to play

Double-click `mindreader.exe` in this folder (or run `mindreader.exe` from a shell opened here).

| Screen      | Controls                                                       |
| ----------- | -------------------------------------------------------------- |
| Title       | `ENTER` to start                                               |
| Name        | type a name (≤18 chars, no `,`), `ENTER` to confirm            |
| Play        | `R` / `P` / `S`, or click one of the three buttons on the left |
| Result      | `ENTER` play again　`L` leaderboard　`ESC` quit                |
| Leaderboard | `ESC` back to result                                           |

- After each move the game pauses 0.85 s to show "you played / the machine played / who won", then
  advances to the next round automatically.
- The **MODELS** panel on the right is live: each model's hit rate so far and what it is guessing
  for this round. The highlighted row is the model the machine is actually trusting right now.
- **Your name is the save key.** Play again with the same name and the machine continues from what it
  already knows — the play screen then shows
  `MEMORY: this machine has already watched you for N rounds`.

## The five models

| Model     | What it assumes                             | How it guesses                                                       |
| --------- | ------------------------------------------- | -------------------------------------------------------------------- |
| BASELINE  | nothing                                     | always ROCK, the 33% anchor                                          |
| FREQUENCY | you repeat whatever you play most           | your most played move                                                |
| MARKOV-1  | your next move depends on your last         | most common move after "played X"                                    |
| MARKOV-2  | it depends on your last two                 | most common move after "played X then Y"                             |
| REACT     | winning and losing bring out different yous | counted separately for "after X and won" / "after X and did not win" |

**How the machine moves**: each round every model makes a guess; the machine picks one of them to go by
and plays the counter to it.

**How you are scored**: the result screen shows the **best hit rate** of any model (hits / trials).

| Hit rate | Verdict                                              |
| -------- | ---------------------------------------------------- |
| ≥ 50%    | `PREDICTABLE. you have a tell.`                      |
| 40–49%   | `SOMEWHAT READABLE. a pattern is forming.`           |
| < 40%    | `UNREADABLE. the machine never got a handle on you.` |

## Where the data lives

| File                      | Contents                                                                                       | Overwritten                       |
| ------------------------- | ---------------------------------------------------------------------------------------------- | --------------------------------- |
| `data/players/<name>.txt` | that player's full count matrices (`PROFILE 1` format), carried across matches and never reset | only once, on the result screen   |
| `data/leaderboard.csv`    | `name,best hit percent,total rounds`, sorted by hit rate                                       | updated at the end of every match |
| `run.log`                 | raylib log of the most recent launch                                                           | **rewritten on every launch**     |
| `data/`, `data/players/`  | created on first run                                                                           | —                                 |

To start over from scratch: close the game and delete `data/players/<your name>.txt`.
