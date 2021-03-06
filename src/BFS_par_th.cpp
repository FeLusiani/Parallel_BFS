#include "../src/build_graph.hpp"
#include <thread>
#include <atomic>
#include <set>
#include "../src/utimer.hpp"
#include <numeric>
#include<mutex>
#include<condition_variable>
#include <functional>


class Barrier
{
 public:
    Barrier(int count, const function<void()>& on_completion)
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
      function<void()> comp_f;
};

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


    bool work = true;
    auto on_completion = [&]() noexcept {
        if (!children_added){
            work = false;
            return;
        }
        children_added = false;
        swap(frontier, next_frontier);
        fill(next_frontier.begin(), next_frontier.end(), false);
    };

    Barrier barrier(NumThreads, on_completion);

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


int BFS_par_th2(int x, const vector<Node> &nodes, int NumThreads, int chunk)
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
            if (explored_nodes[n_id].exchange(true)) continue;
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

    Barrier barrier(NumThreads, on_completion);

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