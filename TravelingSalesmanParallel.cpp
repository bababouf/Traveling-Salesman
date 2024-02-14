#include <iostream>
#include <queue>
#include <iomanip>
#include <vector>
#include <thread>


// In this program, a node contains information that pertains to a route being expanded (or one that has reached full expansion).
struct node{
    std::vector<std::vector<int>> configurationMatrix; // 2D matrix that contains useful information to identify which edges can be included/excluded
    double lowerBound; // Cost of the route at it's current point in expansion. Ex: a -> b : lowerBound = 4 (4 is the cost from a -> b)
    std::pair<int, int> constraint; // Contains the cell (<row><column>) that the node has reached. Important for determining when a route is found (no more cities to visit) 
    std::vector<int> V; 
    bool include;
    bool exclude;
};
  
int readInSimulationMode();
node initializeConfigurationMatrix();
void setAdjacencyMatrix();
void calculateLowerBoundForNode(node &s); 
void nodeExpansionDispatcher();
node acquireUnprocessedNode();
void checkInclude(node poppedNode, int id);
void checkExclude(node poppedNode, int id);
void checkConstraint(node &s);
bool pruneNodes();
bool updateNodeConstraint(node &s);
void modifyMatrix(node &s, bool include);
void print(node s);

// Compares the lower bounds of two nodes, returns true if p1.lB > p2.lB
struct Comparator {
    bool operator()(node const& p1, node const& p2)
    {        
        return p1.lowerBound > p2.lowerBound;
    }
};

struct ProgramVariables {
    /*
        Priority_Queue is a container adapter in the C++ standard library. The first parameter "node" specifies the type being stored.
        The second parameter vector<node> specifies how to store that type, which in this case is a vector of nodes. The third parameter 
        specifies the comparison function which determines how the nodes are stored. 
    */
    std::priority_queue<node, std::vector<node>, Comparator> foundRoutes; // Contains all routes found 
    std::priority_queue<node, std::vector<node>, Comparator> unprocessedNodesQueue; // Contains all nodes that can still be expanded (not complete routes)
    std::vector<std::vector<int> > adjacencyMatrix; // 2D matrix where each cell is the cost between cities. Ex: [a][b] = cost from a->b, b->a

    bool endProgram = false; 
    int numberOfCitiesToVisit;
};

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;

ProgramVariables programVariables;

int main()
{
    programVariables.numberOfCitiesToVisit =  readInSimulationMode(); // Sets global int "numberOfCitiesToVisit" value
    node root = initializeConfigurationMatrix(); 
    setAdjacencyMatrix();
    calculateLowerBoundForNode(root);
    programVariables.unprocessedNodesQueue.push(root);
    nodeExpansionDispatcher(); // Starts the multithreaded route discovery process

    return 0;
   
}

/*
    This method will prompt the user to enter '5' for the 5-city simulation, or '6' for the 6-city simulation.
    Invalid inputs (anything other than numerical '5' or '6') will be rejected and prompt the user to enter a new choice.
*/
int readInSimulationMode()
{
    int input;
    bool validInput = false;

    std::cout << "The salesman is almost ready to embark on their journey. \n " 
        << "Select which simulation to run:  \n "
        << "\t For 5-city simulation, enter '5'  \n "
        << "\t For 6-city simulation, enter '6'  \n ";

    do
    {
        std::cout << "\t Choice: ";
        std::cin >> input;

        if (std::cin.fail() || input > 6 || input < 5) // Cin.fail() is true when a non-integer input is entered, due to cin >> input (input is integer)
        { 
            std::cout << "Invalid input."  << std::endl << std::endl;
            std::cin.clear(); // Clears error status flags from the invalid input 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignores all characters up to the \n (newline character)
        }
        else 
        {
            validInput = true;
        }

    } while (!validInput); 
    
    return input;
}

