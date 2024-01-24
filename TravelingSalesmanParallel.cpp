#include <iostream>
#include <queue>
#include <iomanip>
#include <pthread.h>
#include <unistd.h>


struct node{
    std::vector<std::vector<int> > CM; 
    double lowerBound;
    std::pair<int, int> constraint;
    std::vector<int> V;
    bool include = true;
    bool exclude = true;
};

struct Comparator {
    bool operator()(node const& p1, node const& p2)
    {        
        return p1.lowerBound > p2.lowerBound;
    }
};

bool foundRoute = false;
bool endProgram = false; 
bool printNodes = false; 
bool threadExiting = false;
int cities = 0;

pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
std::priority_queue<node, std::vector<node>, Comparator> Q1;
std::priority_queue<node, std::vector<node>, Comparator> foundRoutes;   

node setup();
void* treeExpansion(void *arg);
void checkConstraint(node &s);
bool pruneNodes(node &s, std::priority_queue<node, std::vector<node>, Comparator> Q1);
bool updateConstraint(node &s);
void calculateAndCreate(node &s);  
void modifyMatrix(node &s, bool include);
void print(node s);

/*  In the main function, I first call the setup function which returns the root node. I then call calculateAndCreate to 
    get the lowerbound for the root. 

    I created four threads; two will be used for simulation one (using the 5 city example we went over in class), and the other
    two are used for the second simulation (in which I used 6 cities).
*/
int main()
{
    node root = setup();
    calculateAndCreate(root);
    Q1.push(root);
    pthread_t T1, T2, T3, T4;
    int l = 1, m = 2, n = 3, o = 4;
    if(cities == 5)
    {
        pthread_create(&T1, NULL, treeExpansion, &l);
        pthread_create(&T2, NULL, treeExpansion, &m);
        
    }
    else
    {
        pthread_create(&T1, NULL, treeExpansion, &l);
        pthread_create(&T2, NULL, treeExpansion, &m);
        pthread_create(&T3, NULL, treeExpansion, &n);
        pthread_create(&T4, NULL, treeExpansion, &o);
    }

    // The main waits for all threads to finish execution, then proceeds to destroy the mutex and condition variables that were created
   pthread_join(T1, NULL);
   pthread_join(T2, NULL);
   pthread_join(T3, NULL);
   pthread_join(T4, NULL);
   pthread_mutex_destroy(&mLock);
   pthread_cond_destroy(&empty);

   return 0;
}

/* This function first prompts user to enter which simulation they would like to run. Then, it sets the configuration Matrix for the root node, which
    sets all zeros except for last column, which is set to (cities - 1).

*/
node setup()
{
    node root;
    int input; 
    // Prompt user for which simulation to run
        do
        {
            std::cout << "Enter 1 for 5 city simulation (with verbose output), enter 2 for 6 city simulation: ";
            std::cin >> input;
        } while (input != 1 && input != 2);

        if (input == 1)
        {
            cities = 5;
            printNodes = true;
        }
        else
        {
            cities = 6;
        }
        std::cout << std::endl;
        std::vector<std::vector<int> > configurationMatrix(cities, std::vector<int>(cities + 2));

    for(int j = 0; j < cities; j++)
        {
            for(int k = 0; k < (cities + 2); k++)
            {
                if(k == (cities + 1))
                {
                    configurationMatrix[j][cities + 1] = (cities - 1);
                }
                else
                {
                    configurationMatrix[j][k] = 0;
                }  
            } 
        }
        root.CM = configurationMatrix;
        root.constraint.first = 0;
        root.constraint.second = 0;

        return root;
}

