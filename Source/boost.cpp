#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <stdint.h>
#include "cow_trie.h"


typedef boost::adjacency_list<> Graph;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, Graph> ERGen;

int main() {
  boost::minstd_rand gen;
  int n_count = 46340;
  double prob = n_count*n_count;
  prob = n_count*4/prob;
  // Create graph with 100 nodes and edges with probability 0.05
  Graph g(ERGen(gen, n_count, prob), ERGen(), n_count);
  std::cout << "done generating\n\n";
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
  typedef boost::graph_traits<Graph> GraphTraits;
  IndexMap index = get(boost::vertex_index, g);
  typename GraphTraits::out_edge_iterator out_i, out_end;
  typename GraphTraits::edge_descriptor e;
  typename boost::graph_traits<Graph>::adjacency_iterator ai;
  typename boost::graph_traits<Graph>::adjacency_iterator ai_end;
  cow_trie_p my_map = cow_trie_alloc(0, 0);
  uint32_t id = 0;
  //for each vertex
  std::cout << "beginning vertex iteration\n\n";
  typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;
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
  std::cout << "begin traversing\n\n";
  int size_walk = 20000;
  int i_index = 0;
  for (int i=0;i<size_walk;i++) {
    boost::tie(ai, ai_end) = adjacent_vertices(vt, g);
    for (ai; ai != ai_end; ++ai) {
      int i_index_new = index[vt];
      vp.first = vp.first + (i_index_new - i_index);
      vt = *vp.first;
    }
  }
  i_index = 0;
  val_t v;
  std::cout << "done traversing\n\n";
  std::cout << "begin traversing cow_trie\n\n";
  for (int i=0;i<size_walk;i++) {
    cow_trie_lookup(my_map, i_index, &v);
    i_index = v._.node.succs[0];
  }
  std::cout << "done traversing cow_trie\n\n";
  
}
