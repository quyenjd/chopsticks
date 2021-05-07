#ifndef STATES_HPP_INCLUDED
#define STATES_HPP_INCLUDED

#include <math.h>
#include <sstream>
#include <stdexcept>
#include <string>

class state
{
private:
    void can_move() const
    {
        if (!is_valid())
            throw std::runtime_error("Invalid state: Game is invalid");
        if (is_over())
            throw std::runtime_error("Invalid move: Game is over");
    }

    void after_move(bool from_split = false)
    {
        white_left_hand  %= white_left_hand_max;
        white_right_hand %= white_right_hand_max;
        black_left_hand  %= black_left_hand_max;
        black_right_hand %= black_right_hand_max;

        if (!from_split || splits_as_moves)
            white_turn ^= 1;
    }

public:
    static const short white_left_hand_max  = 5;
    static const short white_right_hand_max = 5;
    static const short black_left_hand_max  = 5;
    static const short black_right_hand_max = 5;
    static const short white_split_max = -1; // negative value for unlimited splits
    static const short black_split_max = -1; // negative value for unlimited splits

    static const bool splits_as_moves = true;
    static const bool allow_sacrifical_splits = true;
    static const bool allow_regenerative_splits = true;

    short white_left_hand;
    short white_right_hand;
    short black_left_hand;
    short black_right_hand;
    short white_split;
    short black_split;
    bool white_turn;

    state()
    {
        white_left_hand     = 1;
        white_right_hand    = 1;
        black_left_hand     = 1;
        black_right_hand    = 1;
        white_split = white_split_max;
        black_split = black_split_max;
        white_turn          = true;
    }

    bool is_valid() const
    {
        return
            white_left_hand  < white_left_hand_max  &&
            white_right_hand < white_right_hand_max &&
            black_left_hand  < black_left_hand_max  &&
            black_right_hand < black_right_hand_max &&
            (white_split_max < 0 || white_split >= 0) &&
            (black_split_max < 0 || black_split >= 0) &&
            (white_left_hand || white_right_hand || black_left_hand || black_right_hand);
    }

    bool is_over() const
    {
        return is_valid() && (!white_left_hand && !white_right_hand) != (!black_left_hand && !black_right_hand);
    }

    char get_winner() const
    {
        if (is_valid() && is_over())
            return white_left_hand || white_right_hand ? 'W' : 'B';

        throw std::runtime_error("Invalid state: Cannot determine winners of ongoing or invalid games");
    }

    void make_split_move(int left_change, int right_change)
    {
        can_move();

        if ((white_turn ? white_split_max : black_split_max) < 0 && !(white_turn ? white_split : black_split))
            throw std::runtime_error("Invalid move: No split moves remaining for " + std::string(white_turn ? "WHITE" : "BLACK"));

        if (1LL * left_change * right_change >= 0)
            throw std::runtime_error("Invalid move: split moves should consist of decreasing one side and increasing the other");

        bool left_decrease = left_change < 0;
        left_change = abs(left_change);
        right_change = abs(right_change);

        if (!allow_regenerative_splits &&
            !((white_turn ? white_left_hand : black_left_hand) && (white_turn ? white_right_hand : black_right_hand)))
            throw std::runtime_error("Invalid move: Regenerative splits are not allowed");

        if ((left_decrease && (white_turn ? white_left_hand : black_left_hand) < left_change + !allow_sacrifical_splits) || // decrease leads to zero left hand
           (!left_decrease && (white_turn ? white_right_hand : black_right_hand) < right_change + !allow_sacrifical_splits) || // decrease leads to zero right hand
           (!left_decrease && !allow_sacrifical_splits && ((white_turn ? white_left_hand : black_left_hand) + left_change) % (white_turn ? white_left_hand_max : black_left_hand_max) == 0) || // increase leads to zero left hand
            (left_decrease && !allow_sacrifical_splits && ((white_turn ? white_right_hand : black_right_hand) + right_change) % (white_turn ? white_right_hand_max : black_right_hand_max) == 0)) // increase leads to zero right hand
            throw std::runtime_error("Invalid move: Sacrificial splits are not allowed");

        const int saved_white_left_hand  = white_left_hand,
                  saved_white_right_hand = white_right_hand,
                  saved_black_left_hand  = black_left_hand,
                  saved_black_right_hand = black_right_hand;

        if (white_turn)
        {
            white_left_hand += left_change * (left_decrease ? -1 : 1);
            white_right_hand += right_change * (!left_decrease ? -1 : 1);
            --white_split;
        }
        else
        {
            black_left_hand += left_change * (left_decrease ? -1 : 1);
            black_right_hand += right_change * (!left_decrease ? -1 : 1);
            --black_split;
        }

        // check if the moves are hand-alternating
        if (white_turn)
        {
            if (saved_white_left_hand == white_right_hand &&
                saved_white_right_hand == white_left_hand)
                throw std::runtime_error("Invalid move: Hand-alternating split moves are not allowed");
        }
        else
        {
            if (saved_black_left_hand == black_right_hand &&
                saved_black_right_hand == black_left_hand)
                throw std::runtime_error("Invalid move: Hand-alternating split moves are not allowed");
        }

        after_move(true);
    }

