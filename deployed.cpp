#ifndef DISABLE_OPTIMS

#undef _GLIBCXX_DEBUG                // disable run-time bound checking, etc
#pragma GCC optimize("Ofast,inline")
#pragma GCC target("bmi,bmi2,lzcnt,popcnt")
#pragma GCC target("movbe")
#pragma GCC target("aes,pclmul,rdrnd")
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2")

#endif


#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <chrono>

#ifdef UNDEF_DBG
#define DBG(stream)
#else
#define DBG(stream) cerr << stream << endl
#endif

using namespace std;


class Random
{
public:
    static void Init()
    {
        srand(time(nullptr));
    }

    // Return a random number in [0, max[
    static int Rand(int max)
    {
        return rand() % max;
    }
};


double MyLog(unsigned int value)
{
    static array<double, 1000000> cache = {0.};
    if (cache[value] == 0.)
    {
        cache[value] = log(value);
    }
    return cache[value];
}


string G_myDisplay = "O";
string G_enemyDisplay = "X";

enum Player
{
    NONE,
    ME,
    ENEMY,
    UNDEFINED
};


struct Position
{
    int x;
    int y;

    Position(int ix, int iy): x(ix), y(iy)
    {}

    Position(): x(-1), y(-1)
    {}

    bool operator==(const Position& other) const
    {
        return (x == other.x && y == other.y);
    }

    bool operator!=(const Position& other) const
    {
        return !(*this == other);
    }

    string toString() const
    {
        return to_string(x) + "," + to_string(y);
    }
};


class PositionMask
{
public:
    static void InitCache()
    {
        for (uint64_t mask = 0; mask <= 0b111111111; mask++)
        {
            _Cache[mask] = {0,0,0,0,0,0,0,0,0,0};
            _Cache[mask][9] = __builtin_popcountl(mask);
            int index = 0;
            int i = 0;
            while (index < _Cache[mask][9])
            {
                if (mask & (0b100000000 >> i))
                {
                    _Cache[mask][index++] = 1 << (8-i);
                }
                i++;
            }
        }
    }

    PositionMask(): _masks({0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL}), _subgrid(0)
    {}

    void set(int subGridId, uint64_t localMask)
    {
        _masks[subGridId] = localMask;
        _subgrid = subGridId;
    }

    uint64_t get(int subGridId) const
    {
        return _masks[subGridId];
    }

    int lastSetSubgrid() const
    {
        return _subgrid;
    }

    // Count the total number of bit set in this mask
    int countSetBits() const
    {
        int count = 0;
        for (uint64_t mask : _masks)
        {
            count += _Cache[mask][9];
        }
        return count;
    }

    // Get the nth set bit in the full mask
    // Return a mask with just this bit set
    PositionMask getNthBitSet(unsigned int index)
    {
        PositionMask result;
        int subGridId = 0;
        for (uint64_t mask : _masks)
        {
            if (mask != 0)
            {
                int bitsSetCount = _Cache[mask][9];
                if (index < bitsSetCount)
                {
                    result.set(subGridId, GetNthSetBit(mask, index));
                    return result;
                }
                else
                {
                    index -= bitsSetCount;
                }
            }
            subGridId++;
        }
        return result;
    }

    // Get the (x,y) coordinate within subgrid from a subgrid mask
    static Position GetPositionFromSubMask(uint64_t mask)
    {
        switch (mask)
        {
        case 0b1:
            return Position(0,0);
        case 0b10:
            return Position(1,0);
        case 0b100:
            return Position(2,0);
        case 0b1000:
            return Position(0,1);
        case 0b10000:
            return Position(1,1);
        case 0b100000:
            return Position(2,1);
        case 0b1000000:
            return Position(0,2);
        case 0b10000000:
            return Position(1,2);
        case 0b100000000:
            return Position(2,2);
        default:
            return Position(-1,-1);
        }
    }

    // Count the number of set bits in a mask
    // mask on 9 bits max
    static unsigned int CountSetBits(uint64_t mask)
    {
        return _Cache[mask][9];
    }

