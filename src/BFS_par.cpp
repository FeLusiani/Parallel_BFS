#include "../src/build_graph.hpp"
#include <thread>
#include <atomic>
#include <set>

class SpinLock
{
    std::atomic_flag locked = {false};

public:
    void lock()
    {
        while (locked.test_and_set(std::memory_order_acquire))
        {
            ;
        }
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }
};

int BFS_par(int x, const vector<Node> &nodes, int NumThreads)
{

    atomic<int> counter = {0};
    vector<bool> explored_nodes(nodes.size(), false);

    vector<int> frontier{0};
    set<int> next_frontier;
    SpinLock nf_lock;

    auto work_loop = [&](int tid)
    {
        // get your slice of work to do
        int to_process = frontier.size() / NumThreads;
        auto start = frontier.begin() + to_process * tid;
        auto end = tid < (NumThreads - 1) ? start + to_process : frontier.end();

        // process the work
        if (start == end) return;
        int my_counter = 0;
        set<int> my_next_frontier;
        for (auto node = start; node < end; node++)
        {
            int n_id = *node;
            if (explored_nodes[n_id])
                continue;
            explored_nodes[n_id] = true;
            my_counter += nodes[n_id].value == x;
            for (int child : nodes[n_id].children)
                if (!explored_nodes[child])
                    my_next_frontier.insert(child);
        }

        // join results to global results
        counter += my_counter;

        nf_lock.lock();
        next_frontier.insert(my_next_frontier.begin(), my_next_frontier.end());
        nf_lock.unlock();
    };

    while (!frontier.empty())
    {
        vector<thread> workers;
        for (int tid = 0; tid < NumThreads; tid++)
            workers.push_back(thread(work_loop, tid));

        for (auto& t : workers) t.join();

        frontier.resize(next_frontier.size());
        copy(next_frontier.begin(), next_frontier.end(), frontier.begin());
        next_frontier.clear();
    }

    return counter;
}