#include "UI.h"
#include <conio.h>
#include <ctype.h>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <windows.h>

#define KEY_BACKSPACE 8
#define KEY_UP 72
#define KEY_DOWN 80

void to_row_col (int row, int col)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)col, (SHORT)row };
    SetConsoleCursorPosition(hConsole, pos);
}

void get_row_col (int &row, int &col)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    if (GetConsoleScreenBufferInfo(hConsole, &cbsi))
    {
        row = cbsi.dwCursorPosition.Y;
        col = cbsi.dwCursorPosition.X;
    }
    else
        row = col = 0;
}

UI::UI()
{
    game_state = state();
}

UI::UI(state game)
{
    game_state = game;
}

state UI::get_game_state() const
{
    return game_state;
}

std::vector<move_data> UI::get_moves() const
{
    return moves;
}

void UI::make_move(move_data data)
{
    if (data.is_split)
        game_state.make_split_move(data.fparam, data.sparam);
    else
        game_state.make_move((char)data.fparam, (char)data.sparam);

    moves.push_back(data);
}

char UI::menu()
{
    while (true)
    {
        system("cls");
        std::cout << "Press W to play white, B to play black, C to watch computer vs. computer, or Q to quit the game... ";

        char c = toupper(getch());
        if (c == 'W' || c == 'B' || c == 'Q' || c == 'C')
        {
            std::cout << std::endl;
            return c;
        }
    }
}