    // Get a  mask with the nth set bit in a uint64_t starting from the left (most significant bit)
    // Return the position of this bit from the right
    // Warning: counts only amongst the last 9 bits
    static unsigned int GetNthSetBit(uint64_t mask, unsigned int pos)
    {
        return _Cache[mask][pos];
    }

    // Get the nth set bit in a uint64_t starting from the left (most significant bit)
    // Return the position of this bit from the right
    /*static unsigned int GetNthSetBit(uint64_t v, unsigned int r)
    {
        // Code from https://graphics.stanford.edu/~seander/bithacks.html#SelectPosFromMSBRank
        // "Select the bit position (from the most-significant bit) with the given count (rank)"
        // Modified version to have the input rank in the range [0,63]
        //uint64_t v;          // Input value to find position with rank r.
        //unsigned int r;      // Input: bit's desired rank [1-64].
        unsigned int s;      // Output: Resulting position of bit with rank r [1-64]
        uint64_t a, b, c, d; // Intermediate temporaries for bit count.
        unsigned int t;      // Bit count temporary.

        r++; // Added because input r is in [0,63], copied algo is written to have r in [1,64]

        // Do a normal parallel bit count for a 64-bit integer,                     
        // but store all intermediate steps.                                        
        // a = (v & 0x5555...) + ((v >> 1) & 0x5555...);
        a =  v - ((v >> 1) & ~0UL/3);
        // b = (a & 0x3333...) + ((a >> 2) & 0x3333...);
        b = (a & ~0UL/5) + ((a >> 2) & ~0UL/5);
        // c = (b & 0x0f0f...) + ((b >> 4) & 0x0f0f...);
        c = (b + (b >> 4)) & ~0UL/0x11;
        // d = (c & 0x00ff...) + ((c >> 8) & 0x00ff...);
        d = (c + (c >> 8)) & ~0UL/0x101;
        t = (d >> 32) + (d >> 48);
        // Now do branchless select!                                                
        s  = 64;
        // if (r > t) {s -= 32; r -= t;}
        s -= ((t - r) & 256) >> 3; r -= (t & ((t - r) >> 8));
        t  = (d >> (s - 16)) & 0xff;
        // if (r > t) {s -= 16; r -= t;}
        s -= ((t - r) & 256) >> 4; r -= (t & ((t - r) >> 8));
        t  = (c >> (s - 8)) & 0xf;
        // if (r > t) {s -= 8; r -= t;}
        s -= ((t - r) & 256) >> 5; r -= (t & ((t - r) >> 8));
        t  = (b >> (s - 4)) & 0x7;
        // if (r > t) {s -= 4; r -= t;}
        s -= ((t - r) & 256) >> 6; r -= (t & ((t - r) >> 8));
        t  = (a >> (s - 2)) & 0x3;
        // if (r > t) {s -= 2; r -= t;}
        s -= ((t - r) & 256) >> 7; r -= (t & ((t - r) >> 8));
        t  = (v >> (s - 1)) & 0x1;
        // if (r > t) s--;
        s -= ((t - r) & 256) >> 8;
        //s = 64 - s;
        s--; // Modified so s is in [0,63] from the right
        
        return s;
    }*/

private:
    array<uint64_t, 9> _masks;
    int _subgrid; // The last subgrid in which a player was set

    // Cache: array of all the possible subgrid masks
    // For each subgrid mask, elements 0 to 9 are the mask of the nth bit set to 1;
    // the 10th (index 9) is the count of 1s in this mask
    static array<array<uint64_t, 10>, 512> _Cache;
};

array<array<uint64_t, 10>, 512> PositionMask::_Cache;


class SubGrid
{
public:
    SubGrid(): _mySpaces(0UL), _enemySpaces(0UL), _emptySpaces(0b111111111), _winner(UNDEFINED)
    {}

    Player get(int x, int y) const
    {
        if (_mySpaces & (1UL << (x + y*3)))
            return ME;
        else if (_enemySpaces & (1UL << (x + y*3)))
            return ENEMY;
        else
            return NONE;
    }