/*
    The configurationMatrix is a 2D matrix associated with each node. The size of the matrix depends on the numberOfCities that will be used in 
    the simulation. For example, if the user chose the 5-city simulation, the 2D matrix will be of size [5][7]. The 5 x 5 part of the matrix contains 
    either '0', '1', or '-1'. A cell will contain '1' if the edge has been included in the node's current route, and '-1' if the edge has been excluded 
    (meaning that edge is not in the route). It was stated that the size of the 5-city 2D configurationMatrix for a node is [5][7], and this is because
    two extra columns are needed to hold information to decipher whether an edge for that row can be included or excluded. The decision is made with the 
    information in each of these columns.

    The method below initializes the matrix discussed above. To continue with the 5-city example, this method will set the 5 x 5 cells to all '0s'. The last
    column will be set to all '4s', and the 2nd to last column will again be set to all '0s'. More information can be found in the readME file.
*/
node initializeConfigurationMatrix()
{
    node root;
    int numberOfColumns = programVariables.numberOfCitiesToVisit + 2; 
    int lastColumn = programVariables.numberOfCitiesToVisit + 1;
    std::vector<std::vector<int> > configurationMatrix(programVariables.numberOfCitiesToVisit, std::vector<int>(numberOfColumns)); // Creates 2D vector of ints, with row size = # of cities, column size = (# cities + 2)

    for(int row = 0; row < programVariables.numberOfCitiesToVisit; row++)
        {
            for(int column = 0; column < (numberOfColumns); column++)
            {
                if(column == lastColumn) 
                {
                    configurationMatrix[row][column] = (programVariables.numberOfCitiesToVisit - 1);
                }
                else 
                {
                    configurationMatrix[row][column] = 0;
                }  
            } 
        }
        
        root.configurationMatrix = configurationMatrix;
        root.constraint.first = 0;
        root.constraint.second = 0;
        
        return root;
        
}

/*
    Depending on whether the user chose to run the 5-city or 6-city simulation, the adjacencyMatrix (a global 2D matrix containing costs to travel from city to city)
    will be set. A cell in the adjacencyMatrix can be interpreted as such: [x][y] would correspond to the cost of traveling from city x to city y. The values 
    for each of the simulations are trivial and somewhat randomly chosen. It's also important to note [x][x] is 0 for all cities, meaning the cost to travel from 
    a city to itself is 0.
*/
void setAdjacencyMatrix()
{
    if(programVariables.numberOfCitiesToVisit == 5)
    {
        std::vector<std::vector<int> > fiveCitySimulation {
        {0, 3, 4, 2, 7},    
        {3, 0, 4, 6, 3},    
        {4, 4, 0, 5, 8},    
        {2, 6, 5, 0, 6},    
        {7, 3, 8, 6, 0}     
    };

        programVariables.adjacencyMatrix = fiveCitySimulation;
    }
    else
    {
        std::vector<std::vector<int> > sixCitySimulation {
        {0, 2, 4, 1, 7, 2},   
        {3, 0, 2, 7, 3, 4},   
        {4, 9, 0, 7, 8, 2},   
        {2, 9, 5, 0, 6, 6},   
        {7, 9, 8, 7, 0, 2},   
        {3, 9, 5, 7, 2, 0},           
    };
        programVariables.adjacencyMatrix = sixCitySimulation;
    }
}

/*
    This method will calculate the lower bound for a specific node that is passed to it. This lower bound is the current cost for the node's route at the moment.
    // Explain later 
*/
void calculateLowerBoundForNode(node &nodeX)
{
    double lower_bound = 0;
    int smallest = 100;
    std::priority_queue<int> edgeCosts;
    
    for (int row = 0; row < programVariables.numberOfCitiesToVisit; row++)
    {    
        int count = 0; 

        for (int column = 0; column < programVariables.numberOfCitiesToVisit; column++)
        {       
            if(nodeX.configurationMatrix[row][column] == 1) 
            {       
                count++;
                lower_bound += programVariables.adjacencyMatrix[row][column];  
            }
            else if ((nodeX.configurationMatrix[row][column] == 0) && (row != column))
            {
                edgeCosts.push(programVariables.adjacencyMatrix[row][column]); 
            }
        }

        // Count = 0 indicates we have found no '1s' in that row. So, we need to get the two smallest edge costs in this array
        if(count == 0)
        {
            int leastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += leastCost; 
            int nextLeastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += nextLeastCost; 
        }
        // We could also have found one '1', and would need to find only the smallest element in the array
        else if(count == 1)
        {
            int leastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += leastCost;
        }
        count = 0; 

        // Empty the edgeCosts priorityQueue by swapping with an empty priority queue
        std::priority_queue<int> empty; 
        std::swap(edgeCosts, empty );
    }

    lower_bound = lower_bound / 2.0;
    nodeX.lowerBound = lower_bound;

}

