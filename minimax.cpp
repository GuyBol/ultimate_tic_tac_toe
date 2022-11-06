#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#define DBG(stream) cerr << stream << endl;

using namespace std;

const int INFINITY = 999999;

enum Owner
{
    NONE,
    ME,
    ENEMY
};


struct Position
{
    int x;
    int y;

    Position(int ix, int iy): x(ix), y(iy)
    {}
};


class SubGrid
{
public:
    SubGrid()
    {
        for (int i = 0; i < 9; i++)
        {
            _spaces[i] = NONE;
        }
    }

    string toString() const
    {
        string str;
        for (int y = 0; y < 3; y++)
        {
            for (int x = 0; x < 3; x++)
            {
                switch (get(x,y))
                {
                case NONE:
                    str += "-";
                    break;
                case ME:
                    str += "X";
                    break;
                case ENEMY:
                    str += "O";
                    break;
                default:
                    break;
                }
            }
            str += '\n';
        }
        return str;
    }

    inline Owner get(int x, int y) const
    {
        return _spaces[x + y*3];
    }

    inline void set(int x, int y, Owner owner)
    {
        _spaces[x + y*3] = owner;
    }

    vector<Position> getEmptySpaces() const
    {
        vector<Position> pos;
        for (int x = 0; x < 3; x++)
        {
            for (int y = 0; y < 3; y++)
            {
                if (get(x, y) == NONE)
                {
                    pos.push_back(Position(x, y));
                }
            }
        }
        return pos;
    }

    Owner getWinner() const
    {
        if (get(0,0) == get(1,0) && get(0,0) == get(2,0))
            return get(0,0);
        else if (get(0,1) == get(1,1) && get(0,1) == get(2,1))
            return get(0,1);
        else if (get(0,2) == get(1,2) && get(0,2) == get(2,2))
            return get(0,2);
        else if (get(0,0) == get(0,1) && get(0,0) == get(0,2))
            return get(0,0);
        else if (get(1,0) == get(1,1) && get(1,0) == get(1,2))
            return get(1,0);
        else if (get(2,0) == get(2,1) && get(2,0) == get(2,2))
            return get(2,0);
        else if (get(0,0) == get(1,1) && get(0,0) == get(2,2))
            return get(0,0);
        else if (get(0,2) == get(1,1) && get(0,2) == get(2,0))
            return get(0,2);
        else
            return NONE;
    }

    bool completed() const
    {
        return getWinner() != NONE || getEmptySpaces().empty();
    }

private:
    array<Owner, 9> _spaces;
};


class AI
{
public:
    AI(SubGrid& subgrid): _grid(subgrid)
    {}

    Position play()
    {
        vector<Position> possibleActions = _grid.getEmptySpaces();
        int maxEval = -INFINITY;
        Position bestPos = possibleActions[0];
        int alpha = -INFINITY;
        int beta = INFINITY;
        for (const Position& pos : possibleActions)
        {
            _grid.set(pos.x, pos.y, ME);
            int eval = minimax(_grid, 8, alpha, beta, false);
            _grid.set(pos.x, pos.y, NONE);
            if (eval > maxEval)
            {
                maxEval = eval;
                bestPos = pos;
            }
            alpha = max(alpha, eval);
            if (beta <= alpha)
                break;
        }
        return bestPos;
    }

private:
    int minimax(SubGrid& grid, int depth, int alpha, int beta, bool maximize)
    {
        if (depth == 0 || grid.completed())
        {
            return evaluate(grid);
        }
        vector<Position> possibleActions = grid.getEmptySpaces();
        if (maximize)
        {
            int maxEval = -INFINITY;
            for (const Position& pos : possibleActions)
            {
                grid.set(pos.x, pos.y, ME);
                int eval = minimax(grid, depth-1, alpha, beta, false);
                grid.set(pos.x, pos.y, NONE);
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha)
                    break;
            }
            return maxEval;
        }
        else
        {
            int minEval = INFINITY;
            for (const Position& pos : possibleActions)
            {
                grid.set(pos.x, pos.y, ENEMY);
                int eval = minimax(grid, depth-1, alpha, beta, true);
                grid.set(pos.x, pos.y, NONE);
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha)
                    break;
            }
            return minEval;
        }
    }

    int evaluate(const SubGrid& grid)
    {
        Owner winner = grid.getWinner();
        if (winner == ME)
            return 1;
        else if (winner == ENEMY)
            return -1;
        else
            return 0;
    }

    SubGrid& _grid;
};


/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    SubGrid grid;
    AI ai(grid);

    // game loop
    while (1) {
        int opponent_row;
        int opponent_col;
        cin >> opponent_row >> opponent_col; cin.ignore();
        if (opponent_row != -1)
            grid.set(opponent_col, opponent_row, ENEMY);
        int valid_action_count;
        cin >> valid_action_count; cin.ignore();
        for (int i = 0; i < valid_action_count; i++) {
            int row;
            int col;
            cin >> row >> col; cin.ignore();
        }

        DBG(grid.toString());
        Position pos = ai.play();
        grid.set(pos.x, pos.y, ME);

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        cout << pos.y << " " << pos.x << endl;
    }
}