#include "../src/build_graph.hpp"
#include <thread>
#include <atomic>
#include <set>
#include "../src/utimer.hpp"
#include <numeric>
#include<mutex>
#include<condition_variable>

template <typename F>
class my_barrier
{
 public:
    my_barrier(int count, F& on_completion)
     : thread_count(count)
     , counter(0)
     , waiting(0)
     , comp_f(on_completion)
    {}

    void wait()
    {
        std::unique_lock<std::mutex> lk(m);
        ++counter;
        ++waiting;
        cv.wait(lk, [&]{return counter >= thread_count;});
        if (waiting == thread_count)
            comp_f();
        --waiting;
        cv.notify_one();
        if(waiting == 0)
           counter = 0;
        lk.unlock();
    }

 private:
      mutex m;
      condition_variable cv;
      int counter;
      int waiting;
      int thread_count;
      F& comp_f;
};


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
    bool work = true;
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

    auto on_completion = [&]() noexcept {
        if (!children_added){
            work = false;
            return;
        }
        children_added = false;
        swap(frontier, next_frontier);
        fill(next_frontier.begin(), next_frontier.end(), false);
    };

    my_barrier barrier(NumThreads, on_completion);

    auto work_wrapper = [&](int tid){
        while (work){
            workF(tid);
            barrier.wait();
        }
    };

    vector<thread> workers;
    for (int tid = 0; tid < NumThreads; tid++)
        workers.push_back(thread(work_wrapper, tid));

    for (auto& t : workers) t.join();
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
        counter += counters[tid*padding];
    };

    bool work = true;
    auto on_completion = [&]() noexcept {
        frontier.clear();
        
        for (auto& partial : next_frontiers){
            frontier.insert(frontier.end(), partial.begin(), partial.end());
            partial.clear();
        }

        if (frontier.empty()){
            work = false;
            return;
        }
    };

    my_barrier barrier(NumThreads, on_completion);

    auto work_wrapper = [&](int tid){
        while (work){
            workF(tid);
            barrier.wait();
        }
    };

    vector<thread> workers;
    for (int tid = 0; tid < NumThreads; tid++)
        workers.push_back(thread(work_wrapper, tid));
    for (auto& t : workers) t.join();

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