    void set(uint64_t inputMask, Player owner)
    {
        _winner = UNDEFINED;
        uint64_t mask = 0UL;
        switch (inputMask)
        {
        case 0b000000001:
            mask = 0b000001000000001000000001UL;
            break;
        case 0b000000010:
            mask = 0b000000000001000000000010UL;
            break;
        case 0b000000100:
            mask = 0b001000001000000000000100UL;
            break;
        case 0b000001000:
            mask = 0b000000000000010000001000UL;
            break;
        case 0b000010000:
            mask = 0b010010000010000000010000UL;
            break;
        case 0b000100000:
            mask = 0b000000010000000000100000UL;
            break;
        case 0b001000000:
            mask = 0b100000000000100001000000UL;
            break;
        case 0b010000000:
            mask = 0b000000000100000010000000UL;
            break;
        case 0b100000000:
            mask = 0b000100100000000100000000UL;
            break;
        default:
            break;
        }
        if (owner == ME)
        {
            _mySpaces |= mask;
            _emptySpaces &= ~inputMask;
        }
        else if (owner == ENEMY)
        {
            _enemySpaces |= mask;
            _emptySpaces &= ~inputMask;
        }
        else
        {
            _mySpaces &= ~mask;
            _enemySpaces &= ~mask;
            _emptySpaces |= inputMask;
        }
    }

    void set(int x, int y, Player owner)
    {
        _winner = UNDEFINED;
        int posId = x + y*3;
        uint64_t mask = 0UL;
        switch (posId)
        {
        case 0:
            mask = 0b000001000000001000000001UL;
            break;
        case 1:
            mask = 0b000000000001000000000010UL;
            break;
        case 2:
            mask = 0b001000001000000000000100UL;
            break;
        case 3:
            mask = 0b000000000000010000001000UL;
            break;
        case 4:
            mask = 0b010010000010000000010000UL;
            break;
        case 5:
            mask = 0b000000010000000000100000UL;
            break;
        case 6:
            mask = 0b100000000000100001000000UL;
            break;
        case 7:
            mask = 0b000000000100000010000000UL;
            break;
        case 8:
            mask = 0b000100100000000100000000UL;
            break;
        default:
            break;
        }
        if (owner == ME)
        {
            _mySpaces |= mask;
            _emptySpaces &= ~(1UL << posId);
        }
        else if (owner == ENEMY)
        {
            _enemySpaces |= mask;
            _emptySpaces &= ~(1UL << posId);
        }
        else
        {
            _mySpaces &= ~mask;
            _enemySpaces &= ~mask;
            _emptySpaces |= (1UL << posId);
        }
    }

    uint64_t getEmptySpacesMask() const
    {
        return _emptySpaces;
    }

    int countEmptySpaces() const
    {
        return PositionMask::CountSetBits(_emptySpaces);
    }

    bool hasEmptySpaces() const
    {
        return getEmptySpacesMask() != 0;
    }

    Player getWinner() const
    {
        if (_winner != UNDEFINED)
            return _winner;
        static uint64_t winMask = 0b001001001001001001001001UL;
        if (_mySpaces & (_mySpaces >> 1) & (_mySpaces >> 2) & winMask)
        {
            _winner = ME;
            return ME;
        }
        if (_enemySpaces & (_enemySpaces >> 1) & (_enemySpaces >> 2) & winMask)
        {
            _winner = ENEMY;
            return ENEMY;
        }
        else
        {
            _winner = NONE;
            return NONE;
        }
    }

    bool completed() const
    {
        return (getWinner() != NONE || !hasEmptySpaces());
    }

    uint64_t hash() const
    {
        return _mySpaces | (_enemySpaces << 24);
    }

    bool operator==(const SubGrid& other) const
    {
        return _mySpaces == other._mySpaces && _enemySpaces == other._enemySpaces;
    }

    bool operator!=(const SubGrid& other) const
    {
        return !(*this == other);
    }

    bool operator<(const SubGrid& other) const
    {
        return hash() < other.hash();
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
                    str += G_myDisplay;
                    break;
                case ENEMY:
                    str += G_enemyDisplay;
                    break;
                default:
                    break;
                }
            }
            str += '\n';
        }
        return str;
    }

