#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <stdint.h>
#include "cow_trie.h"
#include "tester.h"
#include <fstream>
#include <iostream>

typedef boost::adjacency_list<> Graph;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, Graph> ERGen;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
typedef boost::graph_traits<Graph> GraphTraits;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;

int make_walk(Vertex vt, int *list, int count, int size, Graph g) {
    if (count == size) {
        return 1;
    }
    typename boost::graph_traits<Graph>::out_edge_iterator out_i, out_end;
    typename GraphTraits::edge_descriptor e;
    IndexMap index = get(boost::vertex_index, g);
    int vertexes_tried = 0;
    int e_degree = out_degree(vt, g);
    if (e_degree == 0) {
        return 0;
    }
    while (vertexes_tried < e_degree) {
        int random_edge = random_int(0, e_degree-1);
        boost::tie(out_i, out_end) = out_edges(vt, g);
        out_i = out_i + random_edge;
        e = *out_i;
        Vertex v = target(e, g);
        list[count] = index[v];
        int rc = make_walk(v, list, ++count, size, g);
        if (rc == 1) {
            return 1;
        }
        else {
            ++vertexes_tried;
        }
    }
    return 0;
}

int main() {
  typename GraphTraits::out_edge_iterator out_i, out_end;
  typename GraphTraits::edge_descriptor e;
  typename boost::graph_traits<Graph>::adjacency_iterator ai;
  typename boost::graph_traits<Graph>::adjacency_iterator ai_end;
  boost::minstd_rand gen;
  long long n_count = 10000;
  double prob = n_count*n_count;
  prob = n_count*5/prob;
  // Create graph with 100 nodes and edges with probability 0.05
  Graph g(ERGen(gen, n_count, prob), ERGen(), n_count);
  std::cout << "done generating\n\n";
  IndexMap index = get(boost::vertex_index, g);
  cow_trie_p my_map = cow_trie_alloc(0, 0);
  uint32_t id = 0;
  //for each vertex
  std::cout << "beginning vertex iteration\n\n";
  std::pair<vertex_iter, vertex_iter> vp;
  for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
    Vertex v = *vp.first;
    boost::tie(out_i, out_end) = out_edges(v, g);
    uint32_t *succs = new uint32_t[out_degree(v, g)];
    int eindex = 0;
    for (out_i; out_i != out_end; ++out_i) {
      e = *out_i;
      Vertex src = source(e, g), targ = target(e, g);
      succs[eindex] = index[targ];
      ++eindex;
    }
    uint32_t *preds = new uint32_t[0];
    val_t my_val = cow_trie_create_val(0, 0, 0, eindex, preds, succs, 0, 0);
    cow_trie_insert(my_map, id, my_val, &my_map);
    ++id;
  }
  std::cout << "vertex iteration done, beginning edge iteration\n\n";
  boost::graph_traits<Graph>::edge_iterator ei, ei_end;
  boost::tie(ei, ei_end) = edges(g);
  std::cout << "id before is" << id << "\n\n";
  for (ei; ei != ei_end; ++ei) {
    val_t my_val = cow_trie_create_val(1, 0, 0, 0, 0, 0, index[source(*ei, g)], index[target(*ei, g)]);
    cow_trie_insert(my_map, id, my_val, &my_map);
    ++id;
  }
  std::cout << "id is\n\n" << id << "\n\n";
  std::cout << "done edge iterating\n\n";
  vp = vertices(g);
  Vertex vt = *vp.first;
  std::ifstream infile("nodes.txt");
  int *id_array = (int *)malloc(sizeof(int)*9500);
  int a;
  int count = 0;
  while (infile >> a) {
    id_array[count] = a;
    ++count;
  }
  count=1;
  std::cout << "begin traversal walk\n\n";
  bool t = true;
  /*while (t == true) {
    boost::tie(out_i, out_end) = out_edges(vt, g);
    for (out_i; out_i != out_end; ++out_i) {
      e = *out_i;
      Vertex targ = target(e, g);
      if (index[targ] == id_array[count]) {
        t = false;
        vt = targ;
        ++count;
      }
    }
  }*/
  val_t the_val;
  int t_key = id_array[0];
  for (int i=0;i<9500;i++) {
    cow_trie_lookup(my_map, t_key, &the_val);
    uint32_t *poopoo = the_val._.node.succs;
    for (int j=0;j<the_val._.node.succ_ct;j++) {
      if (poopoo[j] == id_array[count]) {
        ++count;
        t_key = poopoo[j];
      }
    }
  }
}
