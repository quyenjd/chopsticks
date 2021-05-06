#ifndef UI_H
#define UI_H

#include "State.hpp"
#include "Evaluator.h"
#include <vector>

class UI
{
private:
    state game_state;
    std::vector<move_data> moves;
    static char menu();
    static void game(Evaluator *evaluator, bool white_turn);

public:
    UI();
    UI(state game);
    state get_game_state() const;
    std::vector<move_data> get_moves() const;
    void make_move(move_data data);
    static void run();
};

#endif // UI_H
