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

It's important to note here that the adjacency matrices, which are symmetrical and square, contain all zeroes along their diagonals. This is due to the fact that the cost from one city to itself is 0. In addition, since the matrix is symmetrical, all the values are mirrored, meaning cell [row][column] = [column][row], or the cost from city 'A' to 'B' = cost of city 'B' to 'A'

### _Nodes_  
The main data structure used in this program is a node:  

![](https://i.gyazo.com/a58ae079483da113140144049bcd962e.png)  

A node represents a route in expansion, and each of its members contain a critical piece of information regarding that route. Each member is discussed below.

**_configurationMatrix_**: This 2D matrix (n x (n + 2)) identifies which edges have been include and excluded in the current node's expansion. For the n x n part of the matrix, each cell corresponds to the edge from city [row][column]. In other words, [0][1] could be seen as the edge from city 'A' to 'B' (assuming row 0 was city A, row 1 was city B, etc). More information can be found in the configurationMatrix section below.  

**_lowerBound_**: The lowerBound for a node is the total cost (integer value) for the route in its current state. If only 3 edges have been included, the lowerBound is the total cost of those 3 edges.  

**_constraint_**: The constraint is a pair of integers (<int><int>) that determine which edge is being currently examined. For example, when the root node is first created and initialized, the constraint is <0><1>, meaning the first edge to be examined is the (0,1) edge from city 'A' to city 'B' (assuming row 0 was city A, row 1 was city B, etc).
**V**:  
**_include/exclude_**: These boolean flags are set to indicate if a node, given it's constraint, can include or exclude the current edge being examined  


### _ConfigurationMatrix_
![](https://i.gyazo.com/59f2fecc8373f3906127cb08f69bf2a8.png)