private:
    uint64_t _mySpaces;
    uint64_t _enemySpaces;
    uint64_t _emptySpaces;
    mutable Player _winner;

    static const uint64_t _EmptyMask;
};

const uint64_t SubGrid::_EmptyMask = 0b111111111;


typedef array<Position, 81> movesBuffer_t;


class Grid
{
public:
    Grid(): _lastPlay(-1, -1)
    {}

    Player get(int x, int y) const
    {
        return getSubGridByPos(x, y).get(x % 3, y % 3);
    }

    void set(int x, int y, Player owner)
    {
        getSubGridByPos(x, y).set(x % 3, y % 3, owner);
        _lastPlay.x = x;
        _lastPlay.y = y;
    }

    // Set only the first position of the mask
    void set(const PositionMask& posMask, Player owner)
    {
        _lastPlay = PositionMask::GetPositionFromSubMask(posMask.get(posMask.lastSetSubgrid()));
        _subGrids[posMask.lastSetSubgrid()].set(posMask.get(posMask.lastSetSubgrid()), owner);
    }

    int getAllowedMoves(movesBuffer_t& allowed) const
    {
        int counter = 0;
        if (_lastPlay.x != -1 && !getSubGridByPos(3*(_lastPlay.x%3), 3*(_lastPlay.y%3)).completed())
        {
            for (int x = 3*(_lastPlay.x%3); x < 3 + 3*(_lastPlay.x%3); x++)
            {
                for (int y = 3*(_lastPlay.y%3); y < 3 + 3*(_lastPlay.y%3); y++)
                {
                    if (get(x,y) == NONE)
                    {
                        allowed[counter++] = Position(x,y);
                    }
                }
            }
        }
        if (counter == 0)
        {
            for (int x = 0; x < 9; x++)
            {
                for (int y = 0; y < 9; y++)
                {
                    if (get(x,y) == NONE && !getSubGridByPos(x,y).completed())
                    {
                        allowed[counter++] = Position(x,y);
                    }
                }
            }
        }
        return counter;
    }

    void getAllowedMoves(PositionMask& posMask)
    {
        const SubGrid& subGridToPlay = getSubGridByPos(3*(_lastPlay.x%3), 3*(_lastPlay.y%3));
        if (_lastPlay.x != -1 && !subGridToPlay.completed())
        {
            posMask.set(getSubGridIdByPos(3*(_lastPlay.x%3), 3*(_lastPlay.y%3)), subGridToPlay.getEmptySpacesMask());
        }
        else
        {
            int subGridId = 0;
            for (const SubGrid& subGrid : _subGrids)
            {
                if (!subGrid.completed())
                {
                    posMask.set(subGridId, subGrid.getEmptySpacesMask());
                }
                subGridId++;
            }
        }
    }

