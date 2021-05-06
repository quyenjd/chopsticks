#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "State.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct move_data
{
    int fparam, sparam;
    bool is_split;

    move_data (int _fparam = 0, int _sparam = 0, bool _is_split = false):
        fparam(_fparam), sparam(_sparam), is_split(_is_split) {}

    std::string get_displayable() const
    {
        if (is_split)
            return std::string("T") +
                   (fparam < 0 ? "R" : fparam == 0 ? "-" : sparam < 0 ? "L" : "-") +
                   (abs(fparam) == abs(sparam) ? std::to_string(abs(fparam)) : "-");
        else
            return std::string("") +
                   (toupper(fparam) == 'L' || toupper(fparam) == 'R' ?
                   (char)toupper(fparam) : '-') +
                   (toupper(sparam) == 'L' || toupper(sparam) == 'R' ?
                   (char)toupper(sparam) : '-');
    }
};

struct node_data
{
    double winning_lines = 0, losing_lines = 0, score = 0;
    bool evaluated = false;
    std::vector<move_data> moves;
};

class Evaluator
{
private:
    std::unordered_map<int, node_data> table;
    std::unordered_map<int, move_data> evaluated_next_moves;
    void update_node_util(node_data *node, state current, state tmp, move_data move, int &move_evaluated);
    void search(int &state_evaluated,
                int &max_depth,
                state current = state(),
                std::unordered_map<int, bool> *in_stack = new std::unordered_map<int, bool>(),
                int depth = 0);

public:
    Evaluator();
    node_data get_node_data(int hash_state) const;
    node_data get_node_data(state game_state) const;
    move_data get_evaluated_next_move(int hash_state);
    move_data get_evaluated_next_move(state game_state);
};

#endif // EVALUATOR_H