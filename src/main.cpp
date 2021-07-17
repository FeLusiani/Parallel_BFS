#include <iostream>
using namespace std;

#include <ff/ff.hpp>
#include "build_graph.hpp"
#include <Parallel_BFS/BFS_seq.hpp>
#include <Parallel_BFS/BFS_par_th.hpp>
#include "utimer.hpp"
#include <chrono>
#include <ctime>

int main(int argc, char *argv[])
{
	vector<string> args(argc);
	for (int i = 0; i < argc; i++)
	{
		args[i] = string(argv[i]);
	}

	int n_nodes = stoi(args[1]);
	int out_bound = stoi(args[2]);
	int to_find = stoi(args[3]);
	int nw = stoi(args[4]);

	int extra_connections = n_nodes;

	srand(time(NULL));

	Graph graph(n_nodes, out_bound, 10);
	{
		utimer t("building graph");
		graph.build();
		graph.add_connections(extra_connections);
	}
	cerr << "Created graph: nodes=" << n_nodes << " out_bound=" << out_bound << "\n\n";

	int counter = 0;
	for (auto const &n : graph.nodes_array)
		counter += n.value == to_find;

	int occ = 0;
	{
		utimer t("BFS_seq");
		occ = BFS_seq(to_find, graph.nodes_array);
	}
	cerr << "N. of " << to_find << ": " << occ << "- true is " << counter << "\n\n";


	occ = 0;
	{
		utimer t("BFS_par");
		occ = BFS_par_th(to_find, graph.nodes_array, nw);
	}
	cerr << "N. of " << to_find << ": " << occ << "- true is " << counter << "\n\n";
}
