#define LOCAL
#define MCTS_LOOPS_LIMIT 10000
//#define DISABLE_OPTIMS

#include "deployed.cpp"

#include <assert.h>


Grid BuildGrid(const string& str, Player first)
{
    Grid grid;
    int i = 0;
    for (char c : str)
    {
        switch (c)
        {
        case '.':
            grid.set(i%9, i/9, NONE);
            i++;
            break;
        case 'X':
            grid.set(i%9, i/9, first);
            i++;
            break;
        case 'O':
            grid.set(i%9, i/9, first == ME ? ENEMY : ME);
            i++;
            break;
        default:
            break;
        }
    }
    return grid;
}


// Tests here

bool testMyLog()
{
    assert(MyLog(1) == log(1));
    assert(MyLog(1) == log(1));
    assert(MyLog(2) == log(2));
    assert(MyLog(123456) == log(123456));
    assert(MyLog(2) == log(2));
    assert(MyLog(123456) == log(123456));
    return true;
}


bool testSubGrid()
{
    SubGrid subgrid;
    assert(subgrid.getWinner() == NONE);
    assert(subgrid.get(1,0) == NONE);
    assert(subgrid.getEmptySpacesMask() == 0b111111111);
    assert(subgrid.hasEmptySpaces());
    subgrid.set(1,0,ME);
    assert(subgrid.get(1,0) == ME);
    subgrid.set(0,0,ENEMY);
    assert(subgrid.get(0,0) == ENEMY);
    subgrid.set(1,1,ME);
    assert(subgrid.getWinner() == NONE);
    subgrid.set(2,2,ENEMY);
    assert(!subgrid.completed());
    subgrid.set(1,2,ME);
    assert(subgrid.getWinner() == ME);
    assert(subgrid.completed());
    assert(subgrid.getEmptySpacesMask() == 0b001101100);
    assert(subgrid.hasEmptySpaces());
    subgrid.set(0b001000000, ENEMY);
    subgrid.set(0b000100000, ME);
    subgrid.set(0b000001000, ENEMY);
    subgrid.set(0b000000100, ME);
    assert(!subgrid.hasEmptySpaces());

    return true;
}


bool testGetWinner()
{
    Grid grid = BuildGrid("X.O|...|OXX"
                          "XXO|OOO|O.X"
                          "OOX|...|XOO"
                          "---+---+---"
                          "...|XOO|OXX"
                          "OOO|X.X|XXX"
                          "X..|.XX|OO."
                          "---+---+---"
                          ".X.|XXO|X.."
                          "...|..O|X.X"
                          "OOO|X.O|XOO",
                          ME);
    assert(grid.get(3,2) == NONE);
    assert(grid.get(7,4) == ME);
    assert(grid.get(0,8) == ENEMY);
    assert(grid.getWinner() == UNDEFINED);
    assert(grid.countEmptySpaces() == 27);
    grid.set(4,4,ME);
    assert(grid.get(4,4) == ME);
    assert(grid.getWinner() == ME);
    assert(grid.countEmptySpaces() == 26);
    return true;
}


bool testPositionMask()
{
    Grid grid;
    PositionMask pos1;
    // 8,4
    pos1.set(5, 0b000100000);
    assert(pos1.get(2) == 0b000000000);
    assert(pos1.get(5) == 0b000100000);
    PositionMask pos2;
    // 5,2
    pos2.set(1, 0b100000000);
    grid.set(pos1, ME);
    grid.set(pos2, ENEMY);
    assert(grid.get(8,4) == ME);
    assert(grid.get(5,2) == ENEMY);
    grid.set(pos2, NONE);
    assert(grid.get(5,2) == NONE);
    
    return true;
}


