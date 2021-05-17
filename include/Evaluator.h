#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "HashMap.hpp"
#include "State.hpp"
#include "Thread.hpp"
#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define EPSILON 1e-6

#define EVALUATION_DEPTH 24   // depth of minimax evaluation
#define ABS_SCORE        5.0  // winning states are evaluated +ABS_RANGE, losing states -ABS_RANGE
#define SCORE_RANGE      10.0 // scores may not exceed this range at all cost
#define SPLIT_PENALTY    0.2  // maximum penalty for split (if splits are limited)

enum evaluation_state
{
    MOVE_EVALUATED = 2,
    MOVE_EVALUATING = 1,
    MOVE_TO_BE_EVALUATED = 0
};

class move_data
{
public:
    int fparam, sparam;
    bool is_split;

    move_data (int _fparam = 0, int _sparam = 0, bool _is_split = false):
        fparam(_fparam), sparam(_sparam), is_split(_is_split) {}

    std::string get_displayable() const
    {
        if (is_split)
            return std::string("S") +
                   (fparam < 0 ? "R" : fparam == 0 ? "-" : sparam < 0 ? "L" : "-") +
                   (abs(fparam) == abs(sparam) ? std::to_string(abs(fparam)) : "-");
        else
            return std::string("") +
                   (toupper(fparam) == 'L' || toupper(fparam) == 'R' ?
                   (char)toupper(fparam) : '-') +
                   (toupper(sparam) == 'L' || toupper(sparam) == 'R' ?
                   (char)toupper(sparam) : '-');
    }

    static move_data parse_displayable(std::string displayable)
    {
        static const std::string error_msg = "Move parsing failed: Invalid displayable format";
        move_data move;

        for (size_t i = 0; i < displayable.length(); ++i)
            displayable[i] = toupper(displayable[i]);

        if (displayable.length() < 2)
            throw std::runtime_error(error_msg);
        else
        if (displayable.length() == 2)
        {
            if (displayable[0] == 'L')
                move.fparam = 'L';
            else
            if (displayable[0] == 'R')
                move.fparam = 'R';
            else
                throw std::runtime_error(error_msg);

            if (displayable[1] == 'L')
                move.sparam = 'L';
            else
            if (displayable[1] == 'R')
                move.sparam = 'R';
            else
                throw std::runtime_error(error_msg);
        }
        else
        {
            if (displayable[0] != 'S')
                throw std::runtime_error(error_msg);
            if (displayable[1] != 'L' && displayable[1] != 'R')
                throw std::runtime_error(error_msg);

            move.fparam = displayable[1] == 'L' ? 1 : -1;
            move.sparam = displayable[1] == 'R' ? 1 : -1;

            const int change = std::stoi(displayable.substr(2));
            move.fparam *= change;
            move.sparam *= change;
            move.is_split = true;
        }

        if (!move.is_valid())
            throw std::runtime_error(error_msg);

        return move;
    }

    bool is_valid() const
    {
        return is_split ? (fparam != 0 && fparam == -sparam) :
                         ((toupper(fparam) == 'L' || toupper(fparam) == 'R') &&
                          (toupper(sparam) == 'L' || toupper(sparam) == 'R'));
    }
};

class node_data
{
public:
    double score = 0;
    int evaluated_depth = 0;
    move_data best_move;
};

class Evaluator
{
private:
    class evaluating_node_data : public node_data
    {
    public:
        double alpha = -SCORE_RANGE, beta = SCORE_RANGE;
        Thread::HashMap<std::string, evaluation_state> moves;
        bool evaluated_marker = false;
    };

    Thread::HashMap<int, evaluating_node_data> table;
    Thread::HashMap<std::string, bool> in_stack;
    bool evaluated_marker = false;
    Thread::ThreadPool Pool;
    int branch_id_counter;
    Thread::Atomic<size_t> state_evaluated;

    void calculate_original_score (state current, evaluating_node_data &node);
    void after_search (std::tuple<move_data, state, int> st,
                       evaluating_node_data &node,
                       int depth,
                       bool maximizing,
                       double &alpha,
                       double &beta);
    void search(state current,
                std::vector<int> branch = { 0 },
                int depth = EVALUATION_DEPTH,
                double alpha = -ABS_SCORE,
                double beta = ABS_SCORE,
                bool maximizing = true);

public:
    node_data get_node_data(int hash_state);
    node_data get_node_data(state game_state);
    void evaluate_next_move(int hash_state);
    void evaluate_next_move(state game_state);
    size_t get_last_number_of_evaluated_states();
};

#endif // EVALUATOR_H
