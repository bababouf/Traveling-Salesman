# **_Traveling-Salesman_**  
![](https://i.gyazo.com/3c97d9e64f87cfb35b7d00767fb492c0.png)  

## **_Overview_**  
A salesman is looking to travel around to several nearby cities, only stopping at each city once. The journey will end when they return back to their home city. The traveling salesman problem is an optimization
problem that seeks to minimize the cost for this route; in other words, what is the minimum distance required to hit all necessary cities only once and return to the starting city?  

This multithreaded program, written in C++, offers a heuristic approach in determining the optimal route for TSP. 
## **_How To Run_**  
(Steps for MinGW GNU Compiler)  

1. Clone the repository to local filesystem: **_git clone https://github.com/bababouf/Traveling-Salesman.git_**
2. Compile using c++11: **_g++ -std=c++11 TravelingSalesmanParallel.cpp -o TSP_**
3. Run: **_./TSP.exe_**

## **_General Program Details_**  
This program allows the user to choose between a 5 city simulation and a 6 city simulation (both of which use the same heuristic approach to solving TSP).  


![](https://i.gyazo.com/3f84c2615a346a11914f8def2caffadb.png)  

Below are the edge costs for each simulation:  

**5-City Adjacency Matrix**  
![](https://i.gyazo.com/e60d252727d2bcc610b76fdfb03d8219.png)  
**6-City Adjacency Matrix**  
![](https://i.gyazo.com/504c62f3931ee6b2f5a5932f0a33d90d.png)  

Each cell in the above 2D matrices represents the cost to travel from city [row][column]. In this way, the cell [0][1] for the 5-city simulation, which contains the integer value 3, would represent the cost to travel from city 0 to city 1. Throughout the code, as well as in the rest of this readMe file, I will refer to each city in alphabetical order, so row 0 would correspond to city 'A' and row 4 would correspond to city 'E'. In the same way, column 0 would correspond to city 'A' and column 4 would correspond to city 'E'.  

It's important to note here that the adjacency matrices, which are symmetrical and square, contain all zeroes along their diagonals. This is due to the fact that the cost from one city to itself is 0. In addition, since the matrix is symmetrical, all the values are mirrored, meaning cell [row][column] = [column][row], or the cost from city 'A' to 'B' = cost of city 'B' to 'A'.

### _Nodes_  
The main data structure used in this program is a node:  

![](https://i.gyazo.com/a58ae079483da113140144049bcd962e.png)  

A node represents a route in expansion, and each of its members contain a critical piece of information regarding that route. Each member is discussed below.

**_configurationMatrix_**: This 2D matrix (n x (n + 2)) identifies which edges have been include and excluded in the current node's expansion. For the n x n part of the matrix, each cell corresponds to the cell in the adjacency matrix. In other words, [0][1] could be seen as the edge from city 'A' to 'B'. The 2nd to last column holds the number of edges that have been included in that row, and the last column contains the number of edges that can be included or excluded. 

**_lowerBound_**: The lowerBound for a node is the total cost (integer value) for the route in its current state. If only 3 edges have been included, the lowerBound is the total cost of those 3 edges.  

**_constraint_**: The constraint is a pair of integers (<int><int>) that determine which edge is being currently examined. For example, when the root node is first created and initialized, the constraint is <0><1>, meaning the first edge to be examined is the (0,1) edge from city 'A' to city 'B' (assuming row 0 was city A, row 1 was city B, etc).  

**V**:  

**_include/exclude_**: These boolean flags are set to indicate if a node, given it's constraint, can include or exclude the current edge being examined  


### _ConfigurationMatrix_  
Below is the initialized configurationMatrix for the 5-city simulation. The 5 x 5 cells in the matrix are all set to 0, indicating that no edges in the matrix have been included or excluded. The last column, however, is set to (numberOfCities - 1), which is 4 for the 5 city simulation. This indicates that for each row in the matrix, 4 edges can be included or excluded. This column is set to (numberOfCities - 1) since a route cannot contain an edge from a city to itself.  

![](https://i.gyazo.com/59f2fecc8373f3906127cb08f69bf2a8.png)  

## **_Flowchart_**  
![](https://lucid.app/publicSegments/view/27073b7b-b7b4-4432-9223-9e54c02c01a7/image.png)  

The program can be broken up into two major parts:
- Setup
- Program Loop

**Setup**  
1. _readInSimulationMode()_ : prompts the user to select between the 5-city or 6-city simulation. All other choices are rejected
2. _initializeConfigurationMatrix()_ : will initialize the configuration matrix (depending on the simulation selected) as discussed above 
3. _setAdjacencyMatrix()_ : sets the adjacency matrix to either the 5-city matrix or 6-city matrix
4. Push root node into unprocessedNodesQueue
5. _nodeExpansionDispatcher()_ : calling this method starts the program loop

**Program Loop**  

The program loop continues until the unprocessedNodesQueue is empty, upon which the best route will be printed out to the console. Initially, the root node is the only node in the unprocessedNodesQueue. 

1. _aqcuireUnprocessedNode()_ : a node is popped from the unprocessedNodesQueue
2. _updateNodeConstraint()_ : the constraint, as discussed above, keeps track of the cell that will be examined for inclusion/exclusion. A constraint is a pair of integers (<row><column>). At each iteration in the loop, the constraint is updated a column at a time. The root node starts with a constaint of <0><0>, and is updated as follows:

- The column is incremented until the last column (numberOfCities - 1) is reached. For a 5-city simulation, this is column 4.  

- Once the last column is reached, the row is incremented by 1. The column then becomes equal to (row + 1). Example: If we are at constraint <0><4>, the constraint now becomes <1><2> for the next iteration. Similarly, if we are at constraint <2><4>, the constraint becomes <3><4>.  

- Updating the constraint is no longer possible (signifying that a route has been found) when we reach a constraint of 
<(numberOfCities - 2)><(numberOfCities - 1)>. In the above example, <3><4> is the final constraint and cannot be updated.