bool testPositionMaskCounters()
{
    assert(PositionMask::GetPositionFromSubMask(0b1) == Position(0,0));
    assert(PositionMask::GetPositionFromSubMask(0b10000) == Position(1,1));
    assert(PositionMask::GetPositionFromSubMask(0b100000000) == Position(2,2));

    // Invalid if mask is 0, will always return 0
    assert(PositionMask::GetNthSetBit(0UL, 0) == 0);
    assert(PositionMask::GetNthSetBit(0UL, 1) == 0);
    assert(PositionMask::GetNthSetBit(0b1, 0) == 0b1);
    // If not enough bits set, return 0
    assert(PositionMask::GetNthSetBit(0b1, 1) == 0);
    assert(PositionMask::GetNthSetBit(0b101, 0) == 0b100);
    assert(PositionMask::GetNthSetBit(0b101, 1) == 0b001);
    assert(PositionMask::GetNthSetBit(0b101101101, 0) == 0b100000000);
    assert(PositionMask::GetNthSetBit(0b101101101, 1) == 0b001000000);
    assert(PositionMask::GetNthSetBit(0b101101101, 2) == 0b000100000);
    assert(PositionMask::GetNthSetBit(0b101101101, 3) == 0b000001000);
    assert(PositionMask::GetNthSetBit(0b101101101, 4) == 0b000000100);
    assert(PositionMask::GetNthSetBit(0b101101101, 5) == 0b000000001);

    PositionMask pos;
    pos.set(0, 0b100100100);
    pos.set(4, 0b000111000);
    assert(pos.countSetBits() == 6);
    PositionMask pos0 = pos.getNthBitSet(0);
    PositionMask pos1 = pos.getNthBitSet(1);
    PositionMask pos2 = pos.getNthBitSet(2);
    PositionMask pos3 = pos.getNthBitSet(3);
    PositionMask pos4 = pos.getNthBitSet(4);
    PositionMask pos5 = pos.getNthBitSet(5);
    for (int i = 0; i < 9; i++)
    {
        if (i == 0)
        {
            assert(pos0.get(i) == 0b100000000);
            assert(pos1.get(i) == 0b000100000);
            assert(pos2.get(i) == 0b000000100);
            assert(pos3.get(i) == 0);
            assert(pos4.get(i) == 0);
            assert(pos5.get(i) == 0);
        }
        else if (i == 4)
        {
            assert(pos0.get(i) == 0);
            assert(pos1.get(i) == 0);
            assert(pos2.get(i) == 0);
            assert(pos3.get(i) == 0b000100000);
            assert(pos4.get(i) == 0b000010000);
            assert(pos5.get(i) == 0b000001000);
        }
        else
        {
            assert(pos0.get(i) == 0);
            assert(pos1.get(i) == 0);
            assert(pos2.get(i) == 0);
            assert(pos3.get(i) == 0);
            assert(pos4.get(i) == 0);
            assert(pos5.get(i) == 0);
        }
    }

    return true;
}


bool testGetAllowedMoves()
{
    movesBuffer_t buffer;

    // Empty grid
    Grid grid;
    assert(grid.getAllowedMoves(buffer) == 81);
    PositionMask posMask1;
    grid.getAllowedMoves(posMask1);
    for (int i = 0; i < 9; i++)
    {
        assert(posMask1.get(i) == 0b111111111);
    }
    
    // Subgrid must be selected
    grid.set(1, 1, ENEMY);
    int count = grid.getAllowedMoves(buffer);
    assert(count == 9);
    for (int i = 0; i < count; i++)
    {
        Position pos = buffer[i];
        assert(pos.x >= 3 && pos.x < 6 && pos.y >= 3 && pos.y < 6);
    }
    PositionMask posMask2;
    grid.getAllowedMoves(posMask2);
    for (int i = 0; i < 9; i++)
    {
        if (i == 4)
            assert(posMask2.get(i) == 0b111111111);
        else
            assert(posMask2.get(i) == 0);
    }

    // Last play in a completed subgrid
    grid = BuildGrid(   "X.O|...|OXX"
                        "XXO|OOO|O.X"
                        "OOX|...|XOO"
                        "---+---+---"
                        "...|XOO|OXX"
                        "OOO|X.X|XXX"
                        "X..|.XX|OO."
                        "---+---+---"
                        ".X.|XXO|X.."
                        "...|..O|X.X"
                        "OOO|X.O|XOO",
                        ME);
    // Force to set last play
    grid.set(0, 8, ENEMY);
    count = grid.getAllowedMoves(buffer);
    assert(count == 3);
    for (int i = 0; i < count; i++)
    {
        Position pos = buffer[i];
        assert(pos == Position(3,5) || pos == Position(4,4) || pos == Position(7,1));
    }
    PositionMask posMask3;
    grid.getAllowedMoves(posMask3);
    for (int i = 0; i < 9; i++)
    {
        if (i == 2)
            assert(posMask3.get(i) == 0b000010000);
        else if (i == 4)
            assert(posMask3.get(i) == 0b001010000);
        else
            assert(posMask3.get(i) == 0);
    }

    return true;
}


