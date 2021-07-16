#include <Parallel_BFS/BFS_seq.hpp>
#include <set>

int BFS_seq(int x, const vector<Node> &nodes)
{
    int counter = 0;

    set<int> frontier{0};
    set<int> next_frontier{};
    vector<bool> explored_nodes(nodes.size(), false);

    while (!frontier.empty())
    {
        for (int n_id : frontier)
        {
            if (explored_nodes[n_id])
                continue;
            explored_nodes[n_id] = true;
            counter += nodes[n_id].value == x;
            // auto const &children = nodes[n_id].children;
            // next_frontier.insert(children.begin(), children.end());
            for (int child : nodes[n_id].children)
                if (!explored_nodes[child])
                    next_frontier.insert(child);
        }
        frontier = next_frontier;
        next_frontier.clear();
    }

    return counter;
}
