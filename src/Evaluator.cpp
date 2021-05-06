#include "Evaluator.h"
#include <conio.h>
#include <math.h>
#include <iostream>
#include <stdexcept>

void Evaluator::update_node_util(bool is_draw, node_data *node, state current, state tmp, move_data move, int &move_evaluated)
{
    const int tmp_hashed = tmp.get_hash();
    node->winning *= node->moves.size();
    node->drawing *= node->moves.size();
    node->losing *= node->moves.size();

    if (is_draw)
        ++node->drawing;
    else
    {
        if (tmp.white_turn == current.white_turn)
        {
            node->winning += table[tmp_hashed].winning;
            node->losing += table[tmp_hashed].losing;
        }
        else
        {
            node->winning += table[tmp_hashed].losing;
            node->losing += table[tmp_hashed].winning;
        }
    }

    node->moves.push_back(move);
    node->winning /= node->moves.size();
    node->drawing /= node->moves.size();
    node->losing /= node->moves.size();

    ++move_evaluated;
}

void Evaluator::search(int &move_evaluated,
                       int &max_depth,
                       state current, std::unordered_map<int, bool> *in_stack,
                       int depth)
{
    if (!current.is_valid())
        return;

    const int hashed = current.get_hash();
    node_data *node = &table[hashed];

    if (!node->evaluated)
    {
        (*in_stack)[hashed] = true;
        max_depth = std::max(max_depth, depth);

        while (true)
        {
            // std::cerr << "\n    Evaluating" << current.get_displayable();

            // the game is over
            if (current.is_over())
            {
                const char winner = current.get_winner();

                if ((current.white_turn && winner == 'W') ||
                   (!current.white_turn && winner == 'B'))
                {
                    ++node->winning;
                    node->score = 2;
                }
                else
                {
                    ++node->losing;
                    node->score = -1;
                }

                break;
            }

            // hand moves
            const char sides[] = { 'L', 'R' };

            for (char my_side: sides)
                for (char op_side: sides)
                {
                    state tmp = current;
                    try
                    {
                        tmp.make_move(my_side, op_side);

                        // if the state reaches the one that is already in the stack (cycle)
                        const bool is_cycle = (*in_stack)[tmp.get_hash()];

                        if (!is_cycle)
                        {
                            search(move_evaluated, max_depth, tmp, in_stack, depth + 1);
                            node->score -= table[tmp.get_hash()].score;
                        }
                        update_node_util(is_cycle, node, current, tmp, move_data(my_side, op_side), move_evaluated);
                    }
                    catch (std::runtime_error e) {}
                }

            // split moves
            const short low_bound = current.white_turn ? -current.white_left_hand : -current.black_left_hand;
            const short  up_bound = current.white_turn ? current.white_right_hand : current.black_right_hand;
            for (short i = low_bound; i <= up_bound; ++i)
            {
                state tmp = current;
                try
                {
                    tmp.make_split_move(i, -i);

                    // if the state reaches the one that is already in the stack (cycle)
                    const bool is_cycle = (*in_stack)[tmp.get_hash()];

                    if (!is_cycle)
                    {
                        search(move_evaluated, max_depth, tmp, in_stack, depth + 1);
                        node->score += table[tmp.get_hash()].score * (state::splits_as_moves ? -1 : 1);
                    }
                    update_node_util(is_cycle, node, current, tmp, move_data(i, -i, true), move_evaluated);
                }
                catch (std::runtime_error e) {}
            }

            break;
        }

        (*in_stack)[hashed] = false;
        node->evaluated = true;

        if (!node->moves.empty())
            (node->score *= (double)0.3 / node->moves.size()) += (node->winning + node->drawing * 0.1 - node->losing) * 0.7;
        else
            node->score = node->winning + node->drawing * 0.1 - node->losing;

        // if splits are limited, the more we use the more the score is decreased
        if (current.white_turn && current.white_split_max > 0)
            node->score -= exp(((double)1.0 - (double)current.white_split / current.white_split_max) / 4.0) - 1.0;
        if (!current.white_turn && current.black_split_max > 0)
            node->score -= exp(((double)1.0 - (double)current.black_split / current.black_split_max) / 4.0) - 1.0;
    }

    if (!depth)
        delete in_stack;
}

Evaluator::Evaluator()
{
    std::cout << "Preparing evaluator... ";

    int move_evaluated = 0, max_depth = 0;
    search(move_evaluated, max_depth);

    system("cls");
    std::cout << "Evaluation has completed: " << move_evaluated << " moves evaluated at maximum depth " << max_depth << std::endl
              << "Press any key to continue... ";
    getch();
    std::cout << std::endl;
}

node_data Evaluator::get_node_data(int hash_state) const
{
    std::unordered_map<int, node_data>::const_iterator it = table.find(hash_state);
    if (it == table.end())
        throw std::runtime_error("Unknown game state: The state is either invalid or not evaluated");
    return it->second;
}

node_data Evaluator::get_node_data(state game_state) const
{
    return get_node_data(game_state.get_hash());
}

move_data Evaluator::get_evaluated_next_move(int hash_state)
{
    return get_evaluated_next_move(state::parse_hash(hash_state));
}

move_data Evaluator::get_evaluated_next_move(state game_state)
{
    if (!game_state.is_valid() || game_state.is_over())
        throw std::runtime_error("Next move evaluation does not exist for invalid or ended games");

    const int hashed = game_state.get_hash();
    if (evaluated_next_moves.find(hashed) != evaluated_next_moves.end())
        return evaluated_next_moves[hashed];

    node_data node = get_node_data(game_state);
    move_data chosen_move, chosen_same_turn_move;
    double max_score = -10.0, min_score = 10.0;
    bool has_same_turn_moves = false;

    for (move_data move : node.moves)
    {
        state tmp = game_state;
        if (move.is_split)
            tmp.make_split_move(move.fparam, move.sparam);
        else
            tmp.make_move((char)move.fparam, (char)move.sparam);

        const node_data node2 = get_node_data(tmp.get_hash());
        if (tmp.white_turn == game_state.white_turn)
        {
            if (node2.score > max_score)
            {
                max_score = node2.score;
                chosen_same_turn_move = move;
                has_same_turn_moves = true;
            }
        }
        else
        {
            if (node2.score < min_score)
            {
                min_score = node2.score;
                chosen_move = move;
            }
        }
    }

    return (evaluated_next_moves[hashed] = has_same_turn_moves ?
        (max_score - node.score > node.score - min_score ? chosen_same_turn_move : chosen_move) :
            chosen_move);
}
