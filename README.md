# **_Traveling-Salesman_**  
![](https://i.gyazo.com/3c97d9e64f87cfb35b7d00767fb492c0.png)  

## **_Overview_**  
A salesman is looking to travel around to several nearby cities, stopping at each city only once. The journey will end when they return back to their home city. The traveling salesman problem is an optimization
problem that seeks to minimize the cost for this route. In other words, what is the minimum distance required to hit all necessary cities only once and return to the starting city?  

This multithreaded program, written in C++, offers a heuristic approach in determining the optimal route for TSP. 
## **_How To Run_**  
(Steps for MinGW GNU Compiler)  

1. Clone the repository to local filesystem: **_git clone https://github.com/bababouf/Traveling-Salesman.git_**
2. Compile using c++11: **_g++ -std=c++11 TravelingSalesmanParallel.cpp -o TSP_**
3. Run: **_./TSP.exe_**

## **_General Program Details_**  
This program allows the user to choose between a 5, 6, or 7 city simulation. 


![](https://i.gyazo.com/ca826145a575909e1f93ce00bb877f02.png)  

Below are the edge costs for each simulation:  

**5-City Adjacency Matrix**  
![](https://i.gyazo.com/e60d252727d2bcc610b76fdfb03d8219.png)  
**6-City Adjacency Matrix**  
![](https://i.gyazo.com/504c62f3931ee6b2f5a5932f0a33d90d.png)  
**7-City Adjacency Matrix**  
![](https://i.gyazo.com/1d7a80e0609fd3584833b77e76e66fd7.png)  

Each cell in the adjacency matrix represents the cost to travel from city one city to another. For example, the cell (0,1) would represent the cost to travel from city '0' to city '1'. This edge cost is the same as the edge cost to get from city '1' to city '0', since the matrices are symmetrical. In addition, the cost to get from one city to itself is zero, which is the reason why each of the matrices has zeroes along the diagonal.  





### _Nodes_  
The main data structure used in this program is a node:  

![](https://i.gyazo.com/d49da16b9f3d8c5e2d0a3f6b19a40b51.png)  

A node represents a route in expansion, and each of its members contain a critical piece of information regarding that route. Each member is discussed below.

**_configurationMatrix_**: This 2D matrix keeps track of all edges that have been included/excluded. Two additional columns are used to determine if a cell's edge can be included/excluded.

**_lowerBound_**: The lowerBound for a node is the total cost for the edges that have been included so far.

**_constraint_**: The constraint is a pair of integers that determine which edge is being currently examined. For example, when the root node is first created and initialized, the constraint is (0, 0). Each time a node is popped the constraint will be updated and examined if an edge can be included or excluded.   

**_include/exclude_**: These boolean flags are set to indicate if a node, given it's constraint, can include or exclude the current edge being examined  


**_previouslyVisited_**: A vector containing all cities that have been visited 


### _ConfigurationMatrix_  
Below is the initialized configurationMatrix for the 5-city simulation. The matrix itself is size 5 x 7; the 5 x 5 part of the matrix reprsents the edges from one city to the next. The last two columns hold information that determine whether a specific edge can be included or excluded.  

I will refer to the 2nd to last column as the "include column" and the last column as the "exclude column". The include column ensures that each row contains a maximum of two edges (since each city can only have two edges to be a valid TSP route). For example, row '0', which represents city '0', contains all the edges from city '0' to every other city. If three edges were included in this row, city '0' would not be a valid TSP route.  

The exclude column is initialized to the number of cities in the simulation minus one. This column keeps track of the number of edges that have been either excluded or included. If an edge is included or excluded, the appropriate cell in that column is decremented. 

![](https://i.gyazo.com/59f2fecc8373f3906127cb08f69bf2a8.png)  

## **_Flowchart_**  
![](https://i.gyazo.com/cfdd70d09c123412be0a87c743d7f0f6.png)  

The program can be broken up into two major parts:
- Setup
- Program Loop

**Setup**  
1. _readInSimulationMode()_ : prompts the user to select between the 5, 6, or 7 city simulation. All other choices are rejected
2. _initializeConfigurationMatrix()_ : will initialize the configuration matrix (depending on the simulation selected) as discussed above 
3. _setAdjacencyMatrix()_ : sets the adjacency matrix to either the 5, 6, or 7 city matrix

**Program Loop**  


The program loop starts when _nodeExpansionDispatcher()_ is called, and continues until the unprocessedNodesQueue is empty, upon which the best route will be printed out to the console. Initially, the root node is the only node in the unprocessedNodesQueue. 

1. _aqcuireUnprocessedNode()_ : a node is popped from the unprocessedNodesQueue
2. _updateNodeConstraint()_ : the constraint, as discussed above, keeps track of the cell that will be examined for inclusion/exclusion. A constraint is a pair of integers (<row><column>). At each iteration in the loop, the constraint is updated a column at a time. The root node starts with a constaint of <0><0>, and is updated as follows:

- The column is incremented until the *last column* (numberOfCities - 1) is reached. For a 5-city simulation, this is column 4.  

- Once the last column is reached, the row is incremented by 1. The column then becomes equal to (row + 1). Example: If we are at constraint <0><4>, the constraint now becomes <1><2> for the next iteration. Similarly, if we are at constraint <2><4>, the constraint becomes <3><4>.  

- Updating the constraint is *no longer possible* (signifying that a ROUTE has been found) when we reach a constraint of 
<(numberOfCities - 2)><(numberOfCities - 1)>. In the above example, <3><4> is the final constraint and cannot be updated. When a route is found, the updateConstraint method returns true. 

3. _if(routeFound)_ : updateNodeConstraint() returns true if a route is found
   - If true, that route (node) is pushed into a queue of other found routes. This route is then used to prune all other nodes with a higher lowerbound than it from the unprocessedNodesQueue. In the three simulations, each time a route is found it *IS* the best possible route, and all other nodes are terminated from the queue. I will talk more about this in the analysis section below.  
   - If false, continue.
4. _setNodeFlags()_ : each node has two boolean variables, one for include and one for exclude. This method determines what these booleans are set to. As discussed earlier, the configurationMatrix for each node contains two additional columns (beyond the n x n cells) that are used to determine whether the cell we are examining (determined by the constraint) can be expanded to include/exclude that edge. The 2nd to last cell (which will be called the inclusion column) contains the number of edges that have been included in that row, and the last column (which will be called the exclusion column) contains the number of edges that can be included or excluded.
5. At this point, two threads are created; one is assigned to checkInclude() and one to checkExclude(). If an edge cannot be included or excluded, the thread carrying out that task will simply print to console and terminate the node. Otherwise, the following stage will commence:
6. ModifyMatrix() will modify a node's configurationMatrix, adding either '1' or '-1' to the appropriate cell. Since the configurationMatrix is symmetrical (X -> Y == Y -> X) two cells need to be modified with either '1s' or '-1s'. In addition, the includeColumn and excludeColumn for both of these rows that the modified cells are in will be updated. For example, if [0][1] is the edge being added, [1][0] will also be modified and both the include columns for each of those rows will be incremented. Finally, each row's exclude column will be decremented. 
7. CalculateLowerbound() will be called to calculate the new lowerbound for the node. This method takes into account a node's current included edges, plus the lowest cost edges in all other rows. The total cost summed together is the lowerbound for the node, and it may or may not lead to a valid route. For example, if A -> B and A -> C were the node's current included edges, B -> C could potentially be chosen as the least cost edge for the next row, closing the route off (A -> B -> C) before the rest of the cities could be explored. The lowerbound is calculated as a "best guess" heuristic to help decide which node to expand next. 
8. Finally, the updated node(s) will be pushed back into the queue, the threads will be joined, and the main thread will attempt to pop the next node from the queue. In the event that the queue is empty, a route will have been found and all other nodes would have been pruned from the queue. Once this is carried out, the best route will be printed to console.

**5 City Simulation Route Found:**  

![](https://i.gyazo.com/056b0f8755f0fd01e10eb9f5ab7cb51e.png)  
 
## **_Analysis_**  

A naive approach to solving the Traveling Salesman Problem might use brute force, where every possible route is calculated. For X number of cities, this would require (X - 1)! permutations to compute a cost for.  

Using 3 cities as an example, the possible routes would be (3 - 1)!:  
A -> B -> C -> A
A -> C -> B -> A  

As the number of cities grow, however, this number becomes much larger. For 7 cities, the required permutations to compute would be 6!, or 720.  

Another approach is the one taken with this program: branch-and-bound. At every step, only a partial solution is explored. The lowerbound calculated for each node serves as a heuristic to guide the search, prioritizing nodes who contain the lowest lowerbounds. In this way, only a subset of the entire search space must be explored. 





