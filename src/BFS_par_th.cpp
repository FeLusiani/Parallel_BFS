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

#define ARRAY

#ifdef ARRAY

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads)
{
    const int n_nodes = nodes.size();
    atomic<unsigned long> counter = {0};
    vector<unsigned char> explored_nodes(n_nodes, false);

    vector<unsigned char> frontier(n_nodes, false);
    frontier[0] = true;
    vector<unsigned char> next_frontier(n_nodes, false);
    bool children_added = true;

    auto workF = [&](int tid)
    {
        int my_counter = 0;
        for (int n_id = tid; n_id < n_nodes; n_id += NumThreads)
        {
            if (!frontier[n_id] || explored_nodes[n_id]) continue;
            explored_nodes[n_id] = true;
            my_counter += nodes[n_id].value == x;
            for (int child : nodes[n_id].children){
                children_added = true;
                next_frontier[child] = true;
            }
        }
        // join results to global results
        counter += my_counter;
    };

    auto workF2 = [&](int tid)
    {
        int to_process = n_nodes / NumThreads;
        int start = to_process * tid;
        int end = tid < (NumThreads - 1) ? start + to_process : n_nodes;
        
        int my_counter = 0;
        for (int n_id = start; n_id < end; n_id++)
        {
            if (!frontier[n_id]) continue;
            if (explored_nodes[n_id]) continue;
            explored_nodes[n_id] = true;
            my_counter += nodes[n_id].value == x;
            for (int child : nodes[n_id].children){
                children_added = true;
                next_frontier[child] = true;
            }
        }
        // join results to global results
        counter += my_counter;
    };

    long int seq_time = 0;
    long int par_time = 0;
    while (children_added)
    {
        children_added = false;
        // cout << "f size " << accumulate(frontier.begin(), frontier.end(), 0) << endl;
        // cout << "explored " << accumulate(explored_nodes.begin(), explored_nodes.end(), 0) << endl;
        vector<thread> workers;

        {
        utimer par_t1("parallel");
        my_timer t;   
        for (int tid = 0; tid < NumThreads; tid++)
            workers.push_back(thread(workF, tid));

        for (auto& t : workers) t.join();
        par_time += t.get_time();
        }

        {
        // utimer t("merging");
        my_timer t;
        swap(frontier, next_frontier);
        fill(next_frontier.begin(), next_frontier.end(), false);
        seq_time += t.get_time();
        }
    }

    cout << "seq time is " << seq_time << endl;
    cout << "par time is " << par_time << endl;

    return counter;
}

#endif

#ifdef ATOMIC_EXPLORED

int BFS_par_th(int x, const vector<Node> &nodes, int NumThreads)
{
    const int n_nodes = nodes.size();
    atomic<unsigned long> counter = {0};
    auto explored_nodes = new atomic<bool>[n_nodes];
    for (int i =0; i<n_nodes; i++) explored_nodes[i] = false;

    vector<int> frontier{0};
    vector<vector<int>> next_frontiers(NumThreads);
    frontier.reserve(nodes.size() / 2);
    for (auto& nf : next_frontiers) nf.reserve(nodes.size() / NumThreads);

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
        for (auto& partial : next_frontiers){
            frontier.insert(frontier.end(), partial.begin(), partial.end());
            partial.clear();
        }
        seq_time += t.get_time();
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