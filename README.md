# Chopsticks

More about the rule of the game can be found [here](https://en.wikipedia.org/wiki/Chopsticks_(hand_game)).

## Build

Simply compile the `main.cpp` with the `include` and `src` directories and you are ready to go!

The code uses `windows.h` and other Windows API tools. It is recommended you run the project on Windows only.

## How to play

- To make a *striking* move, start by pressing the first `L` or `R` key, then same for the following second key, which executes a `{first key}`-side-attacking-`{second-key}`-side move
- To make a *split* move, start with `S`, then followed by `L` or `R` key, meaning the side that will get an **increase** after splitting, then choose the transferring amount using Up/Down arrows
- To go back if you make any mistake, simply press Backspace
- To proceed, press ENTER
- Other instructions are provided within the application

## Customize rules

You can switch/change the `static` variables in the file [State.hpp](include/State.hpp) for your own rules/variants. Supported parameters:

```
static const short white_left_hand_max  = 5;
static const short white_right_hand_max = 5;
static const short black_left_hand_max  = 5;
static const short black_right_hand_max = 5;
static const short white_split_max = -1; // negative value for unlimited splits
static const short black_split_max = -1; // negative value for unlimited splits

static const bool splits_as_moves = true;
static const bool allow_sacrifical_splits = false;
static const bool allow_regenerative_splits = true;
```

## Issues

You tell me! :slightly_smiling_face:

## License

This project is licensed under [Apache License 2.0](LICENSE). All rights reserved.