/*
    This is the function that all threads are called to work on. It calls a lot of different functions, and I will elaborate further
    thrughout the function.
*/
void* treeExpansion(void * arg)
{
    
    int * id =  (int*)arg; // I passed an ID to this treeExpansion function so that I could see which threads were executing. ID is a pointer to that integer
   
    node poppedNode;
    
    // Main loop 
    while (true)
    {
        // Each thread will need to acquire the mutex lock to even test the while condition. Once the acquire this lock, each will check if Q1 (which holds
        // all of the created nodes that will be used for further expansion) is empty. If it's empty, the thread will wait on the empty condition variable until
        // another thread signals it to wake up
        pthread_mutex_lock(&mLock);
        while(Q1.empty())
        {
            pthread_cond_wait(&empty, &mLock);

        }
        
        
        // At this point, the thread made it out of the while loop and has control of the mutex. It can now safely pop the top node (Q1 is a priority queue so
        // this will be the node with the lowest lowerbound). Once this is done, it frees up the mutex for other threads.
        poppedNode = Q1.top();
        Q1.pop();
        pthread_mutex_unlock(&mLock);
        
        // Print nodes and threadExiting are boolean global variables. Print nodes is set in the setup() function, and it's either true if we
        // are doing the 5 city simulation, or false if 6 city simulation. I use this so I can toggle on and off my cout statements. The threadExiting
        // variable will be explained further below, but basically I used it to try and get all the other threads to exit once the best route was found. 
        if (printNodes || threadExiting == true)
        {
            if(threadExiting == true)
            {
                pthread_exit(NULL);
            }
            // Mutex is used here so I can print the below statements without having another thread interrupt.
            pthread_mutex_lock(&mLock);
            std::cout << "Thread " << *id << " executing" << std::endl << std::endl;
            
            std::cout << "Popped Node to Examine: [" << poppedNode.constraint.first << ", " << poppedNode.constraint.second << "]" << std::endl;
            print(poppedNode);
            pthread_mutex_unlock(&mLock);
            
        }
        
        // Updateconstraint just updates the constraint that the poppedNode has. When we pop a node we want to update the constraint to see if we can
        // further include/exclude. The function returns a boolean, which indicates a route was found. I'll explain more below in the function. 
        bool routeFound = updateConstraint(poppedNode);

        // If a route is found, we can now proceed to prune the nodes in the queue that have a lowerbound greater than this. 
        if (routeFound || threadExiting == true)
        {
            if(threadExiting == true)
            {
                pthread_exit(NULL);
            }
            
            // We are pushing to the global foundRoutes array, so we need a mutex to ensure no race condition / weird values
            pthread_mutex_lock(&mLock);
            foundRoutes.push(poppedNode);
            pruneNodes(poppedNode, Q1); // This function will go through the queue and prune the nodes with lower bound > found route. It's not clear here 
                                        // because I did have it returned from this function, but the pruneNodes will set a global variable "endProgram" if it has
                                        // gone through the entire queue and no other nodes were left to expand. This indicates the best route has been found
            pthread_mutex_unlock(&mLock);
        }

        // As stated above, end program is set in the pruneNodes function and indicates best route was found. 
        if (endProgram == true || threadExiting == true)
        {
            if(threadExiting == true)
            {
                pthread_exit(NULL);
            }
            
            // Mutex used because we are printing the best route
            pthread_mutex_lock(&mLock);
            node bestRoute;
            bestRoute = foundRoutes.top();
            std::cout << "Best route obtained: " << bestRoute.lowerBound << std::endl
                      << std::endl;
            printNodes = true;
            print(bestRoute);
            
            pthread_mutex_unlock(&mLock);
            threadExiting = true; // This is where the above mentioned threadExiting is set. At this point, the best route is found and other threads 
                                   // should proceed to exit. The design of my program made this difficult, since threads can literally be 
                                   // aanywhere and everywhere in the treeExpansion function at any given time. I needed to constantly check this condition so I 
                                   // could get the threads to exit as fast as possible.
            pthread_exit(NULL);
        }
        else
        {
            // Else here indicates we have not found the best route yet. At this point, the thread has a popped node it needs to expand. First, we call check 
            // constraint, which will set the poppedNode's include and exclude boolean variables it contains in the struct. Each is set to true if
            // we can expand/exclude further
            checkConstraint(poppedNode);

            // We first see if we can include
            if (poppedNode.include)
            {
                node includeNode = poppedNode;
                modifyMatrix(includeNode, true); // Modify matrix will, for the include node, take the old (before constraintUpdate) matrix
                                                // and add the additional two ones to the matrix. 
                calculateAndCreate(includeNode); // We then calculate the lower bound for this node
                pthread_mutex_lock(&mLock); // Lock because we are going to print and push to the Q1 queue

                if(printNodes)
                {
                    std::cout << "Thread " << *id << " executing" << std::endl << std::endl;
                    std::cout << "Include Node Added" << std::endl;
                    print(includeNode);
                }
               

                includeNode.V.push_back(includeNode.constraint.second);
                
                Q1.push(includeNode);
                pthread_cond_signal(&empty); // If a thread was waiting at the empty condition variable (meaning Q1 was empty) it 
                                            // can now be signaled to awaken and continue execution
                pthread_mutex_unlock(&mLock);
                usleep(10); // I slept each thread for 10 microseconds because when I didn't do this, sometimes one of the threads would seem
                            // to have priority execution and not let the other threads do as much work. I didn't experiment with lowering this value,
                            // but that might be an option to save execution time
               
                
                
            }
            else
            {   
                pthread_mutex_lock(&mLock);
                if(printNodes)
                {
                    std::cout << "Thread " << *id << " executing" << std::endl << std::endl;
                    std::cout << "Cannot further include. Terminating node. " << std::endl << std::endl;
                }
                pthread_mutex_unlock(&mLock);
            }
            
            // This is the same stuctured logic as above, but for the exclude node
            if (poppedNode.exclude)
            {
                node excludeNode = poppedNode;
                modifyMatrix(excludeNode, false);
                calculateAndCreate(excludeNode);
                pthread_mutex_lock(&mLock);
                
                if(printNodes)
                {
                     std::cout << "Thread " << *id << " executing" << std::endl << std::endl;
                    std::cout << "Exclude Node Added" << std::endl;
                     print(excludeNode);
                }
               
                
                
                excludeNode.V.push_back(excludeNode.constraint.second);
                
                Q1.push(excludeNode);
                pthread_cond_signal(&empty);
                pthread_mutex_unlock(&mLock);
                usleep(10);
                
                
                
                
            }
            else
            {
                pthread_mutex_lock(&mLock);
                if(printNodes)
                {
                    std::cout << "Thread " << *id << " executing" << std::endl << std::endl;
                    std::cout << "Cannot further exclude. Terminating node. " << std::endl << std::endl;
                }
                pthread_mutex_unlock(&mLock);
            }
           
        }
    }
}