bool testMctsFinalize()
{
    // Make sure we always pick the winning move at the end
    Grid grid = BuildGrid(  "XXX|XXX|OOO"
                            "...|...|..."
                            "...|...|..."
                            "---+---+---"
                            "OOO|OOO|XXX"
                            "...|...|..."
                            "...|...|..."
                            "---+---+---"
                            "XXX|OOO|XO."
                            "...|...|OXX"
                            "...|...|OO.",
                            ME);
    // Force to set last play
    grid.set(8, 0, ENEMY);
    AI ai(grid);
    assert(ai.play() == Position(8,8));

    return true;
}


bool testWhyDoILose()
{
    Grid grid = BuildGrid(  "...|...|..."
                            "...|...|..."
                            "...|...|..."
                            "---+---+---"
                            "...|O..|..."
                            "...|.X.|..."
                            "...|...|..."
                            "---+---+---"
                            "...|...|..."
                            "...|...|..."
                            "...|...|...",
                            ME);
    grid.set(3, 3, ENEMY);
    AI ai(grid);
    DBG(grid.toString());
    Position pos = ai.play();
    DBG("Play: " << pos.toString());
    const TreeElem& root = ai.getTreeRoot();
    for (const TreeElem* child : root.getChildren())
    {
        DBG(child->move().toString() << " " << child->score() << "/" << child->plays()
            << " (" << (double)child->score()/(double)child->plays() << ") uct: " << child->computeUct());
    }

    return true;
}


bool testReuseTree()
{
    Grid grid = BuildGrid(  "XXX|XXX|OOO"
                            "...|...|..."
                            "...|...|..."
                            "---+---+---"
                            "OOO|OOO|XXX"
                            "...|...|..."
                            "...|...|..."
                            "---+---+---"
                            "XXX|OOO|..."
                            "...|...|..."
                            "...|...|...",
                            ME);
    // Force to set last play
    grid.set(8, 0, ENEMY);
    AI ai(grid);
    Position playedByMe = ai.play();
    assert(ai.getTreeRoot().getChildren().size() == 9);
    vector<Position> lastMoves = {Position(6,6), Position(6,7)};
    ai.selectTree(lastMoves);
    assert(ai.getTreeRoot().grid().get(6,6) == ME);
    assert(ai.getTreeRoot().grid().get(6,7) == ENEMY);
    assert(ai.getTreeRoot().plays() > 0);
    return true;
}


bool testMctsPerf()
{
    Grid grid;
    AI ai(grid);
    grid.set(7,8,ENEMY);
    DBG(grid.toString());
    DBG(ai.play().toString());
}


/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/

int main()
{
    Random::Init();
    PositionMask::InitCache();

    G_myDisplay = "X";
    G_enemyDisplay = "O";

    // testMyLog();
    // testSubGrid();
    // testGetWinner();
    // testPositionMask();
    // testPositionMaskCounters();
    // testGetAllowedMoves();
    // testMctsFinalize();
    // testReuseTree();

    // testWhyDoILose();

    testMctsPerf();

    DBG("All test passed");
}