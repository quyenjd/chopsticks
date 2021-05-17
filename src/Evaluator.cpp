#include "Evaluator.h"
#include <algorithm>
#include <conio.h>
#include <iostream>
#include <math.h>
#include <stdexcept>
#include <thread>

void Evaluator::calculate_original_score (state current, evaluating_node_data &node)
{
    node.score = SPLIT_PENALTY * (
                     (current.white_split_max > 0 ? ((double)current.white_split / current.white_split_max) : 1.0)
                 -
                     (current.black_split_max > 0 ? ((double)current.black_split / current.black_split_max) : 1.0)
                 );
}

void Evaluator::after_search (std::tuple<move_data, state, int> st,
                              evaluating_node_data &node,
                              int depth,
                              bool maximizing,
                              double &alpha,
                              double &beta)
{
    table[std::get<1>(st).get_hash()].mutate([&](evaluating_node_data &tmp_node) {
        if (maximizing)
        {
            if (-node.score + tmp_node.score > EPSILON)
            {
                node.score = tmp_node.score;
                node.evaluated_depth = depth;
                node.best_move = std::get<0>(st);
            }

            alpha = std::max(alpha, node.score);
        }
        else
        {
            if (-node.score + tmp_node.score < -EPSILON)
            {
                node.score = tmp_node.score;
                node.evaluated_depth = depth;
                node.best_move = std::get<0>(st);
            }

            beta = std::min(beta, node.score);
        }

        node.alpha = alpha;
        node.beta = beta;
    });
}

