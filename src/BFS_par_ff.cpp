#include "../src/build_graph.hpp"

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
using namespace ff;

#include "../src/utimer.hpp"

// ATOMIC_EXPLORED
#define ARRAY

#ifdef ARRAY

int BFS_par_ff(int x, const vector<Node> &nodes, int nw, int chunk)
{
    const int n_nodes = nodes.size();
    vector<unsigned char> explored_nodes(n_nodes, false);

    vector<unsigned char> frontier(n_nodes, false);
    frontier[0] = true;
    vector<unsigned char> next_frontier(n_nodes, false);
    bool children_added = true;

    ParallelForReduce<long int> pf(nw);
    int step = 1;

    auto mapF = [&](int n_id, long& my_counter)
    {
        if (!frontier[n_id] || explored_nodes[n_id]) return;
        explored_nodes[n_id] = true;
        my_counter += nodes[n_id].value == x;
        for (int child : nodes[n_id].children){
            children_added = true;
            next_frontier[child] = true;
        }
    };

    auto reduceF= [](long& red_counter, const long& partial_count){
        red_counter += partial_count;
    };

    long tot_counter = 0;
    // long int seq_time = 0;
    while (children_added)
    {
        long counter = 0;
        children_added = false;
        pf.parallel_reduce(counter, 0, 0, n_nodes, step, chunk, mapF, reduceF, nw);
        swap(frontier, next_frontier);
        fill(next_frontier.begin(), next_frontier.end(), false);
        tot_counter += counter;
    }
    return tot_counter;
}

#endif

#ifdef ATOMIC_EXPLORED

struct worker_result{
    int counter = 0;
    vector<int> next_frontier;
};

int BFS_par_ff(int x, const vector<Node> &nodes, int nw, int chunk)
{
    const int n_nodes = nodes.size();
    ParallelForReduce<worker_result> pf(nw);
    int step = 1;

    auto explored_nodes = new atomic<bool>[n_nodes];
    for (int i =0; i<n_nodes; i++) explored_nodes[i] = false;

    vector<int> frontier{0};
    frontier.reserve(nodes.size() / 2);
    // vector<int> next_frontier{};
    worker_result WR;
    WR.next_frontier.reserve(nodes.size() / 2);

    auto mapF = [&](int i, worker_result& my_wr)
    {
        int n_id = frontier[i];
        if (explored_nodes[n_id])
            return;
        explored_nodes[n_id] = true;
        my_wr.counter += nodes[n_id].value == x;
        my_wr.next_frontier.insert(
            my_wr.next_frontier.end(),
            nodes[n_id].children.begin(),
            nodes[n_id].children.end()
        );
    };

    auto reduceF= [](worker_result& red_wr, const worker_result& wr_part){
        red_wr.counter += wr_part.counter;
        red_wr.next_frontier.insert(
            red_wr.next_frontier.end(),
            wr_part.next_frontier.begin(),
            wr_part.next_frontier.end()
        );
    };

    while (!frontier.empty())
    {
        pf.parallel_reduce(WR, worker_result{}, 0, frontier.size(), step, chunk, mapF, reduceF, nw);

        frontier = move(WR.next_frontier);
        WR.next_frontier.clear();
    }


    delete explored_nodes;
    return WR.counter;
}

#endif

#ifdef SET

#endif