    Player getWinner() const
    {
        // Check if the meta tic-tac-toe is won
        if (getSubGrid(1,1).getWinner() != NONE)
        {
            if ((getSubGrid(1,1).getWinner() == getSubGrid(0,0).getWinner() && getSubGrid(1,1).getWinner() == getSubGrid(2,2).getWinner())
             || (getSubGrid(1,1).getWinner() == getSubGrid(1,0).getWinner() && getSubGrid(1,1).getWinner() == getSubGrid(1,2).getWinner())
             || (getSubGrid(1,1).getWinner() == getSubGrid(2,0).getWinner() && getSubGrid(1,1).getWinner() == getSubGrid(0,2).getWinner())
             || (getSubGrid(1,1).getWinner() == getSubGrid(0,1).getWinner() && getSubGrid(1,1).getWinner() == getSubGrid(2,1).getWinner()))
            {
                return getSubGrid(1,1).getWinner();
            }
        }
        if (getSubGrid(0,0).getWinner() != NONE)
        {
            if ((getSubGrid(0,0).getWinner() == getSubGrid(1,0).getWinner() && getSubGrid(0,0).getWinner() == getSubGrid(2,0).getWinner())
             || (getSubGrid(0,0).getWinner() == getSubGrid(0,1).getWinner() && getSubGrid(0,0).getWinner() == getSubGrid(0,2).getWinner()))
            {
                return getSubGrid(0,0).getWinner();
            }
        }
        if (getSubGrid(2,2).getWinner() != NONE)
        {
            if ((getSubGrid(2,2).getWinner() == getSubGrid(2,0).getWinner() && getSubGrid(2,2).getWinner() == getSubGrid(2,1).getWinner())
             || (getSubGrid(2,2).getWinner() == getSubGrid(0,2).getWinner() && getSubGrid(2,2).getWinner() == getSubGrid(1,2).getWinner()))
            {
                return getSubGrid(2,2).getWinner();
            }
        }
        // If not, won may be awarded if all the subgrid are completed; count the number of subgrids won
        int myCounter = 0;
        int enemyCounter = 0;
        for (const SubGrid& subGrid : _subGrids)
        {
            if (subGrid.completed())
            {
                switch (subGrid.getWinner())
                {
                case ME:
                    myCounter++;
                    break;
                case ENEMY:
                    enemyCounter++;
                    break;
                default:
                    break;
                }
            }
            else
            {
                // If there is at least 1 subgrid uncompleted, there is no winner yet
                return UNDEFINED;
            }
        }
        if (myCounter > enemyCounter)
        {
            //DBG("Winner by count " << myCounter << " vs " << enemyCounter);
            return ME;
        }
        else if (myCounter < enemyCounter)
        {
            //DBG("Winner by count " << myCounter << " vs " << enemyCounter);
            return ENEMY;
        }
        else
        {
            return NONE;
        }
    }

    int countEmptySpaces() const
    {
        int count = 0;
        for (const SubGrid& subGrid : _subGrids)
        {
            count += subGrid.countEmptySpaces();
        }
        return count;
    }

    bool operator==(const Grid& other) const
    {
        for (int i = 0; i < 9; i++)
        {
            if (_subGrids[i] != other._subGrids[i])
                return false;
        }
        return true;
    }

    bool operator<(const Grid& other) const
    {
        for (int i = 0; i < 9; i++)
        {
            if (_subGrids[i].hash() < other._subGrids[i].hash())
                return true;
        }
        return false;
    }

    string toString() const
    {
        string str;
        for (int y = 0; y < 9; y++)
        {
            if (y % 3 == 0 && y > 0)
                str += "---+---+---\n";
            for (int x = 0; x < 9; x++)
            {
                if (x % 3 == 0 && x > 0)
                    str += '|';
                switch (get(x,y))
                {
                case NONE:
                    str += ".";
                    break;
                case ME:
                    str += G_myDisplay;
                    break;
                case ENEMY:
                    str += G_enemyDisplay;
                    break;
                default:
                    break;
                }
            }
            str += '\n';
        }
        return str;
    }

private:
    // Subgrid position in input
    SubGrid& getSubGrid(int x, int y)
    {
        return _subGrids[x + y*3];
    }
    const SubGrid& getSubGrid(int x, int y) const
    {
        return _subGrids[x + y*3];
    }

    // Subgrid by global position
    int getSubGridIdByPos(int x, int y)
    {
        return (x/3 + 3*(y/3));
    }
    SubGrid& getSubGridByPos(int x, int y)
    {
        return getSubGrid(x/3, y/3);
    }
    const SubGrid& getSubGridByPos(int x, int y) const
    {
        return getSubGrid(x/3, y/3);
    }

    array<SubGrid, 9> _subGrids;
    Position _lastPlay;
};


class TreeElem
{
public:
    TreeElem(const Grid& grid, TreeElem* parent, Player player, Position move=Position()):
        _grid(grid),
        _parent(parent),
        _player(player),
        _move(move),
        _score(0),
        _plays(0)
    {
    }

    TreeElem* parent()
    {
        return _parent;
    }

    bool isRoot() const
    {
        return _parent == nullptr;
    }

    void setRoot()
    {
        _parent = nullptr;
    }

    vector<TreeElem*>& getChildren()
    {
        return _children;
    }
    const vector<TreeElem*>& getChildren() const
    {
        return _children;
    }

    bool isLeaf() const
    {
        return _children.empty();
    }

    const Grid& grid() const
    {
        return _grid;
    }

