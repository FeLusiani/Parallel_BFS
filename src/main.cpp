#include <iostream>
using namespace std;

#include "build_graph.hpp"
#include <Parallel_BFS/BFS_seq.hpp>
#include <Parallel_BFS/BFS_par_th.hpp>
#include <Parallel_BFS/BFS_par_ff.hpp>
#include "utimer.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <math.h>

void run_tests(const Graph& graph, int to_find, int nw, int chunk){
	int counter = 0;
	for (auto const &n : graph.nodes_array)
		counter += n.value == to_find;

	const int repeat = 5;
	cout << "Running each BFS for " << repeat << " times.\n";
	cout << "N. workers = " << nw << "\n\n";

	auto run_search = [&](auto search_f, string name){
		int occ = 0;
		{
			cout << "Exec times for " << name << " = ";
			for (int i = 0; i < repeat; i++){
				my_timer t;
				occ = search_f();
				cout << round(t.get_time()/double(repeat)) << ' ';
			}
			cout << endl;
		}
		cerr << "N. of " << to_find << ": " << occ << "- true is " << counter << "\n\n";
	};

	run_search(
		[&](){return BFS_par_th(to_find, graph.nodes_array, nw, chunk);},
		string("BFS_par_th")
	);

	run_search(
		[&](){return BFS_par_th2(to_find, graph.nodes_array, nw, chunk);},
		string("BFS_par_th2")
	);

	run_search(
		[&](){return BFS_par_ff(to_find, graph.nodes_array, nw, chunk);},
		string("BFS_par_ff")
	);

	run_search(
		[&](){return BFS_par_ff2(to_find, graph.nodes_array, nw, chunk);},
		string("BFS_par_ff2")
	);

	run_search(
		[&](){return BFS_seq(to_find, graph.nodes_array);},
		string("BFS_seq")
	);

	cout << "\n\n";
}

int main(int argc, char *argv[])
{
	vector<string> args(argc);
	for (int i = 0; i < argc; i++) args[i] = string(argv[i]);
	cout << "\n\n";

	if (args[1] == string("build_graph")){
		if (argc != 5){
			cout << " - Usage: build_graph N_NODES OUT_BOUND FILEPATH\n";
			return 0;
		}
		int n_nodes = stoi(args[2]);
		int out_bound = stoi(args[3]);

		srand(time(NULL));
		Graph graph(n_nodes, out_bound, 10);
		utimer t("building graph");
		graph.build();
		int extra_connections = n_nodes;
		graph.add_connections(n_nodes);

		ofstream myfile_out;
		myfile_out.open(args[4]);
		myfile_out << graph;
		myfile_out << endl;
		myfile_out.close();
		return 0;
	}

	if (argc != 4 && argc != 6){
		cout << " - Usage: FILEPATH NW CHUNK [UP_TO_NW STEP]\n";
		return 0;
	}
	int to_find = 0;
	int nw = stoi(args[2]);
	int chunk = stoi(args[3]);
	int up_to_nw = argc == 4 ? nw : stoi(args[4]);
	int nw_step = argc == 4 ? 1 : stoi(args[5]); 

	Graph graph(-1,-1,-1);
	ifstream myfile_in;
	myfile_in.open(args[1]);
	myfile_in >> graph;
	myfile_in.close();

	for (int i = nw; i <= up_to_nw; i+=nw_step)
		run_tests(graph, to_find, i, chunk);
}