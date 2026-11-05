#!/usr/bin/env python3
"""Writes the Circus Seal music as ZMT (Zeal Music Tracker) files in assets/.

All tracks are original. Each pattern is 32 sixteenth steps on up to 3 voices; voice 4 is left
to the sound effects. A track holds at most 3 patterns (the game keeps 3 pattern buffers in the dead tileset blob).
Step tokens: '.' hold, '-' note off, 'C5' / 'F#4' notes, 'k s h' drum hits on the noise voice.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import zmt, voice  # noqa: E402

OUT = os.path.join(ROOT, 'assets')

# "Big Top March": the act loop. C major oom-pah with a chromatic run-up, 5 frames a step. A A' A A' B B.
OOMPAH = "k . h . s . h . k . h . s . h . k . h . s . h . k . h . s . s ."
MARCH = [
    [   # A: C | F G
        voice("square", 50, "E5 . F5 . F#5 . G5 . . . E5 . C5 . . . A5 . G5 . F#5 . G5 . E5 . C5 . D5 . . ."),
        voice("triangle", 100, "C3 . . . G2 . . . C3 . . . G2 . . . F2 . . . C3 . . . G2 . . . B2 . . ."),
        voice("noise", 25, OOMPAH),
        None,
    ],
    [   # A': G7 | C
        voice("square", 50, "D5 . E5 . F5 . G5 . . . D5 . B4 . . . G5 . F5 . E5 . D5 . C5 . . . . . - ."),
        voice("triangle", 100, "G2 . . . D3 . . . G2 . . . D3 . . . G2 . . . B2 . . . C3 . . . G2 . . ."),
        voice("noise", 25, OOMPAH),
        None,
    ],
    [   # B: F | C G, drum fill
        voice("square", 50, "A5 . . . C6 . A5 . G5 . . . F5 . . . E5 . F5 . G5 . A5 . G5 . . . - . . ."),
        voice("triangle", 100, "F2 . . . C3 . . . F2 . . . C3 . . . C3 . . . G2 . . . C3 . . . G2 . . ."),
        voice("noise", 25, "k . h . s . h . k . h . s . h . k . k . s . k . s s s s k . . ."),
        None,
    ],
]

# Title: "Under the Big Top", a slower stroll in F, 7 frames a step.
TITLE = [
    [
        voice("triangle", 100, "C5 . . . F5 . . . A5 . . . . . G5 . F5 . . . A5 . . . C6 . . . . . - ."),
        voice("square", 25, "F3 A3 C4 A3 F3 A3 C4 A3 F3 A3 C4 A3 F3 A3 C4 A3 C3 E3 G3 E3 C3 E3 G3 E3 F3 A3 C4 A3 F3 A3 C4 A3"),
        voice("noise", 0, "k . . . h . . . k . . . h . . . k . . . h . . . k . . . h . . ."),
        None,
    ],
    [
        voice("triangle", 100, "D6 . . . C6 . . . A5 . . . . . G5 . F5 . . . G5 . . . F5 . . . . . - ."),
        voice("square", 25, "Bb2 D3 F3 D3 Bb2 D3 F3 D3 F3 A3 C4 A3 F3 A3 C4 A3 C3 E3 G3 E3 C3 E3 G3 E3 F3 A3 C4 A3 F3 C4 F4 . "),
        voice("noise", 0, "k . . . h . . . k . . . h . . . k . . . h . . . k . . . h . . ."),
        None,
    ],
]

# Jingles, one pattern each; the game stops a one-shot when its pattern wraps.
READY = [[   # 4 frames a step: drum roll, then ta-da
    voice("square", 50, ". . . . . . . . . . . . . . . . . . . . C5 . E5 G5 C6 . . . . . - ."),
    voice("triangle", 100, "G2 . . . . . . . G2 . . . . . . . G2 . . . C3 . . . C3 . . . . . - ."),
    voice("noise", 25, "s s s s s s s s s s s s s s s s s s s s k . . . k . . . . . . ."),
    None,
]]
CLEAR = [[   # 5 frames a step
    voice("square", 50, "G5 . G5 . G5 . C6 . . . E6 . . . D6 . C6 . D6 . E6 . . . C6 . . . . . - ."),
    voice("triangle", 100, "C3 . . . E3 . . . G3 . . . C3 . . . G2 . . . G3 . . . C3 . . . - . . ."),
    voice("noise", 25, "k . h . s . h . k . h . s . s s k . . . s . . . k . . . . . . ."),
    None,
]]
OVER = [[   # 8 frames a step: the sad trombone
    voice("saw", 50, "G4 . . . . . F#4 . . . . . F4 . . . . . E4 . . . . . . . . . . . - ."),
    voice("triangle", 100, "C3 . . . . . B2 . . . . . Bb2 . . . . . A2 . . . . . . . . . . . - ."),
    None,
    None,
]]

TRACKS = {
    "march.zmt": zmt("BIG TOP", 20, MARCH, [0, 1, 0, 1, 2, 2]),
    "title.zmt": zmt("UNDER TENT", 28, TITLE, [0, 1]),
    "ready.zmt": zmt("DRUM ROLL", 16, READY, [0]),
    "clear.zmt": zmt("BRAVO", 20, CLEAR, [0]),
    "over.zmt": zmt("WAH WAH", 32, OVER, [0]),
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, data in TRACKS.items():
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        print(f"{name}: {len(data)} bytes")