    const Player& player() const
    {
        return _player;
    }

    const Position& move() const
    {
        return _move;
    }

    int getAllowedMoves(movesBuffer_t& buffer) const
    {
        return _grid.getAllowedMoves(buffer);
    }

    TreeElem* addChild(Position moveToPlay, Player player)
    {
        // TODO custom allocator for perf (and fix memory leak)
        TreeElem* child = new TreeElem(_grid, this, player, moveToPlay);
        _children.push_back(child);
        child->_grid.set(moveToPlay.x, moveToPlay.y, player);
        return child;
    }

    void addScore(int score)
    {
        _score += score;
        _plays++;
    }

    int score() const
    {
        return _score;
    }

    int plays() const
    {
        return _plays;
    }

    double computeUct() const
    {
        // TODO should we consider the defeat as negative score?
        //DBG(_score << " " << _plays << " " << _parent->_plays);
        if (_plays == 0)
            return INFINITY;
        else
            return ((double)_score/(double)_plays) + /*(1.414*/180. * sqrt(MyLog(_parent->_plays)/(double)_plays);
    }

    TreeElem* getChildWithBestUct() const
    {
        double bestUct = -INFINITY;
        TreeElem* bestChild = nullptr;
        for (TreeElem* child : _children)
        {
            double uct = child->computeUct();
            if (uct > bestUct)
            {
                bestUct = uct;
                bestChild = child;
            }
        }
        return bestChild;
    }

    TreeElem* getChildWithBestAverageScore() const
    {
        double bestScore = -INFINITY;
        TreeElem* bestChild = nullptr;
        for (TreeElem* child : _children)
        {
            if ((double)child->_score/(double)child->_plays > bestScore)
            {
                bestScore = (double)child->_score/(double)child->_plays;
                bestChild = child;
            }
        }
        return bestChild;
    }

    TreeElem* findMove(const Position& move)
    {
        for (TreeElem* child : _children)
        {
            if (child->_move == move)
            {
                return child;
            }
        }
        return nullptr;
    }

    void killBrothers()
    {
        if (!isRoot())
        {
            _parent->_children.clear();
            _parent->_children.push_back(this);
        }
    }

private:
    Grid _grid;
    TreeElem* _parent;
    vector<TreeElem*> _children;
    Player _player; // The player that played to reach this node
    Position _move; // The move that lead to this node
    int _score;
    int _plays;
};


class AI
{
public:
    AI(Grid& grid): _grid(grid), _treeRoot(nullptr), _timeout(1000)
    {
    }

    Position play()
    {
        if (_treeRoot == nullptr)
        {
            _treeRoot = new TreeElem(_grid, nullptr, NONE);
        }
        Position pos = mcts(*_treeRoot);
        // After first turn, timeout is 100 ms
        _timeout = 100;
        return pos;
        //return possibleActions[0];
    }

    // Keep the part of the tree corresponding to the given moves
    // If not existing, start from fresh tree
    void selectTree(const vector<Position>& lastActions)
    {
        // Who cares about memory leaks?
        if (lastActions.size() != 2)
        {
            _treeRoot = nullptr;
            return;
        }
        TreeElem* myPlay = _treeRoot->findMove(lastActions[0]);
        if (myPlay != nullptr)
        {
            _treeRoot = myPlay->findMove(lastActions[1]);
            if (_treeRoot != nullptr)
            {
                _treeRoot->setRoot();
            }
        }
        else
        {
            _treeRoot = nullptr;
        }
    }

    // For testing purpose
    TreeElem& getTreeRoot()
    {
        return *_treeRoot;
    }
    const TreeElem& getTreeRoot() const
    {
        return *_treeRoot;
    }

private:
    // Monte Carlo Tree Search
    // https://vgarciasc.github.io/mcts-viz/
    // https://www.youtube.com/watch?v=UXW2yZndl7U
    Position mcts(TreeElem& root)
    {
        DBG("mcts");
        int loops = 0;
        chrono::time_point<chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();
#ifndef MCTS_LOOPS_LIMIT
        while (chrono::duration_cast<std::chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count() < _timeout)
#else
        for (int i = 0; i < MCTS_LOOPS_LIMIT; i++)
#endif
        {
            TreeElem* selected = selection(root);
            TreeElem* expanded = expansion(*selected);
            int score = simulation(*expanded);
            backpropagation(*expanded, score);
            loops++;
        }
        DBG(loops << " loops");
        // Chose child with best uct
        TreeElem* bestChild = root.getChildWithBestAverageScore();
        //TreeElem* bestChild = root.getChildWithBestUct();
        DBG(bestChild->score() << "/" << bestChild->plays());
        return bestChild->move();
    }

