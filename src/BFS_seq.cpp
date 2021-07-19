#include <Parallel_BFS/BFS_seq.hpp>
#include <set>
#include <thread>
#include <chrono>
#include <numeric>
#include "../src/utimer.hpp"

int BFS_seq(int x, const vector<Node> &nodes)
{
    unsigned long counter = 0;

    vector<int> frontier{0};
    frontier.reserve(nodes.size() / 2);
    vector<int> next_frontier{};
    next_frontier.reserve(nodes.size() / 2);
    vector<bool> explored_nodes(nodes.size(), false);

    while (!frontier.empty())
    {
        for (int n_id : frontier)
        {
            if (explored_nodes[n_id]) continue;
            explored_nodes[n_id] = true;
            counter += nodes[n_id].value == x;
            next_frontier.insert(
                next_frontier.end(),
                nodes[n_id].children.begin(),
                nodes[n_id].children.end()
            );
        }
        frontier = move(next_frontier);
        next_frontier.clear();
    }
    return counter;
}


/*

BFS SEQ using sets

int BFS_seq(int x, const vector<Node> &nodes)
{
    int counter = 0;

    set<int> frontier{0};
    set<int> next_frontier{};
    vector<bool> explored_nodes(nodes.size(), false);
    // vector<int> frontier_size;

    while (!frontier.empty())
    {
        // frontier_size.push_back(frontier.size());
        for (int n_id : frontier)
        {
            if (explored_nodes[n_id])
                continue;
            explored_nodes[n_id] = true;
            counter += nodes[n_id].value == x;
            for (int child : nodes[n_id].children)
                if (!explored_nodes[child])
                    next_frontier.insert(child);
        }
        frontier = next_frontier;
        next_frontier.clear();
    }
    // cout << "frontier sizes: " << frontier_size << endl;
    return counter;
}

*/