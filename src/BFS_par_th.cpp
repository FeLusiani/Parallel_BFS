#include "../src/build_graph.hpp"
#include <thread>
#include <atomic>
#include <set>
#include "../src/utimer.hpp"
#include <numeric>


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

// ATOMIC_EXPLORED
#define ARRAY

#ifdef ARRAY

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads, int chunk)
{
    const int n_nodes = nodes.size();
    atomic<unsigned long> counter = {0};
    vector<unsigned char> explored_nodes(n_nodes, false);

    vector<unsigned char> frontier(n_nodes, false);
    frontier[0] = true;
    vector<unsigned char> next_frontier(n_nodes, false);
    bool children_added = true;
    const int padding = 8;
    vector<int> counters(NumThreads*padding, 0);
    if (chunk < 1) chunk = 32;

    auto workF = [&](int tid)
    {
        counters[tid*padding] = 0;
        for (int pos = tid*chunk; pos < n_nodes; pos += NumThreads*chunk)
        {
            int my_chunk = min(chunk, n_nodes-pos);
            for (int offset=0; offset < my_chunk; offset++){
            int n_id = pos+offset;
            if (!frontier[n_id] || explored_nodes[n_id]) continue;
            explored_nodes[n_id] = true;
            counters[tid*padding] += nodes[n_id].value == x;
            for (int child : nodes[n_id].children){
                next_frontier[child] = true;
                if (!children_added) children_added = true;
            }

            }
        }
        counter += counters[tid*padding];
    };

    long int seq_time = 0;
    long int par_time = 0;
    while (children_added)
    {
        children_added = false;
        vector<thread> workers;
 
        for (int tid = 0; tid < NumThreads; tid++)
            workers.push_back(thread(workF, tid));
        for (auto& t : workers) t.join();

        swap(frontier, next_frontier);
        fill(next_frontier.begin(), next_frontier.end(), false);
    }
    return counter;
}

#endif

#ifdef ATOMIC_EXPLORED

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads, int chunk)
{
    const int n_nodes = nodes.size();
    atomic<unsigned long> counter = {0};
    auto explored_nodes = new atomic<bool>[n_nodes];
    for (int i =0; i<n_nodes; i++) explored_nodes[i] = false;

    vector<int> frontier{0};
    vector<vector<int>> next_frontiers(NumThreads);
    frontier.reserve(nodes.size() / 2);
    for (auto& nf : next_frontiers) nf.reserve(nodes.size() / NumThreads);

    const int padding = 8;
    vector<int> counters(NumThreads*padding, 0);

    if (chunk < 1) chunk = 32;

    auto workF = [&](int tid)
    {
        counters[tid*padding] = 0;
        for (int f_pos = tid*chunk; f_pos < frontier.size(); f_pos += NumThreads*chunk)
        {
            int my_chunk = min(chunk, int(frontier.size()-f_pos));
            for (int offset=0; offset < my_chunk; offset++){

            int n_id = frontier[f_pos+offset];
            if (explored_nodes[n_id]) continue;
            explored_nodes[n_id] = true;
            counters[tid*padding] += nodes[n_id].value == x;
            next_frontiers[tid].insert(
                next_frontiers[tid].end(),
                nodes[n_id].children.begin(),
                nodes[n_id].children.end()
            );

            }
        }
        // join results to global results
        counter += counters[tid*padding];
    };

    long int seq_time = 0;
    long int par_time = 0;
    while (!frontier.empty())
    {
        // cout << "f size " << frontier.size() << endl;
        vector<thread> workers;

        {
        // utimer par_t1("parallel");
        // my_timer t;   
        for (int tid = 0; tid < NumThreads; tid++)
            workers.push_back(thread(workF, tid));

        for (auto& t : workers) t.join();
        // par_time += t.get_time();
        }

        frontier.clear();

        {
        // utimer t("merging");
        // my_timer t;
        for (auto& partial : next_frontiers){
            frontier.insert(frontier.end(), partial.begin(), partial.end());
            partial.clear();
        }
        // seq_time += t.get_time();
        }

    }

    cout << "seq time is " << seq_time << endl;
    cout << "par time is " << par_time << endl;
    delete explored_nodes;
    return counter;
}

#endif

#ifdef SET

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads)
{
    const int n_nodes = nodes.size();
    atomic<unsigned long> counter = {0};
    vector<unsigned char> explored_nodes(n_nodes, false);

    vector<int> frontier{0};
    vector<vector<int>> next_frontiers(NumThreads);
    frontier.reserve(nodes.size() / 2);
    for (auto& nf : next_frontiers) nf.reserve(nodes.size() / NumThreads);

    set<int> next_frontier;
    SpinLock nf_lock;

    auto workF = [&](int tid)
    {
        int my_counter = 0;
        {
        // utimer t("frontier");
        for (int f_pos = tid; f_pos < frontier.size(); f_pos+=NumThreads)
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
        nf_lock.lock();
        for (int e : next_frontiers[tid])
            next_frontier.insert(e);
        next_frontiers[tid].clear();
        nf_lock.unlock();

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
            workers.push_back(thread(workF, tid));

        for (auto& t : workers) t.join();
        par_time += t.get_time();
        }

        frontier.clear();

        {
        // utimer t("merging");
        my_timer t;
        frontier.clear();
        copy(next_frontier.begin(), next_frontier.end(), back_inserter(frontier));
        next_frontier.clear();
        seq_time += t.get_time();
        }

    }

    cout << "seq time is " << seq_time << endl;
    cout << "par time is " << par_time << endl;
    return counter;
}

#endif