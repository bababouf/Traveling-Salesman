struct node{
    std::vector<std::vector<int>> configurationMatrix; 
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