    TreeElem* selection(TreeElem& treeElem)
    {
        if (treeElem.isLeaf())
        {
            return &treeElem;
        }
        else
        {
            return selection(*treeElem.getChildWithBestUct());
        }
    }

    TreeElem* expansion(TreeElem& treeElem)
    {
        if (treeElem.plays() > 0 || treeElem.isRoot())
        {
            // Add all possible children
            movesBuffer_t allowedMoves;
            int movesCount = treeElem.getAllowedMoves(allowedMoves);
            if (movesCount > 0)
            {
                for (int i = 0; i < movesCount; i++)
                {
                    treeElem.addChild(allowedMoves[i], nextPlayer(treeElem.player()));
                }
                return treeElem.getChildren()[0];
            }
            else
            {
                return &treeElem;
            }
        }
        else
        {
            return &treeElem;
        }
    }

    int simulation(TreeElem& treeElem)
    {
        Grid grid = treeElem.grid();
        Player player = treeElem.player();
        Player winner = grid.getWinner();
        if (winner == player)
        {
            treeElem.killBrothers();
        }
        while (winner == UNDEFINED)
        {
            PositionMask allowedMoves;
            grid.getAllowedMoves(allowedMoves);
            PositionMask picked = allowedMoves.getNthBitSet(Random::Rand(allowedMoves.countSetBits()));
            player = nextPlayer(player);
            grid.set(picked, player);
            winner = grid.getWinner();
        }
        switch (winner)
        {
        case ME:
            return 81 - grid.countEmptySpaces();
            //return 2;
        case ENEMY:
            return grid.countEmptySpaces() - 81;
            //return 0;//-1;
        default:
            return 0;
            //return 1;
        }
    }

    void backpropagation(TreeElem& treeElem, int score)
    {
        treeElem.addScore(score);
        TreeElem* currentElem = &treeElem;
        while (!currentElem->isRoot())
        {
            currentElem = currentElem->parent();
            currentElem->addScore(score);
        }
    }

    Player nextPlayer(Player player)
    {
        switch (player)
        {
        case NONE:
            // First play is always for me
            return ME;
        case ME:
            return ENEMY;
        case ENEMY:
            return ME;
        default:
            return UNDEFINED;
        }
    }

    Grid& _grid;
    TreeElem* _treeRoot;
    int _timeout;
};


/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

#ifndef LOCAL
int main()
{
    Random::Init();
    PositionMask::InitCache();

    Grid grid;
    AI ai(grid);
    vector<Position> lastActions;

    // game loop
    while (1) {
        int opponent_row;
        int opponent_col;
        cin >> opponent_row >> opponent_col; cin.ignore();
        if (opponent_row == -1)
        {
            G_myDisplay = "X";
            G_enemyDisplay = "O";
        }
        else
        {
            grid.set(opponent_col, opponent_row, ENEMY);
            if (lastActions.size() == 2)
            {
                lastActions[1] = Position(opponent_col, opponent_row);
            }
        }
        int valid_action_count;
        vector<Position> possibleActions;
        cin >> valid_action_count; cin.ignore();
        for (int i = 0; i < valid_action_count; i++) {
            int row;
            int col;
            cin >> row >> col; cin.ignore();
            possibleActions.push_back(Position(col, row));
        }

        DBG(grid.toString());

        ai.selectTree(lastActions);
        Position pos = ai.play();
        grid.set(pos.x, pos.y, ME);

        lastActions.resize(2);
        lastActions[0] = pos;

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;
        cout << pos.y << " " << pos.x << endl;
    }
}
#endif