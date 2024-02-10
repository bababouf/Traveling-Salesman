#include <iostream>
#include <queue>
#include <iomanip>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "node.h"
#include <limits>

  
int readInSimulationMode();
node setConfigurationMatrix(int citiesToVisit);
void setAdjacencyMatrix(int citiesToVisit);
void calculateLowerBoundForNode(node &s, int citiesToVisit); 
void* processNode(void *arg);
node acquireUnprocessedNode(int citiesToVisit);
void checkInclude(node poppedNode, int id, int citiesToVisit);
void checkExclude(node poppedNode, int id, int citiesToVisit);
void checkConstraint(node &s, int citiesToVisit);
bool pruneNodes(node &s, int citiesTo);
bool updateNodeConstraint(node &s, int citiesToVisit);
void calculateLowerBoundForNode(node &s, int citiesToVisit);  
void modifyMatrix(node &s, bool include, int citiesToVisit);
void print(node s, int citiesToVisit);



struct ProgramVariables {
    bool foundRoute = false;
    bool endProgram = false; 

    /*
        Priority_Queue is a container adapter in the C++ standard library. The first parameter "node" specifies the type being stored.
        The second parameter vector<node> specifies how to store that type, which in this case is a vector of nodes. The third parameter 
        specifies the comparison function which determines how the nodes are stored. 
    */
    std::priority_queue<node, std::vector<node>, Comparator> foundRoutes; 
    std::priority_queue<node, std::vector<node>, Comparator> unprocessedNodesQueue;
    std::vector<std::vector<int> > adjacencyMatrix;
};

struct ThreadArgs {
    int threadID;
    int numberOfCitiesToVisit;
};

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t isEmpty = PTHREAD_COND_INITIALIZER;

ProgramVariables programVars;

