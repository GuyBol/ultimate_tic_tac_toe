#define LOCAL
#define UNDEF_DBG

#include "deployed.cpp"
namespace aiReference
{
#include "aiReference.cpp"
}

#define SIMULATOR_DBG(stream) std::cerr << stream << std::endl;


template <typename T>
void cleanTreeMemory(T* treeElem)
{
    if (treeElem->isLeaf())
    {
        delete treeElem;
    }
    else
    {
        for (T* child : treeElem->getChildren())
        {
            cleanTreeMemory(child);
        }
    }
}


Player playGame(Player player)
{
    // Check who starts first
    if (player == ME)
    {
        G_myDisplay = "X";
        G_enemyDisplay = "O";
        aiReference::G_myDisplay = "O";
        aiReference::G_enemyDisplay = "X";
    }
    else if (player == ME)
    {
        G_myDisplay = "O";
        G_enemyDisplay = "X";
        aiReference::G_myDisplay = "X";
        aiReference::G_enemyDisplay = "O";
    }
    // The simulator grid
    Grid grid;
    // Create my player
    Grid myGrid;
    AI myAi(myGrid);
    vector<Position> lastActionsForMe;
    // Create enemy player
    aiReference::Grid enemyGrid;
    aiReference::AI enemyAi(enemyGrid);
    vector<aiReference::Position> lastActionsForEnemy;
    // To keep control on memory
    TreeElem* myTreeRoot = nullptr;
    aiReference::TreeElem* enemyTreeRoot = nullptr;
    // Play loop
    while (grid.getWinner() == UNDEFINED)
    {
        //DBG(grid.toString());
        if (player == ME)
        {
            myAi.selectTree(lastActionsForMe);
            Position pos = myAi.play();
            if (myTreeRoot == nullptr)
                myTreeRoot = &myAi.getTreeRoot();
            myGrid.set(pos.x, pos.y, ME);
            grid.set(pos.x, pos.y, ME);
            enemyGrid.set(pos.x, pos.y, aiReference::ENEMY);
            lastActionsForMe.resize(2);
            lastActionsForMe[0] = pos;
            if (lastActionsForEnemy.size() == 2)
                lastActionsForEnemy[1] = aiReference::Position(pos.x, pos.y);
            player = ENEMY;
        }
        else if (player == ENEMY)
        {
            enemyAi.selectTree(lastActionsForEnemy);
            aiReference::Position pos = enemyAi.play();
            if (enemyTreeRoot == nullptr)
                enemyTreeRoot = &enemyAi.getTreeRoot();
            enemyGrid.set(pos.x, pos.y, aiReference::ME);
            myGrid.set(pos.x, pos.y, ENEMY);
            grid.set(pos.x, pos.y, ENEMY);
            lastActionsForEnemy.resize(2);
            lastActionsForEnemy[0] = pos;
            if (lastActionsForMe.size() == 2)
                lastActionsForMe[1] = Position(pos.x, pos.y);
            player = ME;
        }
    }
    SIMULATOR_DBG(grid.toString());
    cleanTreeMemory(myTreeRoot);
    cleanTreeMemory(enemyTreeRoot);
    return grid.getWinner();
}


int main()
{
    int runs = 20;
    int wins = 0;
    int losses = 0;
    int ties = 0;
    Player player = ME;
    for (int i = 0; i < runs; i++)
    {
        Player winner = playGame(player);
        if (winner == ME)
        {
            wins++;
            SIMULATOR_DBG("Winner: ME");
        }
        if (winner == ENEMY)
        {
            losses++;
            SIMULATOR_DBG("Winner: ENEMY");
        }
        if (winner == NONE)
        {
            ties++;
            SIMULATOR_DBG("Winner: NONE");
        }
        if (player == ME)
            player = ENEMY;
        else if (player == ENEMY)
            player = ME;
    }
    SIMULATOR_DBG("Wins: " << wins << "/" << wins + losses + ties << " (" << 100 * wins / (wins + losses + ties) << "%)");
    SIMULATOR_DBG("Losses: " << losses << "/" << wins + losses + ties << " (" << 100 * losses / (wins + losses + ties) << "%)");
    SIMULATOR_DBG("Ties: " << ties << "/" << wins + losses + ties << " (" << 100 * ties / (wins + losses + ties) << "%)");
}
