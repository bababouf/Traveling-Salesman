#include <iostream>
#include <queue>
#include <iomanip>
#include <vector>
#include <thread>
#include <set>

// A node encapsulates a "route" in expansion
struct node{
    std::vector<std::vector<int>> configurationMatrix; // 2D matrix containing included/excluded edges, and information (in the last 2 cells) to determine if further edges can be included/excluded
    double lowerBound = 0; // Smallest cost of all edges in the route; sums the cost of the current included edges with the least cost edges 
    std::pair<int, int> constraint; // Indexes a cell (edge) that will be examined for possible inclusion/exclusion
    bool include; // Set to true if the constraint cell can be included
    bool exclude; // Set to true if the constraint cell can be excluded
    std::vector<int> previouslyVisited; // Vector containing cities previously visited. Important for disallowing multiple cycles (TSP allows every city to only be visited once (except home city))
};

enum City { A, B, C, D, E, F, G };

int readInSimulationMode();
node initializeConfigurationMatrix();
void setAdjacencyMatrix();
void nodeExpansionDispatcher(node root);
bool updateNodeConstraint(node &s);
void checkConstraint(node &s);
void checkInclude(node poppedNode, int id);
void checkExclude(node poppedNode, int id);
void modifyMatrix(node &s, bool include);
void calculateLowerBoundForNode(node &s); 
void pruneNodesUpdated();
void print(node s);
std::string cityToString(City city);
void printBestRoute(node node);

// Compares the lower bounds of two nodes, returns true if p1.lB > p2.lB. Used as comparison function for priority queue.
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
        specifies the comparison function which determines how the nodes are stored. The comparator above is used as this function.
    */
    std::priority_queue<node, std::vector<node>, Comparator> unprocessedNodesQueue; // Contains all nodes that can still be expanded (not complete routes)
    std::vector<std::vector<int> > adjacencyMatrix; // 2D matrix where each cell is the cost between cities. Ex: [a][b] = cost from a->b, b->a
    node foundRoute; 
    bool endProgram = false; 
    int numberOfCitiesToVisit;
};

pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
ProgramVariables programVariables;

// Driver method
int main()
{
    programVariables.numberOfCitiesToVisit = readInSimulationMode(); 
    node root = initializeConfigurationMatrix(); 
    setAdjacencyMatrix();
    nodeExpansionDispatcher(root); // Starts the multithreaded route discovery process
    return 0;
   
}

