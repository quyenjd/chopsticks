#include "UI.h"
#include <conio.h>
#include <ctype.h>
#include <iostream>
#include <stdlib.h>
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
        std::cout << "Press W to play white, B to play black, or Q to quit the game... ";

        char c = getch();
        if (c == 'W' || c == 'w' || c == 'B' || c == 'b' || c == 'Q' || c == 'q')
        {
            std::cout << std::endl;
            return c;
        }
    }
}

void UI::game(Evaluator *evaluator, bool white_turn)
{
    state game_state;
    UI game_handler(game_state);

    while (true)
    {
        game_state = game_handler.get_game_state();

        system("cls");
        std::cout << "    Move #" << (game_handler.get_moves().size() + 1) << std::endl
                  << game_state.get_displayable();

        to_row_col(20, 0);
        std::cout << "--  Made moves:" << std::endl;
        const std::vector<move_data> moves = game_handler.get_moves();
        if (moves.empty())
            std::cout << "    No moves";
        else
            for (size_t i = 0; i < moves.size(); ++i)
                std::cout << (i ? " | " : "    ") << moves[i].get_displayable();

        const node_data node = evaluator->get_node_data(game_state);
        const double total = node.winning_lines + node.losing_lines;
        std::cout << std::endl << std::endl
                  << "--  Evaluation:" << std::endl
                  << "        [ Score]  " << node.score << std::endl
                  << "        [  Wins]  " << (node.winning_lines / total) << std::endl
                  << "        [Losses]  " << (node.losing_lines / total);

        to_row_col(17, 0);
        if (!game_state.is_over())
        {
            if (game_state.white_turn == white_turn)
                std::cout << "    Your move:  ";
            else
            {
                std::cout << "    Computer is playing... ";
                game_handler.make_move(evaluator->get_evaluated_next_move(game_state));
                continue;
            }
        }
        else
        {
            std::cout << "    " << (game_state.white_turn ? "Black" : "White") << " wins" << std::endl
                      << "    Press any key to continue or Q to exit... ";
            char ch = getch();
            if (toupper(ch) != 'Q')
                game(evaluator, !white_turn);
            return;
        }

        while (true)
        {
            move_data move;
            bool left_decrease;
            int counter;

first_input:

            move.is_split = false;

            to_row_col(17, 16);
            std::cout << "[   ]                     ";
            to_row_col(17, 18);

            char ch = '\0';
            while (toupper(ch) != 'L' && toupper(ch) != 'R' && toupper(ch) != 'S' && ch != KEY_BACKSPACE)
                ch = getch();
            if (ch == KEY_BACKSPACE)
            {
                to_row_col(17, 16);
                std::cout << "[ Quit the game? (y/n)   ]";
                to_row_col(17, 39);
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

            to_row_col(17, 23);
            std::cout << "[   ]              ";
            to_row_col(17, 25);

            ch = '\0';
            while (toupper(ch) != 'L' && toupper(ch) != 'R' && ch != KEY_BACKSPACE)
                ch = getch();
            if (ch == KEY_BACKSPACE)
                goto first_input;
            else
            {
                ch = toupper(ch);
                std::cout << ch;

                if (move.is_split)
                {
                    left_decrease = ch == 'R';
                    goto third_input;
                }
                else
                {
                    move.sparam = ch;

                    std::cout << "  proceed? ]";
                    to_row_col(17, 36);

                    while (ch != KEY_BACKSPACE && ch != '\n' && ch != '\r')
                        ch = getch();

                    if (ch == KEY_BACKSPACE)
                        goto second_input;
                    else
                        goto finalize;
                }
            }

third_input:

            to_row_col(17, 30);
            std::cout << "[ 0 ]       ";
            to_row_col(17, 32);

            counter = 0;
            while (true)
            {
                ch = getch();
                if (ch == KEY_UP)
                    counter = (counter + 1) % 1000;
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

                to_row_col(17, 32 - (counter > 99));
                std::cout << counter;
                to_row_col(17, 33 - (counter < 10));
            }

finalize:
            try
            {
                game_handler.make_move(move);
                break;
            }
            catch (std::runtime_error e)
            {
                goto first_input;
            }

        }
    }
}

void UI::run()
{
    Evaluator *evaluator = new Evaluator();

    char user = menu();

    if (user != 'Q' && user != 'q')
        game(evaluator, toupper(user) == 'W');

    system("cls");
    std::cout << "Good bye!" << std::endl;
}