int main()
{
    
    int numberOfCitiesToVisit = readInSimulationMode();
    node root = setConfigurationMatrix(numberOfCitiesToVisit); 
    setAdjacencyMatrix(numberOfCitiesToVisit);
    calculateLowerBoundForNode(root, numberOfCitiesToVisit);
    programVars.unprocessedNodesQueue.push(root);


    struct ThreadArgs t1args = {1, numberOfCitiesToVisit};
    struct ThreadArgs t2args = {2, numberOfCitiesToVisit};
    struct ThreadArgs t3args = {3, numberOfCitiesToVisit};
    struct ThreadArgs t4args = {4, numberOfCitiesToVisit};

    pthread_t t1, t2, t3, t4;

    if(numberOfCitiesToVisit == 5)
    {
        pthread_create(&t1, NULL, processNode, &t1args);
        pthread_create(&t2, NULL, processNode, &t2args);
    }
    else
    {
        pthread_create(&t1, NULL, processNode, &t1args);
        pthread_create(&t2, NULL, processNode, &t2args);
        pthread_create(&t3, NULL, processNode, &t3args);
        pthread_create(&t4, NULL, processNode, &t4args);
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&isEmpty);

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
    The configurationMatrix is a 2D matrix associated with each node. //Explain more later
*/
node setConfigurationMatrix(int numberOfCitiesToVisit)
{
    node root;
    int numberOfColumns = numberOfCitiesToVisit + 2;
    int lastColumn = numberOfCitiesToVisit + 1;
    std::vector<std::vector<int> > configurationMatrix(numberOfCitiesToVisit, std::vector<int>(numberOfColumns)); // Creates 2D vector of ints, with row size = # of cities, column size = (# cities + 2)

    for(int row = 0; row < numberOfCitiesToVisit; row++)
        {
            for(int column = 0; column < (numberOfColumns); column++)
            {
                if(column == lastColumn) 
                {
                    configurationMatrix[row][column] = (numberOfCitiesToVisit - 1);
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
void setAdjacencyMatrix(int numberOfCitiesToVisit)
{
    if(numberOfCitiesToVisit == 5)
    {
        std::vector<std::vector<int> > fiveCitySimulation {
        {0, 3, 4, 2, 7},    
        {3, 0, 4, 6, 3},    
        {4, 4, 0, 5, 8},    
        {2, 6, 5, 0, 6},    
        {7, 3, 8, 6, 0}     
    };

        programVars.adjacencyMatrix = fiveCitySimulation;
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
        programVars.adjacencyMatrix = sixCitySimulation;
    }
}

/*
    This method will calculate the lower bound for a specific node that is passed to it. Calculating the lower bound for a node is important for the pruning process; once a 
    lowerbound is found for a node, all other nodes which have lower bounds higher than this can be pruned. 
*/
void calculateLowerBoundForNode(node &nodeX, int numberOfCitiesToVisit)
{
    double lower_bound = 0;
    int smallest = 100;
    std::priority_queue<int> edgeCosts;
    
    /* This nested for loop will go through each row, column by column. For each of the rows, a count will be calculated. The count corresponds to how many '1s' were
    found in the cells for that row in the configurationMatrix. The lowerbound value is a running total (accumulated over all the rows in the matrix), and is calculated by
    adding up each cell cost in the adjacencyMatrix. 
    */
    for (int row = 0; row < numberOfCitiesToVisit; row++)
    {    
        int count = 0; // Count is initialized to zero for each row

        for (int column = 0; column < numberOfCitiesToVisit; column++)
        {       
            if(nodeX.configurationMatrix[row][column] == 1) // Count and lowerbound values are only updated when a '1' is found in the configurationMatrix
            {       
                count++;
                lower_bound += programVars.adjacencyMatrix[row][column]; // Add the cost of the adjacencyMatrix cell to the running lower_bound total   
            }
            else if ((nodeX.configurationMatrix[row][column] == 0) && (row != column))
            {
                
                edgeCosts.push(programVars.adjacencyMatrix[row][column]); // Add edge cost to a priority queue
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
        
        count = 0; // Reset count for calculating next row

        // Empty the edgeCosts priorityQueue by swapping with an empty priority queue
        std::priority_queue<int> empty; 
        std::swap(edgeCosts, empty ); 
    }

    lower_bound = lower_bound / 2.0;
    nodeX.lowerBound = lower_bound;
}

/*
    This method allows for threads to acquire a node from the unprocessedNodesQueue. In order to avoid a race condition between the threads, a mutex
    must first be acquired by the thread before a node is acquired. If the queue is empty, nodes will be queued on the conditional variable empty and 
    relinquish the mutex. When more nodes are pushed into the queue, the first node waiting in the queue will be signaled to continue on.
*/
node acquireUnprocessedNode() {

    node poppedNode;
    pthread_mutex_lock(&queueMutex); // Threads must acquire mutex

    while(programVars.unprocessedNodesQueue.empty())
    {
        pthread_cond_wait(&isEmpty, &queueMutex); // While the unprocessedNodesQueue is empty, threads will be queued to the conditional variable and wait for a signal
                                            
    }

    // Threads that make it to this point can guarantee that there is a node to pop from the unprocessedNodesQueue 
    poppedNode = programVars.unprocessedNodesQueue.top(); 
    programVars.unprocessedNodesQueue.pop(); 
    pthread_mutex_unlock(&queueMutex); // In the event a thread did not wait on the conditional variable and relinquish the mutex, it is relinquished here
    return poppedNode;
}

/*
    The driver program will spawn threads (either 2 for the 5-city simulation or 4 for the 6-city simulation) that will each run the processNode method below. 
    The main flow for this method goes like this: threads attempt to pop a node from the unprocessedNodesQueue. If they are successful, 
*/
void* processNode(void * arg)
{
    ThreadArgs* threadArgs = (ThreadArgs*)arg; // This casts the void* arg pointer to a pointer to a struct (the ThreadsArg struct)
    int threadID = threadArgs->threadID; // Dereferences the threadArgs pointer to access threadID member
    int numberOfCitiesToVisit = threadArgs->numberOfCitiesToVisit; // Dereferences the threadArgs pointer to access numberOfCitiesToVisit member

    while (true)
    {
        node poppedNode = acquireUnprocessedNode();   
        bool routeFound = updateNodeConstraint(poppedNode, numberOfCitiesToVisit);

        pthread_mutex_lock(&queueMutex);
        if(routeFound) 
        {
            programVars.foundRoutes.push(poppedNode);
            pruneNodes(poppedNode, numberOfCitiesToVisit); 
        }

        pthread_mutex_unlock(&queueMutex);

        if (programVars.endProgram )
        {
            pthread_exit(NULL);
        }
        else 
        {
            
            checkConstraint(poppedNode, numberOfCitiesToVisit);
            checkInclude(poppedNode, threadID, numberOfCitiesToVisit);
            checkExclude(poppedNode, threadID, numberOfCitiesToVisit);
        }
    }      
        
}


void checkInclude(node poppedNode, int id, int numberOfCitiesToVisit)
{
    if (poppedNode.include)
    {
        node includeNode = poppedNode;
        modifyMatrix(includeNode, true, numberOfCitiesToVisit);                            
        calculateLowerBoundForNode(includeNode, numberOfCitiesToVisit); 
        pthread_mutex_lock(&queueMutex); 
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Include Node Added" << std::endl;
        print(includeNode, numberOfCitiesToVisit);
        pthread_mutex_unlock(&printMutex);
        includeNode.V.push_back(includeNode.constraint.second);
        programVars.unprocessedNodesQueue.push(includeNode);
        pthread_cond_signal(&isEmpty); 
        pthread_mutex_unlock(&queueMutex);
        usleep(10); 
                        
    }
    else
    {   
        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Cannot further include. Terminating node. " << std::endl << std::endl;
        pthread_mutex_unlock(&printMutex);
    }

}
void checkExclude(node poppedNode, int id, int numberOfCitiesToVisit)
{
    if (poppedNode.exclude)
    {
        node excludeNode = poppedNode;
        modifyMatrix(excludeNode, false, numberOfCitiesToVisit);
        calculateLowerBoundForNode(excludeNode, numberOfCitiesToVisit);
        pthread_mutex_lock(&queueMutex);

        pthread_mutex_lock(&printMutex);
        std::cout << "Thread " << id << " executing" << std::endl << std::endl;
        std::cout << "Exclude Node Added" << std::endl;
        print(excludeNode, numberOfCitiesToVisit);
        pthread_mutex_unlock(&printMutex);
          
        excludeNode.V.push_back(excludeNode.constraint.second);
        programVars.unprocessedNodesQueue.push(excludeNode);
        pthread_cond_signal(&isEmpty);
        pthread_mutex_unlock(&queueMutex);
        usleep(10);
                   
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
void print(node nodeX, int numberOfCitiesToVisit)
{
    
    int character = 0;
    char ch = 'A';
    
    std::cout << "Lowerbound : " << nodeX.lowerBound << std::endl << std::endl;
    std::cout << std::setw(2);
    for(int i = 0; i < numberOfCitiesToVisit; i++)
    {
        character = int(ch);
        std::cout << ch << "  ";
        character++;
        ch = char(character);
    }
    std::cout << "#1" << " " << "~#1" << std::endl; 
    std::cout << "----------------------";
    std::cout << std::endl;

    for (int row = 0; row < numberOfCitiesToVisit; row++)
    {
        for (int column = 0; column < (numberOfCitiesToVisit + 2); column++)
        {
            std::cout << std::setw(2) << nodeX.configurationMatrix[row][column] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "**********************" << std::endl << std::endl;
    
}

/*
    This method is called when a route is found, and it is used to 
*/
bool pruneNodes(node &temp, int numberOfCitiesToVisit)
{
    int count = 0;
    node s = programVars.foundRoutes.top(); // Gets the best node we have so far in the foundRoutes priority queue
    bool flag = false;
    //s = routes.top();
    // We pop until we have found the next node that is to be expanded, or until the queue is empty, meaning we got the best route
    programVars.unprocessedNodesQueue.pop();
    while ((programVars.unprocessedNodesQueue.empty() == false) && (flag == false))
    {
        count++;
        temp = programVars.unprocessedNodesQueue.top();

        if (temp.lowerBound >= s.lowerBound)
        {
            std::cout << "Node terminated. Lowerbound: " << temp.lowerBound << " > calculated Route " << std::endl << std::endl;
            programVars.unprocessedNodesQueue.pop();
        }
        else
        {
            flag = true;
        }
    }
    // If empty, we have found the best route
    if (programVars.unprocessedNodesQueue.empty())
    {
            node bestRoute = programVars.foundRoutes.top();
            std::cout << "Best route obtained: " << bestRoute.lowerBound << std::endl << std::endl;
            print(bestRoute,numberOfCitiesToVisit);
            
        
        
        // call printRoute blah blah blah
        //programVars.endProgram = true;
        return true;
    }
    else if (count > 0)
    {
        updateNodeConstraint(temp, numberOfCitiesToVisit);
    }
    return false;
    


}

// Takes a node and a boolean. Boolean is used to determine if we are adding 1's or -1's to the matrix
void modifyMatrix(node &s, bool include, int numberOfCitiesToVisit)
{
    int excludeColumnTotal = numberOfCitiesToVisit - 1;
    int nextRowExclude = numberOfCitiesToVisit -1;
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
    
    for (int i = 0; i < numberOfCitiesToVisit; i++)
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
    
    s.configurationMatrix[s.constraint.first][numberOfCitiesToVisit ] = includeColumnTotal; // Will calculate the 2nd to last column, indicating how many edges have been included
    s.configurationMatrix[s.constraint.first + 1][numberOfCitiesToVisit] = nextRowInclude;  // Calculates next rows 2nd to last column
    s.configurationMatrix[s.constraint.first][numberOfCitiesToVisit + 1] = excludeColumnTotal; // calculates last column 
    s.configurationMatrix[s.constraint.first + 1][numberOfCitiesToVisit + 1] = nextRowExclude; // calculates next rows last column
}


//This function is used to determine whether we can include/exclude further for a given node

void checkConstraint(node &s, int numberOfCitiesToVisit)
{
    s.include = true;
    s.exclude = true;
    int row = s.constraint.first;
    
    // Can't include if 2nd to last column is 2
    if (s.configurationMatrix[row][numberOfCitiesToVisit] == 2)
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
    if ((s.configurationMatrix[row][numberOfCitiesToVisit] == 0) && (s.configurationMatrix[row][numberOfCitiesToVisit + 1] == 2) || (s.configurationMatrix[row][numberOfCitiesToVisit] == 1 && s.configurationMatrix[row][numberOfCitiesToVisit + 1] == 1))
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
bool updateNodeConstraint(node &nodeX, int numberOfCitiesToVisit)
{
   
    bool foundRoute = false;
    
    // A route is found if we constraint.first is in the 2nd to last row, and we are in the last column of that row. 
    // This means we have 1 full row under the cell we are in. 
    if((nodeX.constraint.first == numberOfCitiesToVisit - 2) && (nodeX.constraint.second == numberOfCitiesToVisit - 1))
    {
        std::cout << "Route found. " << std::endl;
        foundRoute = true; 
        
    }
   
    // Otherwise, we continue to update constraints in the following manner:
    else
    {
        // If we are at the last column, we need to go down a row 
        if (nodeX.constraint.second == (numberOfCitiesToVisit - 1))
        {
            nodeX.constraint.first++; // Next row: if we are at row 0, now we are at row 1
            nodeX.constraint.second = nodeX.constraint.first + 1; // Instead of starting at column 1, we start at constraint.firs

            if (nodeX.constraint.second > numberOfCitiesToVisit - 1)
            {
                nodeX.constraint.second = numberOfCitiesToVisit - 1;
            }
        }
        else
        {
            nodeX.constraint.second++;
        }
        
    }
    
    return foundRoute;
}