// Acquire a node from the unprocessedNodesQueue
node acquireUnprocessedNode() {

        node poppedNode = programVariables.unprocessedNodesQueue.top(); 
        programVariables.unprocessedNodesQueue.pop(); 
        return poppedNode;
    
}


/*
    This dispatcher method contains the main program loop, which is carried out as follows: a node is first popped from the unprocssedNodesQueue, and 
    checked to see if it can be expanded further. If a node cannot be expanded, that node contains a route and will pushed to the global foundRoutes 
    priority queue. It will then be used to prune nodes from the unprocessedNodesQueue who contain higher lower bounds than it. 

    If a route is not found, it will be determined whether the node can include or exclude another edge in that row. For each of the conditions that evaluate
    to true, a thread will be created and carry out the process of creating another node that contains that include/excluded edge. This(these) node(s) will then
    be added back into the unprocessedNodesQueue for further expansion.

    Initially, the root node is the only node in the unprocessedNodesQueue. It gets popped, and will always spawn two threads, one for include and one for exclude, 
    that will result in two additional nodes being added to the unprocessedNodesQueue. In each iteration of the loop, a specific cell is examined to either include or 
    exclude. The location of the cell is given by the node's contrainst (a pair of ints <int><int> that give the row and column). For example, the constraint for the root
    node is <0><1>, and in other words, we are looking to see whether or not we can include/exclude the <0><1> (which represents cost of city A -> B). The constraint will
    be updated column by column for each node, meaning the root node will become<0><2> eventually, and <0><3>, <0><4>. When the last city (last column) is reached, the constraint
    will + 1 the row, meaning <0><4> becomes <1><0>. However, instead of going back to the 0 column, we will now start at <row><row + 1>, or <1><2>. This is due to the fact that 
    if we started at <1><0>, we will be including an edge that we already expanded in the first row (<0><1> edge = <1><0> edge since A -> B = B -> A). Columns will continue to progress
    like the first row (<1><2>, <1><3>, ...).

*/
void nodeExpansionDispatcher() 
{
    while (!programVariables.unprocessedNodesQueue.empty()) 
    {
        node poppedNode = acquireUnprocessedNode();
        bool routeFound = updateNodeConstraint(poppedNode);

        if (routeFound) 
        {
            programVariables.foundRoutes.push(poppedNode);
            pruneNodes();
        }
        
        checkConstraint(poppedNode);
       
        if (poppedNode.include && !programVariables.endProgram) 
        {
            std::thread includeThread(checkInclude, poppedNode, 1);
            includeThread.join(); // Join only if the thread is created
        }

        if (poppedNode.exclude && !programVariables.endProgram) 
        {
            std::thread excludeThread(checkExclude, poppedNode, 2);
            excludeThread.join(); // Join only if the thread is created
        }
    }

        node bestRoute = programVariables.foundRoutes.top();
        std::cout << "Best route obtained: " << bestRoute.lowerBound << std::endl << std::endl;
        print(bestRoute);

}


void checkInclude(node poppedNode, int id)
{
    
    if (poppedNode.include)
    {
        
        node includeNode = poppedNode;
        modifyMatrix(includeNode, true); 
        calculateLowerBoundForNode(includeNode); 
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Include Node Added" << std::endl;
        print(includeNode);
        pthread_mutex_unlock(&printMutex);
        pthread_mutex_lock(&queueMutex);
        includeNode.V.push_back(includeNode.constraint.second);
        programVariables.unprocessedNodesQueue.push(includeNode);
        pthread_mutex_unlock(&queueMutex);
                     
    }
    else
    {   
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Cannot further include. Terminating node. " << std::endl << std::endl;
        pthread_mutex_unlock(&printMutex);
    }

}
void checkExclude(node poppedNode, int id)
{
    
    if (poppedNode.exclude)
    {
        node excludeNode = poppedNode;
        modifyMatrix(excludeNode, false);
        calculateLowerBoundForNode(excludeNode);
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Exclude Node Added" << std::endl;
        print(excludeNode);
        pthread_mutex_unlock(&printMutex);  
        pthread_mutex_lock(&queueMutex);  
        excludeNode.V.push_back(excludeNode.constraint.second);
        programVariables.unprocessedNodesQueue.push(excludeNode); 
        pthread_mutex_unlock(&queueMutex);           
    }
    else
    {
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Cannot further exclude. Terminating node. " << std::endl << std::endl;
        pthread_mutex_unlock(&printMutex);
    }
}