    void make_move(char my_side, char op_side)
    {
        can_move();

        my_side = toupper(my_side);
        op_side = toupper(op_side);

        if ((my_side != 'L' && my_side != 'R') || (op_side != 'L' && op_side != 'R'))
            throw std::runtime_error("Invalid move: Sides should be L and R (case-insensitive) only");


        if ((my_side == 'L' && !(white_turn ? white_left_hand : black_left_hand)) ||
            (my_side == 'R' && !(white_turn ? white_right_hand : black_right_hand)))
            throw std::runtime_error(std::string("Invalid move: Cannot use eliminated ") +
                                     my_side + "-side of " + (white_turn ? "WHITE" : "BLACK"));

        if ((op_side == 'L' && !(!white_turn ? white_left_hand : black_left_hand)) ||
            (op_side == 'R' && !(!white_turn ? white_right_hand : black_right_hand)))
            throw std::runtime_error(std::string("Invalid move: ") +
                                     my_side + "-side of " + (white_turn ? "BLACK" : "WHITE") + " has already been eliminated");

        if (white_turn)
        {
            const short add = my_side == 'L' ? white_left_hand : white_right_hand;
            if (op_side == 'L')
                black_left_hand += add;
            else
                black_right_hand += add;
        }
        else
        {
            const short add = my_side == 'L' ? black_left_hand : black_right_hand;
            if (op_side == 'L')
                white_left_hand += add;
            else
                white_right_hand += add;
        }

        after_move();
    }

    int get_hash() const
    {
        if (!is_valid())
            throw std::runtime_error("Invalid state: Cannot get hash of invalid games");

        int result = white_turn;

        (result *= white_left_hand_max) += white_left_hand;
        (result *= white_right_hand_max) += white_right_hand;
        (result *= black_left_hand_max) += black_left_hand;
        (result *= black_right_hand_max) += black_right_hand;

        if (white_split_max > 0)
            (result *= white_split_max + 1) += white_split;
        if (black_split_max > 0)
            (result *= black_split_max + 1) += black_split;

        return result;
    }

    static state parse_hash(int hashed)
    {
        state result;

        if (black_split_max > 0)
        {
            result.black_split = hashed % (black_split_max + 1);
            hashed /= black_split_max + 1;
        }

        if (white_split_max > 0)
        {
            result.white_split = hashed % (white_split_max + 1);
            hashed /= white_split_max + 1;
        }

        result.black_right_hand = hashed % black_right_hand_max;
        hashed /= black_right_hand_max;

        result.white_right_hand = hashed % white_right_hand_max;
        hashed /= white_right_hand_max;

        result.black_left_hand = hashed % black_left_hand_max;
        hashed /= black_left_hand_max;

        result.white_left_hand = hashed % white_left_hand_max;
        hashed /= white_left_hand_max;

        result.white_turn = hashed;

        if (!result.is_valid())
            throw std::runtime_error("Parse error: Invalid game hash");

        return result;
    }

    std::string get_displayable() const
    {
        const bool valid = is_valid();
        std::ostringstream oss;

        oss << std::endl
            << "\\  Splits alter turns  " << (splits_as_moves ? "YES" : "NO") << std::endl
            << "\\  Sacrificial splits  " << (allow_sacrifical_splits ? "ALLOWED" : "NOT ALLOWED") << std::endl
            << "\\ Regenerative splits  " << (allow_regenerative_splits ? "ALLOWED" : "NOT ALLOWED") << std::endl
            << std::endl
            << " " << (valid && white_turn ? ">>" : "  ") << " White  - L - R -" << std::endl
            << "           | " << white_left_hand << " | " << white_right_hand << " |" << std::endl
            << "           - - - - -" << std::endl;

        if (white_split_max < 0)
            oss << "                      S: unlimited" << std::endl;
        else
            oss << "                      S: " << white_split << " left" << std::endl;

        oss << " " << (valid && !white_turn ? ">>" : "  ") << " Black  - L - R -" << std::endl
            << "           | " << black_left_hand << " | " << black_right_hand << " |" << std::endl
            << "           - - - - -" << std::endl;

        if (black_split_max < 0)
            oss << "                      S: unlimited" << std::endl;
        else
            oss << "                      S: " << black_split << " left" << std::endl;

        oss << std::endl
            << " " << (!valid ? ">>" : "  ") << (valid ? " GAME ON!" : " GAME IS OVER!") << std::endl
            << std::endl;

        return oss.str();
    }
};

#endif // STATES_HPP_INCLUDED
