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

## **_Program Details_**  
This program allows the user to choose between a 5 city simulation and a 6 city simulation (both of which use the same heuristic approach to solving TSP).  


![](https://i.gyazo.com/3f84c2615a346a11914f8def2caffadb.png)  

Below are the edge costs for each simulation:  

**5-City Adjacency Matrix**  
![](https://i.gyazo.com/e60d252727d2bcc610b76fdfb03d8219.png)  
**6-City Adjacency Matrix**  
![](https://i.gyazo.com/504c62f3931ee6b2f5a5932f0a33d90d.png)  

Each cell in the above 2D matrices represents the cost to travel from city [row][column], and the values can be seen as arbitrary. Both matrices are symmetrical, and their diagonals contain only zeroes since the cost
of traveling from city x to city x is 0.  

### _Nodes_  
The main data structure used in this program is a node:  

![](https://i.gyazo.com/a58ae079483da113140144049bcd962e.png)  

A node represents a route in expansion

### _ConfigurationMatrix_
![](https://i.gyazo.com/59f2fecc8373f3906127cb08f69bf2a8.png)