void UI::game(Evaluator *evaluator, bool white_turn, bool two_computers)
{
    state game_state;
    UI game_handler(game_state);
    bool which_computer = false;

    while (true)
    {
        game_state = game_handler.get_game_state();

        system("cls");
        std::cout << "    Move #" << (game_handler.get_moves().size() + 1) << std::endl
                  << game_state.get_displayable();

        to_row_col(23, 0);
        std::cout << "--  Made moves:" << std::endl;
        const std::vector<move_data> moves = game_handler.get_moves();
        if (moves.empty())
            std::cout << "    No moves";
        else
            for (size_t i = 0; i < moves.size(); ++i)
                std::cout << (i ? " | " : "    ") << moves[i].get_displayable();

        to_row_col(21, 0);
        std::cout << "--  Evaluating... ";
        if (!game_state.is_over())
        {
            evaluator->evaluate_next_move(game_state);
            node_data node = evaluator->get_node_data(game_state);
            to_row_col(21, 0);
            std::cout << "--  Evaluation (states: " << evaluator->get_last_number_of_evaluated_states()
                      << ", depth " << node.evaluated_depth << "):  " << node.score;
        }
        else
        {
            to_row_col(21, 0);
            std::cout << "--  Evaluation is not available";
        }

        to_row_col(18, 0);
        if (!game_state.is_over())
        {
            if (!two_computers && game_state.white_turn == white_turn)
                std::cout << "    Your move:  ";
            else
            {
                std::cout << "    Computer " << (two_computers ? (std::to_string(1 + which_computer) + " ") : "") << "is playing... ";

                const move_data move = evaluator->get_node_data(game_state).best_move;
                game_handler.make_move(move);

                to_row_col(18, 4);
                std::cout << "Computer " << (two_computers ? (std::to_string(1 + which_computer) + " ") : "") << "has played: " << move.get_displayable() << " | Press any key to continue or Q to exit... ";

                which_computer ^= 1;

                char ch = getch();
                if (toupper(ch) == 'Q')
                    return;

                continue;
            }
        }
        else
        {
            std::cout << "    " << (game_state.white_turn ? "Black" : "White") << " wins" << std::endl
                      << "    Press any key to continue or Q to exit... ";
            char ch = getch();
            if (toupper(ch) != 'Q')
                game(evaluator, two_computers || !white_turn, two_computers);
            return;
        }

        while (true)
        {
            move_data move;
            bool left_decrease;
            int counter;

first_input:

            move.is_split = false;

            to_row_col(18, 16);
            std::cout << "[   ]                     ";
            to_row_col(18, 18);

            char ch = '\0';
            while (toupper(ch) != 'L' && toupper(ch) != 'R' && toupper(ch) != 'S' && ch != KEY_BACKSPACE)
                ch = getch();
            if (ch == KEY_BACKSPACE)
            {
                to_row_col(18, 16);
                std::cout << "[ End this game? (y/n)   ]";
                to_row_col(18, 39);
                while (toupper(ch) != 'Y' && toupper(ch) != 'N')
                    ch = getch();
                if (toupper(ch) == 'Y')
                    return;
                else
                    goto first_input;
            }
            else
            {
                ch = toupper(ch);

                if (ch == 'S')
                {
                    if (!(game_state.white_turn ? game_state.white_split : game_state.black_split))
                        goto first_input;
                    else
                    {
                        std::cout << ch;
                        move.is_split = true;
                        goto second_input;
                    }
                }
                else
                {
                    if (ch == 'L' && !(game_state.white_turn ? game_state.white_left_hand : game_state.black_left_hand))
                        goto first_input;
                    if (ch == 'R' && !(game_state.white_turn ? game_state.white_right_hand : game_state.black_right_hand))
                        goto first_input;

                    std::cout << ch;
                    move.fparam = ch;
                    goto second_input;
                }
            }

second_input:

            to_row_col(18, 23);
            std::cout << "[   ]              ";
            to_row_col(18, 25);

            ch = '\0';
            while (toupper(ch) != 'L' && toupper(ch) != 'R' && ch != KEY_BACKSPACE)
                ch = getch();
            if (ch == KEY_BACKSPACE)
                goto first_input;
            else
            {
                ch = toupper(ch);

                if (move.is_split)
                {
                    if (ch == 'L' && !((game_state.white_turn ? game_state.white_left_hand : game_state.black_left_hand) || game_state.allow_regenerative_splits))
                        goto second_input;
                    if (ch == 'R' && !((game_state.white_turn ? game_state.white_right_hand : game_state.black_right_hand) || game_state.allow_regenerative_splits))
                        goto second_input;

                    std::cout << ch;
                    left_decrease = ch == 'R';
                    goto third_input;
                }
                else
                {
                    if (ch == 'L' && !(game_state.white_turn ? game_state.black_left_hand : game_state.white_left_hand))
                        goto second_input;
                    if (ch == 'R' && !(game_state.white_turn ? game_state.black_right_hand : game_state.white_right_hand))
                        goto second_input;

                    move.sparam = ch;
                    std::cout << ch;

                    std::cout << "  proceed? ]";
                    to_row_col(18, 36);

                    while (ch != KEY_BACKSPACE && ch != '\n' && ch != '\r')
                        ch = getch();

                    if (ch == KEY_BACKSPACE)
                        goto second_input;
                    else
                        goto finalize;
                }
            }

third_input:

            to_row_col(18, 30);
            std::cout << "[ 0 ]       ";
            to_row_col(18, 32);

            counter = 0;
            while (true)
            {
                ch = getch();
                if (ch == KEY_UP)
                    counter = std::min((counter + 1) % 1000,
                                       std::max(0, (left_decrease ?
                                                   (game_state.white_turn ? game_state.white_left_hand : game_state.black_left_hand) :
                                                   (game_state.white_turn ? game_state.white_right_hand : game_state.black_right_hand)) -
                                                   !game_state.allow_sacrifical_splits));
                else if (ch == KEY_DOWN)
                    counter = std::max(0, counter - 1);
                else if (ch == '\n' || ch == '\r')
                {
                    move.fparam = counter * (left_decrease ? -1 : 1);
                    move.sparam = counter * (left_decrease ? 1 : -1);
                    goto finalize;
                }
                else if (ch == KEY_BACKSPACE)
                    goto second_input;

                to_row_col(18, 32 - (counter > 99));
                std::cout << counter;
                to_row_col(18, 33 - (counter < 10));
            }

finalize:

            try
            {
                game_handler.make_move(move);
                break;
            }
            catch (const std::runtime_error &e)
            {
                goto first_input;
            }

        }
    }
}

void UI::run()
{
    std::cout << std::fixed << std::setprecision(15);

    Evaluator *evaluator = new Evaluator();

    char user = '\0';

    while (user != 'Q')
    {
        user = menu();

        if (user == 'C')
            game(evaluator, true, true);
        else
        if (user != 'Q')
            game(evaluator, user == 'W', false);
    }

    system("cls");
    std::cout << "Good bye!" << std::endl;
}