// This function calculates the lower bound for a given node
void calculateAndCreate(node &s)
{
    double lower_bound = 0;
    int smallest = 100;
    std::vector<int> temp;
    int count = 0;
    
    // Matrix that contains edge cost cells for each simulation
    std::vector<std::vector<int> > simulationOne {
        {0, 3, 4, 2, 7},    
        {3, 0, 4, 6, 3},    
        {4, 4, 0, 5, 8},    
        {2, 6, 5, 0, 6},    
        {7, 3, 8, 6, 0}     
    };

    std::vector<std::vector<int> > simulationTwo {
        {0, 2, 4, 1, 7, 2},   
        {3, 0, 2, 7, 3, 4},   
        {4, 9, 0, 7, 8, 2},   
        {2, 9, 5, 0, 6, 6},   
        {7, 9, 8, 7, 0, 2},   
        {3, 9, 5, 7, 2, 0},           
    };

    std::vector<std::vector<int> > adjacencyMatrix;
    // Cities was set in the setup, so we can use it to determine which data we are using
    if(cities == 5)
    {
        adjacencyMatrix = simulationOne; 
    }
    else
    {
        adjacencyMatrix = simulationTwo;
    }
    
    // Nested for loop through the matrix looking for 1's. 
    for (int j = 0; j < cities; j++)
    {    
        for (int k = 0; k < cities; k++)
        {       
            if(s.CM[j][k] == 1) 
            {       
                count++;
                lower_bound += adjacencyMatrix[j][k];   
            }
            else if ((s.CM[j][k] == 0) && (j != k))
            {
                temp.push_back(adjacencyMatrix[j][k]); // We save the zeros that aren't along the columns (i.e dont save AA, BB, CC) 
            }
        }

        // Sorts smallest to largest
        sort(temp.begin(), temp.end());

        // Count = 0 indicates we have found no zeros in that row. So, we need to get the two smallest edge costs in this array
        if(count == 0)
        {
            int s = temp.front();
            temp.erase(temp.begin());
            lower_bound+= s; // First one obtained
            int d = temp.front();
            temp.erase(temp.begin());
            lower_bound+=d; // Second one obtained
        }
        // We could also have found one 1, and would need to find only the smallest element in the array
        if(count == 1)
        {
            int s = temp.front();   
            temp.erase(temp.begin());
            lower_bound += s;
        }
        // If count = 2, the lower bound should be the desired result since we calculated it above 
        count = 0;
        temp.clear();
    }
    lower_bound = lower_bound / (double)2;
    s.lowerBound = lower_bound;
}

