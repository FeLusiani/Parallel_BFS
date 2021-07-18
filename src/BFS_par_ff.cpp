#include "../src/build_graph.hpp"

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>
using namespace ff;

#include "../src/utimer.hpp"


struct worker_result{
    int counter = 0;
    vector<int> next_frontier;
};

int BFS_par_ff(int x, const vector<Node> &nodes, int nw)
{
    ParallelForReduce<worker_result> pf(nw);
    int step = 1;
    int chunk = 0;

    ffTime(START_TIME);
    vector<bool> explored_nodes(nodes.size(), false);

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

    // long int seq_time = 0;
    while (!frontier.empty())
    {
        pf.parallel_reduce(WR, worker_result{}, 0, frontier.size(), step, chunk, mapF, reduceF, nw);
        ffTime(STOP_TIME);

        frontier = move(WR.next_frontier);
        WR.next_frontier.clear();
    }


    // cout << "seq time is " << seq_time << endl;
    cout << "par time is " << ffTime(GET_TIME) << endl;
    return WR.counter;
}