void Evaluator::search(state current,
                       std::vector<int> branch,
                       int depth,
                       double alpha,
                       double beta,
                       bool maximizing)
{
    // reset
    if (depth == EVALUATION_DEPTH)
    {
        in_stack.clear();
        state_evaluated.set(branch_id_counter = 0);
        maximizing = current.white_turn;
    }

    // invalid state or branch
    if (!current.is_valid() || branch.empty())
        return;

    const int hashed = current.get_hash();

    // a flag that determines whether it is necessary to do searching on the current node
    bool flag = true;

    // a flag that determines whether the state is being evaluated further up the current branch
    Thread::Atomic<bool> &in_branch = in_stack[std::to_string(hashed) + '|' + std::to_string(branch.back())];

    Thread::Atomic<evaluating_node_data> &node = table[hashed];

    node.access([&](const evaluating_node_data &node) {
        // the state has been well evaluated before
        if (node.evaluated_depth >= depth && node.alpha - alpha >= -EPSILON && node.beta - beta <= EPSILON)
            flag = false;
    });
    if (!flag)
        return;

    state_evaluated.mutate([](size_t &n) { ++n; });

    node.mutate([&](evaluating_node_data &node) {
        // the game is over
        if (current.is_over())
        {
            node.score = ABS_SCORE * (current.get_winner() == 'W' ? 1 : -1);
            node.evaluated_depth = EVALUATION_DEPTH + 1; // ending states need no further evaluation
            flag = false;
            return;
        }

        // depth reaches 0
        if (!depth)
        {
            calculate_original_score(current, node);
            node.evaluated_depth = depth;
            flag = false;
            return;
        }

        // generate moves/states
        if (node.evaluated_marker != evaluated_marker)
        {
            node.moves.clear();

            // hand moves
            const char sides[] = { 'L', 'R' };
            for (char my_side: sides)
                for (char op_side: sides)
                    try
                    {
                        state tmp = current;
                        tmp.make_move(my_side, op_side);
                        node.moves[move_data(my_side, op_side).get_displayable()].set(MOVE_TO_BE_EVALUATED);
                    }
                    catch (const std::runtime_error &e) {}

            // split moves
            const short low_bound = current.white_turn ? -current.white_left_hand : -current.black_left_hand;
            const short  up_bound = current.white_turn ? current.white_right_hand : current.black_right_hand;
            for (short i = low_bound; i <= up_bound; ++i)
                try
                {
                    state tmp = current;
                    tmp.make_split_move(i, -i);
                    node.moves[move_data(i, -i, true).get_displayable()].set(MOVE_TO_BE_EVALUATED);
                }
                catch (const std::runtime_error &e) {}

            node.evaluated_marker = evaluated_marker;
        }

        // reset the score
        node.score = SCORE_RANGE * (maximizing ? -1 : 1);
    });

    // mark as in-hashed
    in_branch.set(true);

    // evaluate all the moves
    std::vector<std::tuple<move_data, state, evaluation_state> > moves;

    node.mutate([&](evaluating_node_data &node) {
        for (auto &&p : node.moves.entities())
        {
            const move_data move = move_data::parse_displayable(p.first);

            state tmp = current;
            if (move.is_split)
                tmp.make_split_move(move.fparam, move.sparam);
            else
                tmp.make_move((char)move.fparam, (char)move.sparam);

            // the state is already being evaluated further up this branch
            bool pushed = true;
            for (int branch_id : branch)
                if (!(pushed = !in_stack[std::to_string(tmp.get_hash()) + '|' + std::to_string(branch_id)].get()))
                    break;

            if (pushed)
                moves.push_back(std::make_tuple(move, tmp, p.second));
        }
    });

    // we prioritize the winning moves and moves that lead to hand advantage
    std::sort(moves.begin(), moves.end(), [current](auto x, auto y) {
        if (std::get<1>(x).is_over() && std::get<1>(x).get_winner() == (current.white_turn ? 'W' : 'B'))
            return true;
        if (std::get<1>(y).is_over() && std::get<1>(y).get_winner() == (current.white_turn ? 'W' : 'B'))
            return false;

        const int white_hands = !!std::get<1>(x).white_left_hand + !!std::get<1>(x).white_right_hand,
                  black_hands = !!std::get<1>(x).black_left_hand + !!std::get<1>(x).black_right_hand;
        if (current.white_turn ? (white_hands > black_hands) : (black_hands > white_hands))
            return true;

        if (std::get<2>(x) == MOVE_EVALUATING)
            return true;

        return false;
    });

    auto evaluate = [&](Thread::Atomic<evaluation_state> &status,
                        std::tuple<move_data, state, evaluation_state> st,
                        evaluating_node_data &node,
                        std::vector<int> branch) {
        status.wait_until_cond(MOVE_EVALUATING);

        if (status.get() == MOVE_TO_BE_EVALUATED)
        {
            status.set(MOVE_EVALUATING);
            search(std::get<1>(st), branch, depth - 1, alpha, beta, !maximizing);
            status.set(MOVE_EVALUATED);
        }

        after_search(st, node, depth, maximizing, alpha, beta);
    };

    for (auto move : moves)
    {
        bool should_break = false;

        node.mutate([&](evaluating_node_data &node) {
            Thread::Atomic<evaluation_state> &status = node.moves[std::get<0>(move).get_displayable()];

            if (depth == EVALUATION_DEPTH)
            {
                branch.push_back(++branch_id_counter);
                Pool.add(evaluate, std::ref(status), move, std::ref(node), branch);
                branch.pop_back();
            }
            else
            {
                evaluate(status, move, node, branch);

                if (alpha - beta >= -EPSILON)
                    should_break = true;
            }
        });

        if (should_break)
            break;
    }

    // mark as out-hashed
    in_branch.set(false);
}

node_data Evaluator::get_node_data(int hash_state)
{
    node_data ret;

    if (!table.has_key(hash_state))
        throw std::runtime_error("Unknown game state: The state is either invalid or not evaluated");

    table[hash_state].access([&](const evaluating_node_data &node) {
        ret.score = node.score;
        ret.evaluated_depth = node.evaluated_depth;
        ret.best_move = node.best_move;
    });

    return ret;
}

node_data Evaluator::get_node_data(state game_state)
{
    return get_node_data(game_state.get_hash());
}

void Evaluator::evaluate_next_move(int hash_state)
{
    return evaluate_next_move(state::parse_hash(hash_state));
}

void Evaluator::evaluate_next_move(state game_state)
{
    if (!game_state.is_valid() || game_state.is_over())
        throw std::runtime_error("Next move evaluation does not exist for invalid or ended games");

    evaluated_marker ^= 1;

    Pool.add([=]() { search(game_state); });
    Pool.wait();
}

size_t Evaluator::get_last_number_of_evaluated_states()
{
    return state_evaluated.get();
}