/*
    This method will prompt the user to enter '5' for the 5-city simulation, '6' for the 6-city simulation, or '7' for the 7-city simulation.
    Invalid inputs (anything other than numerical '5', '6', or '7') will be rejected and prompt the user to enter a new choice.
*/
int readInSimulationMode()
{
    int input;
    bool validInput = false;

    std::cout << std::endl;
    std::cout << "The salesman is almost ready to embark on their journey. \n " 
        << "Select which simulation to run:  \n "
        << "\t For 5-city simulation, enter '5'  \n "
        << "\t For 6-city simulation, enter '6'  \n "
        << "\t For 7-city simulation, enter '7'  \n ";

    do
    {
        std::cout << "\t Choice: ";
        std::cin >> input;

        if (std::cin.fail() || input > 7 || input < 5) // Cin.fail() is true when a non-integer input is entered, due to cin >> input (input is integer)
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

    std::cout << std::endl;

    return input;
}

/*
    The configurationMatrix is a 2D matrix associated with each node. The size of the matrix depends on the numberOfCities that will be used in 
    the simulation. For example, if the user chose the 5-city simulation, the 2D matrix will be of size [5][7]. The 5 x 5 part of the matrix contains 
    either '0', '1', or '-1'. A cell will contain '1' if the edge has been included in the node's current route, and '-1' if the edge has been excluded 
    (meaning that edge is not in the route). It was stated that the size of the 5-city 2D configurationMatrix for a node is [5][7], and this is because
    two extra columns are needed to hold information to decipher whether an edge for that row can be included or excluded. The decision is made with the 
    information in each of these last two columns.

    The method below initializes the configurationMatrix for the root node. To continue with the 5-city example, this method will set the 5 x 5 cells to all '0s'. The last
    column will be set to all '4s', and the 2nd to last column will again be set to all '0s'. More information can be found in the readME file.
*/
node initializeConfigurationMatrix()
{
    node root;
    int numberOfColumns = programVariables.numberOfCitiesToVisit + 2; 
    int numberOfRows = programVariables.numberOfCitiesToVisit;
    int lastColumn = programVariables.numberOfCitiesToVisit + 1;
    std::vector<std::vector<int> > configurationMatrix(programVariables.numberOfCitiesToVisit, std::vector<int>(numberOfColumns)); // Creates 2D vector of ints, with row size = # of cities, column size = (# cities + 2)

    for(int row = 0; row < numberOfRows; row++)
        {
            for(int column = 0; column < numberOfColumns; column++)
            {
                if(column == lastColumn) // Exclude column 
                {
                    configurationMatrix[row][column] = (programVariables.numberOfCitiesToVisit - 1);
                }
                else // All other columns
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
    Depending on whether the user chose to run the 5-city, 6-city, or 7-city simulation, the adjacencyMatrix (a global 2D matrix containing costs to travel from city to city)
    will be set. A cell in the adjacencyMatrix can be interpreted as such: [x][y] would correspond to the cost of traveling from city x to city y. The values 
    for each of the simulations are trivial and randomly chosen. It's also important to note [x][x] is 0 for all cities, meaning the cost to travel from 
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

    else if(programVariables.numberOfCitiesToVisit == 6)
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
    else
    {
        std::vector<std::vector<int> > sevenCitySimulation {
        {0, 6, 8, 2, 6, 1, 9},   
        {6, 0, 5, 4, 1, 9, 2},   
        {8, 5, 0, 6, 1, 1, 8},   
        {2, 4, 6, 0, 2, 9, 3},   
        {6, 1, 1, 2, 0, 2, 9},   
        {1, 9, 1, 9, 2, 0, 7},
        {9, 2, 8, 3, 9, 7, 0}, 
    };
    programVariables.adjacencyMatrix = sevenCitySimulation;
    }
}

/*
    This dispatcher method contains the main program loop, which is carried out as follows: a node is first popped from the unprocssedNodesQueue, and 
    is checked to see if it can be expanded further. If a node cannot be expanded, that node contains a route and  It will then be used to prune nodes 
    from the unprocessedNodesQueue who contain higher lower bounds than it. 

    If a route is not found, it is determined whether the node can include or exclude another edge in that row. For each of the conditions that evaluate
    to true, a thread will be created and carry out the process of creating another node that contains that include/excluded edge. This(these) node(s) will then
    be added back into the unprocessedNodesQueue for further expansion.
*/
void nodeExpansionDispatcher(node root) 
{
    programVariables.unprocessedNodesQueue.push(root);

    while (!programVariables.unprocessedNodesQueue.empty()) // Program loop starts
    {
        node poppedNode = programVariables.unprocessedNodesQueue.top(); 
        programVariables.unprocessedNodesQueue.pop();
        bool routeFound = updateNodeConstraint(poppedNode); // Initially the constraint for the root node is <0><0>; this updates it to <0><1>. For more details on how the update is carried out, check updateNodeConstraint()

        if (routeFound) 
        {
            programVariables.foundRoute = poppedNode;
            pruneNodesUpdated();
        }
        
        checkConstraint(poppedNode); // Sets each of poppedNode.include and poppedNode.exclude depending on whether or not inclusion/exclusion is possible
       
        if (!programVariables.endProgram) 
        {
            std::thread includeThread(checkInclude, poppedNode, 1); // Thread created to check if edge can be included
            includeThread.join(); 
        }
        if (!programVariables.endProgram) 
        {
            std::thread excludeThread(checkExclude, poppedNode, 2); // Thread created to check if edge can be excluded
            excludeThread.join(); 
        } 
    }

        std::cout << "Best route obtained: " << programVariables.foundRoute.lowerBound << std::endl << std::endl;
        print(programVariables.foundRoute); 
        printBestRoute(programVariables.foundRoute);

}

/*
    The constraint for a node is updated column by column until the end of the row is reached. Upon reaching the end of a row, the row is incremented and the column is 
    set to the row + 1. Using the 5-city simulation as an example, the constraint would start at <0><0> and would be incremented column by column until it reached <0><4>. 
    The next update would set the constraint to <1><2>, and consecutive updates would go back to column by column until the end of the row was reached again. 

    This method is also important in that it returns true if a route is found. A route is found when the constraint can no longer be updated. In the 5-city simulation,
    this would be at <3><4>, for if we tried to update the constraint, we would have <4><5> and fall off the matrix. 
*/
bool updateNodeConstraint(node &nodeX)
{
    bool foundRoute = false;
    int currentRow = nodeX.constraint.first;
    int currentColumn = nodeX.constraint.second;
    int lastCityIndex = programVariables.numberOfCitiesToVisit - 1; 
    int secondToLastRow = programVariables.numberOfCitiesToVisit - 2;

    if((currentRow == secondToLastRow) && (currentColumn == lastCityIndex)) // Constraint cannot be updated further; a route has been found
    {
        std::cout << "Route found. " << std::endl;
        foundRoute = true; 
    }
    else
    {
        if (currentColumn == lastCityIndex) // If the last index is reached, increment the row and set the column to the row + 1
        {
            ++currentRow;
            currentColumn = currentRow + 1;
        }
        else // Most common, simply increment the column
        {
            ++currentColumn;
        }
        
    }

    // Set constraint for node to updated values
    nodeX.constraint.first = currentRow; 
    nodeX.constraint.second = currentColumn;
    
    return foundRoute;
}

/*
    Each node contains boolean variables to indicate whether the current edge given by the constraint can be included or excluded. This method will 
    set those boolean variables by examining the last two columns of the configurationMatrix. The second to last column will give the number of edges
    that have been included for each row, and last column will give the total number of edges that can still be included or excluded. When the configurationMatrix
    for the root node is first initialized, the last column is set to numberOfCitiesToVisit - 1. To use the 5-city simulation as an example, the last column
    will be initialized to 4, meaning 4 edges in each row can be included or excluded. 

    When an edge is included, the include column is incremented, and the exclude column is decremented. When an edge is excluded, only the exclude column is decremented.
*/
void checkConstraint(node &nodeX)
{
    nodeX.include = false;;
    nodeX.exclude = true;;
    int currentRow = nodeX.constraint.first;
    int includeColumn = programVariables.numberOfCitiesToVisit; // # of edges included in a row
    int excludeColumn = programVariables.numberOfCitiesToVisit + 1; // # of edges that can be included/excluded in a row

    if(nodeX.configurationMatrix[currentRow][includeColumn] < 2) // For an edge to be included, it cannot have 2 edges already included in that row
    {
        if(nodeX.configurationMatrix[currentRow][includeColumn] == 0 && nodeX.configurationMatrix[currentRow][excludeColumn] >= 2) // If no edges have been included, and there is still 2 available edges to include/exclude (given by the exclude column), the edge is safe to include
        {
            nodeX.include = true;
        }
        else if(nodeX.configurationMatrix[currentRow][includeColumn] == 1 && nodeX.configurationMatrix[currentRow][excludeColumn] >= 1) // If 1 edge has been included, and there is at least 1 available edge to include/exclude (given by the exclude column), the edge is safe to include
        {
            nodeX.include = true;
        }
    }

    // Check if the city (constraint.first) has been included previously. If this is the case, we need to make sure we are not visiting a city that we have also been to (would create a cycle)
    bool previouslyIncluded = false;

    for(int i = 0; i < nodeX.previouslyVisited.size(); i++)
    {
        if(nodeX.constraint.first == nodeX.previouslyVisited[i])
        {
            previouslyIncluded = true;
        }
    }

    
    if(previouslyIncluded)
    {
        // Check that we are not including a city we have already visited 
        for(int j = 0; j < nodeX.previouslyVisited.size(); j++)
        {
            if(nodeX.constraint.second == nodeX.previouslyVisited[j])
            {
                nodeX.include = false;
            }
        }
    }

    // These selection statements determine whether an edge can be excluded. There are two cases where we cannot exclude:
    if(nodeX.configurationMatrix[currentRow][excludeColumn] == 2 && nodeX.configurationMatrix[currentRow][includeColumn] == 0) // If the exclude column has 2 edges that can be included/excluded AND we have currently 0 included edges
    {
        nodeX.exclude = false; 
    }

    // If 1 edge can be included/excluded AND we either have 1 edge left to include, or 0 edges left to include, we CANNOT exclude
    if(nodeX.configurationMatrix[currentRow][excludeColumn] == 1 && (nodeX.configurationMatrix[currentRow][includeColumn] == 1 || nodeX.configurationMatrix[currentRow][includeColumn] == 0 )) 
    {
        nodeX.exclude = false;
    }
}

/*
    This method is called after it is determined that the current cell edge (given by the constraint) can be included. In order
    to ensure no race conditions when printing to console, a mutex is used around the cout stream.
*/
void checkInclude(node nodeX, int id)
{
    
    if (nodeX.include)
    {
        modifyMatrix(nodeX, true); // Adds the edge that will be included to the appropriate configurationMatrix cell
        calculateLowerBoundForNode(nodeX); // Calculates new lower bound with consideration for included edge

        pthread_mutex_lock(&printMutex);
        std::cout << "* * * * * * * * * * * * * * *" << std::endl << std::endl;
        std::cout << "Thread " << id << " executing..." << std::endl;
        std::cout << "Including edges at " << "[" << nodeX.constraint.first << "][" << nodeX.constraint.second << "]"<< " & ["<< nodeX.constraint.second << "][" << nodeX.constraint.first << "]"<<  std::endl;
        print(nodeX);
        pthread_mutex_unlock(&printMutex);

        nodeX.previouslyVisited.push_back(nodeX.constraint.second); // A node carries along a vector of cities that have been visited. This is important for disallowing cycles, and is checked in checkConstraint()
        programVariables.unprocessedNodesQueue.push(nodeX); // The node, now having an updated lowerbound and constraint, is pushed back into the unprocessedNodesQueue            
    }
    else
    {   
        pthread_mutex_lock(&printMutex);
        std::cout << "* * * * * * * * * * * * * * *" << std::endl << std::endl;
        std::cout << "Thread " << id << " executing..." << std::endl;
        std::cout << "Cannot further include. Terminating node. " << std::endl << std::endl;
        pthread_mutex_unlock(&printMutex);
    }

}

/*
    This method is called after it is determined that the current cell edge (given by the constraint) can be included. 
*/
void checkExclude(node nodeX, int id)
{
    
    if (nodeX.exclude)
    {
        modifyMatrix(nodeX, false); // Adds the edge that will be excluded to the appropriate configurationMatrix cell
        calculateLowerBoundForNode(nodeX); // Calculates new lower bound with consideration for excluded edge

        pthread_mutex_lock(&printMutex);
        std::cout << "* * * * * * * * * * * * * * *" << std::endl << std::endl;
        std::cout << "Thread " << id << " executing..." << std::endl;
        std::cout << "Excluding edges at " << "[" << nodeX.constraint.first << "][" << nodeX.constraint.second << "]"<< " & ["<< nodeX.constraint.second << "][" << nodeX.constraint.first << "]"<<  std::endl;
        print(nodeX);
        pthread_mutex_unlock(&printMutex);  
        
        programVariables.unprocessedNodesQueue.push(nodeX);        
    }
    else
    {
        pthread_mutex_lock(&printMutex);
        std::cout << "* * * * * * * * * * * * * * *" << std::endl << std::endl;
        std::cout << "Thread " << id << " executing" << std::endl;
        std::cout << "Cannot further exclude. Terminating node. " << std::endl << std::endl;
        pthread_mutex_unlock(&printMutex);
    }
}

/*
    Each time an edge is included or excluded, this method is called to update the configurationMatrix for that node. 
    If an edge is included, two cells are modified with '1s' (X -> Y and Y -> X, given by the constraint). The same is true
    for excluded edges, except two cells are modified with '-1s'. 

    In addition, each modification to the matrix must updated the last two columns (include and exclude columns). If an edge
    is added, the include column is incremented, and the exclude column decremented. Again, since we are modifying two cells in two
    different rows, these columns must be updated for each row. If an edge is excluded, the exclude column only need to be decremented
    in each of the rows.
*/
void modifyMatrix(node &nodeX, bool include)
{
    int currentRow = nodeX.constraint.first;
    int currentColumn = nodeX.constraint.second;
    int excludeColumn = programVariables.numberOfCitiesToVisit + 1;
    int includeColumn = programVariables.numberOfCitiesToVisit;

    if(include)
    {
        nodeX.configurationMatrix[currentRow][currentColumn] = 1; // Sets current cell to 1
        nodeX.configurationMatrix[currentColumn][currentRow] = 1; // Sets the symmetrical cell to 1

        nodeX.configurationMatrix[currentRow][includeColumn] += 1; // Increments includeColumn
        nodeX.configurationMatrix[currentRow][excludeColumn] -= 1; // Decrements excludeColumn 
        nodeX.configurationMatrix[currentColumn][includeColumn] += 1; // Same for symmetrical cell
        nodeX.configurationMatrix[currentColumn][excludeColumn] -= 1; // Same for symmetrical cell
        
    }
    else
    {
        nodeX.configurationMatrix[currentRow][currentColumn] = -1; // Set current cell to -1
        nodeX.configurationMatrix[currentColumn][currentRow] = -1; // Set symmetrical cell to -1

        nodeX.configurationMatrix[currentRow][excludeColumn] -= 1; // Decrement excludeColumn
        nodeX.configurationMatrix[currentColumn][excludeColumn] -= 1; // Same for symmetrical cell

    }

}

/*
    This method calculates the lowerbound for a given node, which is the smallest possible value that may or may not create a 
    valid route. The lowerbound is calculated by considering the edges that have been included and excluded thus far. For each row,
    the costs for the included edges will be taken and accumulated in the lowerbound variable. 

    Each row can have at most two edges included; the traveling salesman problem specifies that a city can only be visited once, meaning 
    each city can have at most two outgoing edges. Since a row represents the costs from one city to the rest, each row can only 
    include two edges.

    When calculating the lowerbound for a node, the first edges that are summed are those that have been included. This is done for all rows
    in each node's configurationMatrix; for example, the root node would start out with no edges included, so the lowerbound will be calculated
    by taking two of the least cost edges from every row and summing them. This lowerbound, however, may not create a valid route, but it is used
    as a heuristic to better predict which nodes to expand next. To illustrate how a valid route may not be created here is an example situation assuming we
    are expanding the root node which has no edges included. 

    In the first row, A -> B and A -> C were included as they were the least cost edges. In the next row, B -> C and B -> D were taken. This has now caused
    a cycle to be created, formed by A -> B, A -> C, and B -> C; therefore, this route would be invalid. 

*/
void calculateLowerBoundForNode(node &nodeX)
{
    double lower_bound = 0;
    int smallest = 100;
    std::priority_queue<int> edgeCosts;
    
    for (int row = 0; row < programVariables.numberOfCitiesToVisit; row++) // All rows in the configurationMatrix are visited
    {    
        int count = 0; // Keeps track of the # of included edges in each row; resets for each row

        for (int column = 0; column < programVariables.numberOfCitiesToVisit; column++) // All columns in the configurationMatrix are visited
        {       
            if(nodeX.configurationMatrix[row][column] == 1) // Signifies an edge has been included
            {       
                count++; 
                lower_bound += programVariables.adjacencyMatrix[row][column]; // Add the cost of the included edge to lowerbound running total
            }
            else if ((nodeX.configurationMatrix[row][column] == 0) && (row != column)) // This ensures cells that contain -1 are not considered, as well as diagonal cells (A->A, B->B, C->C, etc)
            {
                edgeCosts.push(programVariables.adjacencyMatrix[row][column]); 
            }
        }

        /*
            After visiting all the cells in a row, count will either be 0, 1, or 2, depending on the number of included edges.
        */
        if(count == 0) // If count == 0, no edges have been included and we need to take the two least cost edges and add them to the lowerbound total
        {
            int leastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += leastCost; 
            int nextLeastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += nextLeastCost; 
        }
        // We could also have found one '1', and would need to add the least cost edge
        else if(count == 1)
        {
            int leastCost = edgeCosts.top();
            edgeCosts.pop();
            lower_bound += leastCost;
        }

        // If count is 2, we already have the lowerbound added.

        count = 0; 

        // Empty the edgeCosts priorityQueue by swapping with an empty priority queue
        std::priority_queue<int> empty; 
        std::swap(edgeCosts, empty );
    }

    lower_bound = lower_bound / 2.0; // Divide by 2 since X -> Y and Y -> X have been included in lowerbound calculation 
    nodeX.lowerBound = lower_bound;

}

/*
    After a route is found, this method is called to ensure there are no other nodes that have yet to be expanded that have a smaller
    lowerbound than the node which contains the foundRoute. 
*/
void pruneNodesUpdated()
{
   
    while ((!programVariables.unprocessedNodesQueue.empty()))
    {
        node unprocessedNode = programVariables.unprocessedNodesQueue.top(); // Each call to unprocessedNodesQueue.top will be the node with the lowest lowerBound

        if (unprocessedNode.lowerBound >= programVariables.foundRoute.lowerBound)
        {
            std::cout << "Node terminated. Lowerbound: " << unprocessedNode.lowerBound << " > calculated Route " << std::endl << std::endl;
            programVariables.unprocessedNodesQueue.pop();
        }
        else
        {
            break;
        }
       
    }
  
    if (programVariables.unprocessedNodesQueue.empty())
    {
        std::cout << "The unprocessed nodes queue is empty, printing best route and ending program..." << std::endl << std::endl;
        programVariables.endProgram = true;
    }
  

}

/*
    Prints the configurationMatrix for a node to the console.
*/
void print(node nodeX)
{
    int character = 0;
    char ch = 'A';
    std::cout << "Lowerbound : " << nodeX.lowerBound << std::endl;
    std::cout << "Configuration Matrix: " << std::endl << std::endl;
    std::cout << std::setw(2);

    // For each column, the alphabetical character that represents the city is printed
    for(int i = 0; i < programVariables.numberOfCitiesToVisit; i++)
    {
        character = int(ch);
        std::cout << ch << "  ";
        character++;
        ch = char(character);
    }

    std::cout << "#1" << " " << "~#1" << std::endl; 
    
    // Prints the values for each cell in the configurationMatrix
    for (int row = 0; row < programVariables.numberOfCitiesToVisit; row++)
    {
        for (int column = 0; column < (programVariables.numberOfCitiesToVisit + 2); column++)
        {
            std::cout << std::setw(2) << nodeX.configurationMatrix[row][column] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Constraint: " << "<" << nodeX.constraint.first << "><" << nodeX.constraint.second << ">" << std::endl << std::endl;
    
}

/*
    City is an enumeration; this method sets City to a string
*/
std::string cityToString(City city) {
    switch (city) {
        case A: return "A";
        case B: return "B";
        case C: return "C";
        case D: return "D";
        case E: return "E";
        case F: return "F";
        case G: return "G";
        default: return "";
    }
}

/*
    This method will begin at the starting city A, and print the lowest cost route city by city
*/
void printBestRoute(node nodeX)
{
    int numberOfCities = programVariables.numberOfCitiesToVisit;

    std::vector<City> route;
    std::set<City> visited;
    City currentCity = A; // Start from city A

    // While loop makes sure each city is only visited once
    while (visited.size() < numberOfCities) {
        route.push_back(currentCity);
        visited.insert(currentCity);

        for (int j = 0; j < numberOfCities; ++j) {
            // If the edge has been include in the current column, and we have yet to visit the city
            if (nodeX.configurationMatrix[currentCity][j] == 1 && visited.find(static_cast<City>(j)) == visited.end()) // visited.find returns visited.end() if not found (meaning city hasnt been visited)
            {
                currentCity = static_cast<City>(j); // Cast j back to city
                break;
            }
        }
    }

    // Return to the starting city
    route.push_back(A);

    // Print the route
    for (int i = 0; i < route.size() - 1; i++) {
        std::cout << cityToString(route[i]) << " -> ";
    }
    std::cout << cityToString(route.back()) << std::endl;
}






