// Game of Othello -- Example of main
// Universidad Simon Bolivar, 2012.
// Author: Blai Bonet
// Last Revision: 1/11/16
// Modified by:

#include <iostream>
#include <limits>
#include "othello_cut.h" // won't work correctly until .h is fixed!
#include "utils.h"

#include <unordered_map>

#define INFINITY 10000000

using namespace std;

unsigned expanded = 0;
unsigned generated = 0;
int tt_threshold = 32; // threshold to save entries in TT

// Transposition table (it is not necessary to implement TT)
struct stored_info_t
{
    int value_;
    int type_;
    enum
    {
        EXACT,
        LOWER,
        UPPER
    };
    stored_info_t(int value = -100, int type = LOWER) : value_(value), type_(type) {}
};

struct hash_function_t
{
    size_t operator()(const state_t &state) const
    {
        return state.hash();
    }
};

class hash_table_t : public unordered_map<state_t, stored_info_t, hash_function_t>
{
};

hash_table_t TTable[2];

// int maxmin(state_t state, int depth, bool use_tt);
// int minmax(state_t state, int depth, bool use_tt = false);
// int maxmin(state_t state, int depth, bool use_tt = false);
int negamax(state_t state, int depth, int color, bool use_tt = false);
int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);
int scout(state_t state, int depth, int color, bool use_tt = false);
int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);

int main(int argc, const char **argv)
{
    state_t pv[128];
    int npv = 0;
    for (int i = 0; PV[i] != -1; ++i)
        ++npv;

    int algorithm = 0;
    if (argc > 1)
        algorithm = atoi(argv[1]);
    bool use_tt = argc > 2;

    // Extract principal variation of the game
    state_t state;
    cout << "Extracting principal variation (PV) with " << npv << " plays ... " << flush;
    for (int i = 0; PV[i] != -1; ++i)
    {
        bool player = i % 2 == 0; // black moves first!
        int pos = PV[i];
        pv[npv - i] = state;
        state = state.move(player, pos);
    }
    pv[0] = state;
    cout << "done!" << endl;

    // print principal variation
#if 0
    for( int i = 0; i <= npv; ++i )
        cout << pv[npv - i];
#endif

    // Print name of algorithm
    cout << "Algorithm: ";
    if (algorithm == 1)
        cout << "Negamax (minmax version)";
    else if (algorithm == 2)
        cout << "Negamax (alpha-beta version)";
    else if (algorithm == 3)
        cout << "Scout";
    else if (algorithm == 4)
        cout << "Negascout";
    cout << (use_tt ? " w/ transposition table" : "") << endl;

    // Run algorithm along PV (bacwards)
    cout << "Moving along PV:" << endl;
    for (int i = 0; i <= npv; ++i)
    {
        // cout << pv[i];
        int value = 0;
        TTable[0].clear();
        TTable[1].clear();
        float start_time = Utils::read_time_in_seconds();
        expanded = 0;
        generated = 0;
        int color = i % 2 == 1 ? 1 : -1;

        try
        {
            if (algorithm == 1)
            {
                value = negamax(pv[i], 33, color, use_tt);
            }
            else if (algorithm == 2)
            {
                value = negamax(pv[i], 33, -200, 200, color, use_tt);
            }
            else if (algorithm == 3)
            {
                value = scout(pv[i], 33, color, use_tt);
            }
            else if (algorithm == 4)
            {
                value = negascout(pv[i], 33, -200, 200, color, use_tt);
            }
        }
        catch (const bad_alloc &e)
        {
            cout << "size TT[0]: size=" << TTable[0].size() << ", #buckets=" << TTable[0].bucket_count() << endl;
            cout << "size TT[1]: size=" << TTable[1].size() << ", #buckets=" << TTable[1].bucket_count() << endl;
            use_tt = false;
        }

        float elapsed_time = Utils::read_time_in_seconds() - start_time;

        printf("%2d. %s moves: value=%15d, #expanded=%6u, #generated=%6u, seconds=%f, #generated/second=%8.0f\n",
               npv + 1 - i, (color == 1 ? "Black" : "White"), color * value, expanded, generated, elapsed_time, generated / elapsed_time);
    }

    return 0;
}

int negamax(state_t state, int depth, int color, bool use_tt)
{
    ++generated;

    if (depth == 0 || state.terminal())
    {
        return color * state.value();
    }

    bool not_valid_move = true;
    bool boolean_color = color == 1; // Convertir color a booleano para su uso con outflank
    int alpha = -INFINITY;

    for (int pos = 0; pos < DIM; ++pos)
    {
        if (state.outflank(boolean_color, pos))
        {
            state_t child = state.move(boolean_color, pos);
            int value = -negamax(child, depth - 1, -color, use_tt);
            if (value > alpha)
            {
                alpha = value;
            }
            not_valid_move = false;
        }
    }

    if (not_valid_move)
    {
        // Si no hay movimientos posibles, pasar el turno al oponente.
        alpha = std::max(alpha, -negamax(state, depth - 1, -color, use_tt));
    }

    ++expanded;
    return alpha;
}

int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt)
{
    ++generated;

    if (depth == 0 || state.terminal())
    {
        return color * state.value();
    }

    bool not_valid_move = true;
    bool boolean_color = color == 1; // Convertir color a booleano para su uso con outflank

    for (int pos = 0; pos < DIM; ++pos)
    {
        if (state.outflank(boolean_color, pos))
        {
            state_t child = state.move(boolean_color, pos);
            int value = -negamax(child, depth - 1, -beta, -alpha, -color, use_tt);
            if (value >= beta)
            {
                return value;
            }
            if (value > alpha)
            {
                alpha = value;
            }
            not_valid_move = false;
        }
    }

    if (not_valid_move)
    {
        // Si no hay movimientos posibles, pasar el turno al oponente.
        alpha = std::max(alpha, -negamax(state, depth - 1, -beta, -alpha, -color, use_tt));
    }

    ++expanded;
    return alpha;
}

int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt)
{
    ++generated;

    if (depth == 0 || state.terminal())
    {
        return color * state.value();
    }

    bool firsChild = true;
    int score = 0;
    bool not_valid_move = true;
    bool boolean_color = color == 1; // Convertir color a booleano para su uso con outflank

    for (int pos = 0; pos < DIM; ++pos)
    {
        if (state.outflank(boolean_color, pos))
        {
            state_t child = state.move(boolean_color, pos);
            if (firsChild)
            {
                score = -negascout(child, depth - 1, -beta, -alpha, -color, use_tt);
                firsChild = false;
            }
            else
            {
                score = -negascout(child, depth - 1, -alpha - 1, -alpha, -color, use_tt);
                if (alpha < score && score < beta)
                {
                    score = -negascout(child, depth - 1, -beta, -score, -color, use_tt);
                }
            }
            if (score >= beta)
            {
                return score;
            }
            if (score > alpha)
            {
                alpha = score;
            }
            not_valid_move = false;
        }
    }

    if (not_valid_move)
    {
        // Si no hay movimientos posibles, pasar el turno al oponente.
        alpha = std::max(alpha, -negascout(state, depth - 1, -beta, -alpha, -color, use_tt));
    }

    ++expanded;
    return alpha;
}