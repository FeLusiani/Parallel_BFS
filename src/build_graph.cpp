#include "build_graph.hpp"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <algorithm>

#include <sstream>
#include <utility>
#include <iterator>

#include <iterator> // needed for std::ostram_iterator

Node::Node(int id, int max_val) : id{id}
{
	value = rand() % (max_val+1);
};

Node::Node() : id{-1}, value{-1} {};

ostream &operator<<(ostream &out, const Node &node)
{
	out << node.id << ' ' << node.value << '\t';
	for (int child : node.children) out << child << ' ';
	return out;
}

Graph::Graph(int num_nodes, int out_bound, int max_val) : n_nodes{num_nodes},
														  out_bound{out_bound},
														  max_val{max_val},
														  first_free{0},
														  depth{0}
{
}

bool Graph::full() { return first_free >= n_nodes; }

int Graph::new_node()
{
	if (full())  return -1;
	int node_id = first_free++;
	nodes_array[node_id] = Node(node_id, max_val);
	return node_id;
}

void Graph::build()
{
	nodes_array = vector<Node>(n_nodes, Node());

	set<int> parent_nodes{new_node()};
	set<int> children_nodes{};

	while (!full())
	{
		for (auto &parent : parent_nodes)
		{
			int n_children = rand() % (out_bound+1);
			if (n_children == 0){
				children_nodes.insert(parent);
				continue;
			}
			while (!full() && n_children > 0)
			{
				int child = new_node();
				nodes_array[parent].children.push_back(child);
				children_nodes.insert(child);
				n_children--;
			}
		}
		parent_nodes.clear();
		parent_nodes.swap(children_nodes);
		depth += 1;
	}
}

template <typename T>
T const &random_elem(const vector<T> &v)
{
	if (v.empty())
		throw out_of_range("Cannot get random_elem of empty vector");
	return v[rand() % v.size()];
}

int Graph::random_walk(int start_node, int max_depth)
{
	int target_depth = rand() % (max_depth+1);

	int current = start_node;
	for (int curr_depth = 0; curr_depth < target_depth; curr_depth++)
	{
		if (nodes_array[current].children.empty())
			break;
		current = random_elem(nodes_array[current].children);
	}
	return current;
}

void Graph::add_connections(int n)
{
	if (nodes_array.empty()) return;
	for (int i = 0; i < n; i++)
	{
		int rand_parent = random_walk(0, depth);
		int rand_child = random_walk(rand_parent, depth/2);

		auto &v = nodes_array[rand_parent].children;
		if (rand_child != rand_parent && find(v.begin(), v.end(), rand_child) == v.end())
		{
			v.push_back(rand_child);
		}
	}
}

ostream& operator<<(ostream& out, const Graph& graph){
	out << "Graph" << endl << graph.nodes_array.size() << endl;
	for (auto& node : graph.nodes_array){
		out << node << endl;
	}
	return out; 
}


istream& operator>>(istream& in, Graph& graph)
{
	graph.n_nodes = 0;
	graph.nodes_array.clear();

	string tmp;
    in >> tmp;
	if (tmp != string("Graph")){
		cerr << "Error reading Graph object from stream " << endl;
		return in;
	}

	int N;
	in >> N;

	graph.n_nodes = N;
	graph.nodes_array = vector<Node>(N, Node());

	int n_id_in, n_value;
	for (int n_id = 0; n_id < N; n_id ++){
		in >> n_id_in;
		if (n_id_in != n_id){
			cerr << "Error reading Graph object from stream: ";
			cerr << "got n_id " << n_id_in << " at pos " << n_id << endl;
			return in;
		}
		in >> n_value;
		graph.nodes_array[n_id] = Node();
		graph.nodes_array[n_id].id = n_id;
		graph.nodes_array[n_id].value = n_value;
		int child;
		
		string line;
		getline(in, line);
		istringstream this_line(line);
		istream_iterator<int> begin(this_line), end;
		vector<int> children(begin, end);
		graph.nodes_array[n_id].children = children;
	}
    return in;
}