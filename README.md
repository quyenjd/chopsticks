# Chopsticks

More about the rules of the game can be found [here](https://en.wikipedia.org/wiki/Chopsticks_(hand_game)).

## Build

Check the file [Chopsticks.cbp](Chopsticks.cbp) for build options. *Yes, I use CodeBlocks!*

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
static const bool allow_sacrifical_splits = true;
static const bool allow_regenerative_splits = true;
static const bool meta_variant = false;
```

## Issues

```
terminate called after throwing an instance of 'std::system_error'
  what():  Operation not permitted
```

This occurred whenever I started the multi-threading process of the evaluation. Even commenting the evaluation part (inside the `Evaluator::search` method) entirely still gives the error. Solutions such as `-lpthread`, `-pthread`, `-Wl,--no-as-needed` have been tested but **no working**. Any help is appreciated.

## License

This project is licensed under [Apache License 2.0](LICENSE). All rights reserved.