// This function calculates the lower bound for a given node

// Prints a node
void print(node nodeX)
{
    int character = 0;
    char ch = 'A';
    std::cout << "Lowerbound : " << nodeX.lowerBound << std::endl << std::endl;
    std::cout << std::setw(2);

    for(int i = 0; i < programVariables.numberOfCitiesToVisit; i++)
    {
        character = int(ch);
        std::cout << ch << "  ";
        character++;
        ch = char(character);
    }

    std::cout << "#1" << " " << "~#1" << std::endl; 
    std::cout << "----------------------";
    std::cout << std::endl;

    for (int row = 0; row < programVariables.numberOfCitiesToVisit; row++)
    {
        for (int column = 0; column < (programVariables.numberOfCitiesToVisit + 2); column++)
        {
            std::cout << std::setw(2) << nodeX.configurationMatrix[row][column] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "**********************" << std::endl << std::endl;
    std::cout << "Constraint: " << "<" << nodeX.constraint.first << "><" << nodeX.constraint.second << ">" << std::endl << std::endl;
    
}

/*
    This method is called when a route is found, and it is used to 
*/
bool pruneNodes()
{
    int count = 0;
    node s = programVariables.foundRoutes.top(); // Gets the best node we have so far in the foundRoutes priority queue
    bool flag = false;

    // We pop until we have found the next node that is to be expanded, or until the queue is empty, meaning we got the best route
    //programVariables.unprocessedNodesQueue.pop();
    node temp;
    
    while ((programVariables.unprocessedNodesQueue.empty() == false) && (flag == false))
    {
        count++;
        temp = programVariables.unprocessedNodesQueue.top();

        if (temp.lowerBound >= s.lowerBound)
        {
            std::cout << "Node terminated. Lowerbound: " << temp.lowerBound << " > calculated Route " << std::endl << std::endl;
            programVariables.unprocessedNodesQueue.pop();
        }
        else
        {
            flag = true;
        }
    }
  
    if (count > 0)
    {
        updateNodeConstraint(temp);
    }
    if (programVariables.unprocessedNodesQueue.empty())
    {
        std::cout << "Empty queue empty queue empty queue" << std::endl << std::endl;
        programVariables.endProgram = true;
    }
  
    
    return false;
    


}

// Takes a node and a boolean. Boolean is used to determine if we are adding 1's or -1's to the matrix
void modifyMatrix(node &s, bool include)
{
    int excludeColumnTotal = programVariables.numberOfCitiesToVisit - 1;
    int nextRowExclude = programVariables.numberOfCitiesToVisit -1;
    int includeColumnTotal = 0;
    int nextRowInclude = 0;
    int row, column = 0;
    
    if(include)
    {
        row = s.constraint.first;
        column = s.constraint.second;
        s.configurationMatrix[row][column] = 1;
        row = s.constraint.second;
        column = s.constraint.first;
        s.configurationMatrix[row][column] = 1;        
    }
    else
    {
        row = s.constraint.first;
        column = s.constraint.second;
        s.configurationMatrix[row][column] = -1;
        row = s.constraint.second;
        column = s.constraint.first;
        s.configurationMatrix[row][column] = -1;
    }
    
    for (int i = 0; i < programVariables.numberOfCitiesToVisit; i++)
    {
        if (s.configurationMatrix[s.constraint.first][i] == 1)
        {
            includeColumnTotal += 1;
            excludeColumnTotal -= 1;
            
        }
        if (s.configurationMatrix[s.constraint.first + 1][i] == 1)
        {
            nextRowInclude += 1;
            nextRowExclude -= 1;
            
        }
        if (s.configurationMatrix[s.constraint.first][i] == -1)
        {
            excludeColumnTotal -= 1;
            
        }
        if (s.configurationMatrix[s.constraint.first + 1][i] == -1)
        {
            nextRowExclude -= 1;       
        }
    }
    
    s.configurationMatrix[s.constraint.first][programVariables.numberOfCitiesToVisit ] = includeColumnTotal; // Will calculate the 2nd to last column, indicating how many edges have been included
    s.configurationMatrix[s.constraint.first + 1][programVariables.numberOfCitiesToVisit] = nextRowInclude;  // Calculates next rows 2nd to last column
    s.configurationMatrix[s.constraint.first][programVariables.numberOfCitiesToVisit + 1] = excludeColumnTotal; // calculates last column 
    s.configurationMatrix[s.constraint.first + 1][programVariables.numberOfCitiesToVisit + 1] = nextRowExclude; // calculates next rows last column
}


//This function is used to determine whether we can include/exclude further for a given node

void checkConstraint(node &s)
{
    s.include = true;
    s.exclude = true;
    int row = s.constraint.first;
    
    // Can't include if 2nd to last column is 2
    if (s.configurationMatrix[row][programVariables.numberOfCitiesToVisit] == 2 || (s.configurationMatrix[row][programVariables.numberOfCitiesToVisit] == 0 && s.configurationMatrix[row][programVariables.numberOfCitiesToVisit + 1] < 4))
    {
        s.include = false;
    }

    for (int i = 0; i < s.V.size(); i++)
    {
        
        if ((s.constraint.first == s.V[i]) && (s.constraint.second == s.V[i]))
        {
            s.include = false;
            i = 123;
        }
    }

    // Can't exclude if these conditions are true. 
    if ((s.configurationMatrix[row][programVariables.numberOfCitiesToVisit] == 0) && (s.configurationMatrix[row][programVariables.numberOfCitiesToVisit + 1] == 2) || (s.configurationMatrix[row][programVariables.numberOfCitiesToVisit] == 1 && s.configurationMatrix[row][programVariables.numberOfCitiesToVisit + 1] == 1))
    {
        s.exclude = false;
    }
    if((s.configurationMatrix[row][programVariables.numberOfCitiesToVisit] == 0) && (s.configurationMatrix[row][programVariables.numberOfCitiesToVisit + 1] == 1))
    {
        
        s.exclude = false;
    }
    
}

/*
    A constraint for a node is an integer pair (<int><int>) that is used to identify when a route is eventually found for a node.
    The constraint for the root nodes starts at <0><0>. This corresponds to the top left cell of the adjacencyMatrix (the matrix that holds
    the costs from city to city). As mentioned before, a cell in this matrix [x][y] corresponds to the cost from city x to city y. The adjacencyMatrix
    is a symmetrical matrix, meaning the [x][y] cell = [y][x] cell.

    In this way, the updateConstraint method does not need to
*/
bool updateNodeConstraint(node &nodeX)
{
   
    bool foundRoute = false;
    
    // A route is found if we constraint.first is in the 2nd to last row, and we are in the last column of that row. 
    // This means we have 1 full row under the cell we are in. 
    if((nodeX.constraint.first == programVariables.numberOfCitiesToVisit - 2) && (nodeX.constraint.second == programVariables.numberOfCitiesToVisit - 1))
    {
        std::cout << "Route found. " << std::endl;
        foundRoute = true; 
        
    }
   
    // Otherwise, we continue to update constraints in the following manner:
    else
    {
        // If we are at the last column, we need to go down a row 
        if (nodeX.constraint.second == (programVariables.numberOfCitiesToVisit - 1))
        {
            nodeX.constraint.first++; // Next row: if we are at row 0, now we are at row 1
            nodeX.constraint.second = nodeX.constraint.first + 1; // Instead of starting at column 1, we start at constraint.firs

            if (nodeX.constraint.second >= programVariables.numberOfCitiesToVisit - 1)
            {
                nodeX.constraint.second = programVariables.numberOfCitiesToVisit - 1;
            }
        }
        else
        {
            nodeX.constraint.second++;
        }
        
    }
    
    return foundRoute;
}