// Prints a node
void print(node s)
{
    
    int character = 0;
    char ch = 'A';
    
    if (printNodes)
    {
        std::cout << "Lowerbound : " << s.lowerBound << std::endl << std::endl;
        std::cout << std::setw(2);
        for(int i = 0; i < cities; i++)
        {
            character = int(ch);
            std::cout << ch << "  ";
            character++;
            ch=char(character);
        }
        std::cout << "#1" << " " << "~#1" << std::endl;
        
        std::cout << "----------------------";
        std::cout << std::endl;
        for (int j = 0; j < cities; j++)
        {
            for (int k = 0; k < (cities + 2); k++)
            {
                std::cout << std::setw(2) << s.CM[j][k] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "**********************" << std::endl << std::endl;
    }
    
    
}

/* This is the pruneNodes function, which will be called after we have found a route and want to prune nodes with lower bound > this.
*/
bool pruneNodes(node &temp, std::priority_queue<node, std::vector<node>, Comparator> Q1)
{
    int count = 0;
    node s = foundRoutes.top(); // Gets the best node we have so far in the foundRoutes priority queue
    bool flag = false;
    //s = routes.top();
    // We pop until we have found the next node that is to be expanded, or until the queue is empty, meaning we got the best route
    Q1.pop();
    while ((Q1.empty() == false) && (flag == false))
    {
        count++;
        temp = Q1.top();

        if (temp.lowerBound >= s.lowerBound)
        {
            if (printNodes)
            {

                std::cout << "Node terminated. Lowerbound: " << temp.lowerBound << " > calculated Route " << std::endl
                          << std::endl;
            }
            Q1.pop();
        }
        else
        {
            flag = true;
        }
    }
    // If empty, we have found the best route
    if (Q1.empty())
    {
        endProgram = true;
        return true;
    }
    else if (count > 0)
    {
        updateConstraint(temp);
    }
    return false;
    


}
// Takes a node and a boolean. Boolean is used to determine if we are adding 1's or -1's to the matrix
void modifyMatrix(node &s, bool include)
{
    int excludeColumnTotal = cities - 1;
    int nextRowExclude = cities -1;
    int includeColumnTotal = 0;
    int nextRowInclude = 0;
    int row, column = 0;
    
    if(include)
    {
        row = s.constraint.first;
        column = s.constraint.second;
        s.CM[row][column] = 1;
        row = s.constraint.second;
        column = s.constraint.first;
        s.CM[row][column] = 1;        
    }
    else
    {
        row = s.constraint.first;
        column = s.constraint.second;
        s.CM[row][column] = -1;
        row = s.constraint.second;
        column = s.constraint.first;
        s.CM[row][column] = -1;
    }
    
    for (int i = 0; i < cities; i++)
    {
        if (s.CM[s.constraint.first][i] == 1)
        {
            includeColumnTotal += 1;
            excludeColumnTotal -= 1;
            
        }
        if (s.CM[s.constraint.first + 1][i] == 1)
        {
            nextRowInclude += 1;
            nextRowExclude -= 1;
            
        }
        if (s.CM[s.constraint.first][i] == -1)
        {
            excludeColumnTotal -= 1;
            
        }
        if (s.CM[s.constraint.first + 1][i] == -1)
        {
            nextRowExclude -= 1;       
        }
    }
    
    s.CM[s.constraint.first][cities ] = includeColumnTotal; // Will calculate the 2nd to last column, indicating how many edges have been included
    s.CM[s.constraint.first + 1][cities] = nextRowInclude;  // Calculates next rows 2nd to last column
    s.CM[s.constraint.first][cities + 1] = excludeColumnTotal; // calculates last column 
    s.CM[s.constraint.first + 1][cities + 1] = nextRowExclude; // calculates next rows last column
}

/*
    This function is used to determine whether we can include/exclude further for a given node

*/
void checkConstraint(node &s)
{
    s.include = true;
    s.exclude = true;
    int row = s.constraint.first;
    
    // Can't include if 2nd to last column is 2
    if (s.CM[row][cities] == 2)
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
    if ((s.CM[row][cities] == 0) && (s.CM[row][cities + 1] == 2) || (s.CM[row][cities] == 1 && s.CM[row][cities + 1] == 1))
    {
        s.exclude = false;
    }
}

// This function will update the constraint of a given poppedNode we are trying to expand
bool updateConstraint(node &s)
{
    // We can no longer expand, route is found
    if((s.constraint.first == cities - 2) && (s.constraint.second == cities - 1))
    {
        std::cout << "Route found. " << std::endl;
        foundRoute = true; 
        
    }
    // Otherwise
    if(foundRoute == false)
    {
        // If we are at the last column, we need to go down a row 
        if (s.constraint.second == (cities - 1))
        {
            s.constraint.first++; // go down a row
            s.constraint.second = s.constraint.first + 1; // go over to column 1 past what we were just on in the previous row

            if (s.constraint.second > cities - 1)
            {
                s.constraint.second = cities - 1;
            }
        }
        else
        {
            s.constraint.second++;
        }
    }
    return foundRoute;
}






