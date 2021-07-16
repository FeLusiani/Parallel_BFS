#include "../src/build_graph.hpp"
#include <thread>
#include <atomic>
#include <set>
#include "../src/utimer.hpp"


class SpinLock
{
    std::atomic_flag locked = {false};

public:
    void lock()
    {
        while (locked.test_and_set(std::memory_order_acquire));
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }
};

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads)
{

    atomic<int> counter = {0};
    vector<bool> explored_nodes(nodes.size(), false);

    vector<int> frontier{0};
    vector<vector<int>> next_frontiers(NumThreads);

    auto work_loop = [&](int tid)
    {
        int to_process = frontier.size() / NumThreads;
        int start = to_process * tid;
        int end = tid < (NumThreads - 1) ? start + to_process : frontier.size();
        
        // process the work
        if (start == end) return;
        int my_counter = 0;
        {
        utimer t("frontier");
        for (int f_pos = start; f_pos < end; f_pos++)
        {
            int n_id = frontier[f_pos];
            if (explored_nodes[n_id])
                continue;
            explored_nodes[n_id] = true;
            my_counter += nodes[n_id].value == x;
            next_frontiers[tid].insert(
                next_frontiers[tid].end(),
                nodes[n_id].children.begin(),
                nodes[n_id].children.end()
            );
        }
        }
        // join results to global results
        counter += my_counter;
    };

    long int seq_time = 0;
    long int par_time = 0;
    while (!frontier.empty())
    {
        cout << "f size " << frontier.size() << endl;
        vector<thread> workers;

        {
        utimer par_t1("parallel");
        my_timer t;   
        for (int tid = 0; tid < NumThreads; tid++)
            workers.push_back(thread(work_loop, tid));

        for (auto& t : workers) t.join();
        par_time += t.get_time();
        }

        frontier.clear();

        {
        // utimer t("merging");
        my_timer t;
        for (auto& partial : next_frontiers){
            frontier.insert(frontier.end(), partial.begin(), partial.end());
            partial.clear();
        }
        seq_time += t.get_time();
        }

    }

    cout << "seq time is " << seq_time << endl;
    cout << "par time is " << par_time << endl;
    return counter;
}

// int BFS_par_ff(int x, const vector<Node> &nodes, int nw);
