#ifndef BUILD_GRAPH
#define BUILD_GRAPH

#include <string>
#include <vector>
#include <iostream>
#include <iterator> // needed for std::ostram_iterator

using namespace std;

void build(string filepath);

template <typename T>
ostream &operator<<(ostream &out, const vector<T> &v)
{
    if (!v.empty())
    {
        out << '[';
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "\b\b]";
    }
    else{
        out << "[]";
    }
    return out;
}

struct Node
{
    int id;
    int value;
    vector<int> children;
    Node();
    Node(int id, int max_val);
};

ostream &operator<<(ostream &out, const Node &node);

class Graph
{
    int n_nodes;
    int out_bound;
    int max_val;
    int first_free;
    int depth;

    int random_walk(int start_node, int max_depth);

public:
    vector<Node> nodes_array;
    Graph(int num_nodes, int out_bound, int max_val);
    bool full();
    int new_node();
    void build();
    void add_connections(int n);
    friend ostream& operator<<(ostream& out, const Graph& graph);
    friend istream& operator>>(istream& in, Graph& graph);
};



#endif