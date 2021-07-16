#include "build_graph.hpp"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <set>
#include <algorithm>

#include <iterator> // needed for std::ostram_iterator

Node::Node(int id, int max_val) : id{id}
{
	value = rand() % (max_val+1);
};

ostream &operator<<(ostream &out, const Node &node)
{
	out << "<" << node.id << "," << node.value << ">";
	out << node.children;
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
	if (!full())
	{
		int node_id = first_free++;
		nodes_array[node_id] = Node(node_id, max_val);
		return node_id;
	}
	else
	{
		return -1;
	}
}

void Graph::build()
{
	nodes_array = vector<Node>(n_nodes, Node(-1, max_val));

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

void build(string filepath)
{
	int S_LEN;
	cin >> S_LEN;
	string old_string = to_string(45);
	string new_string = string(S_LEN - old_string.length(), '0') + old_string;
	cout << new_string << '\n';

	ofstream myfile;
	myfile.open(filepath);
	myfile << "Writing this to a file.\n";
	myfile.close();
}