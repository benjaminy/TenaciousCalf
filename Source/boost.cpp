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
#include <map>


typedef boost::adjacency_list<> Graph;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, Graph> ERGen;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
typedef boost::graph_traits<Graph> GraphTraits;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;

uint32_t size_walk = 2000000;
uint32_t *id_array = (uint32_t *)malloc(sizeof(uint32_t)*size_walk);
uint32_t node_count = 0;
long long n_count = 2000000;
Vertex vt;

int random_walk(Vertex vt, IndexMap index, Graph g) {
    uint32_t *wa = (uint32_t *)malloc(sizeof(uint32_t) * size_walk);
    for (int i=0;i<size_walk;i++) {
        boost::graph_traits<Graph>::out_edge_iterator out_i, out_end;
        GraphTraits::edge_descriptor e;
        int e_degree = out_degree(vt, g);
        while (e_degree == 0) {
            int r = random_int(0, n_count-1);
            std::pair<vertex_iter, vertex_iter> vp;
            vp = vertices(g);
            vp.first = vp.first + r;
            vt = *vp.first;
            e_degree = out_degree(vt, g);
        }
        int random_edge = random_int(0, e_degree-1);
        boost::tie(out_i, out_end) = out_edges(vt, g);
        out_i = out_i + random_edge;
        e = *out_i;
        vt = target(e, g);
        wa[i]=index[vt];
    }
    for (int i=0;i<size_walk;i++) {
        std::cout << wa[i] << "\n";
    }
    return 1;
}
int random_walk_c(val_t vt, cow_trie_p g) {
    uint32_t *wa = (uint32_t *)malloc(sizeof(uint32_t) * size_walk);
    for (int i=0;i<size_walk;i++) {
        uint32_t e_degree = vt._.node.succ_ct;
        while (e_degree == 0) {
            int random_vertex = random_int(0, n_count-1);
            cow_trie_lookup(g, random_vertex, &vt);
            e_degree = vt._.node.succ_ct;
        }
        uint32_t random_edge = random_int(0, e_degree-1);
        uint32_t key = vt._.node.succs[random_edge];
        cow_trie_lookup(g, key, &vt);
        wa[i] = vt._.edge.succ;
        cow_trie_lookup(g, vt._.edge.succ, &vt);
    }
    for (int i=0;i<size_walk;i++) {
        std::cout << wa[i] << "\n";
    }
    return 1;
}

int main() {
  GraphTraits::out_edge_iterator out_i, out_end;
  GraphTraits::edge_descriptor e;
  boost::graph_traits<Graph>::adjacency_iterator ai;
  boost::graph_traits<Graph>::adjacency_iterator ai_end;
  boost::minstd_rand gen;
  std::map<uint64_t, uint32_t> m;
  uint64_t *v_array = (uint64_t *)malloc(sizeof(uint64_t)*(n_count + n_count*3));
  // Create graph with 100 nodes and edges with probability 0.05
  double prob = n_count*n_count;
  prob = n_count*3/prob;
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
    uint32_t deg = out_degree(v, g);
    uint32_t *succs = new uint32_t[deg];
    uint32_t *preds = new uint32_t[0];
    val_t my_val = cow_trie_create_val(0, 0, 0, deg, preds, succs, 0, 0);
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
    m[index[source(*ei, g)] << 32 | index[target(*ei, g)]] = id;
    ++id;
  }
  std::cout << "id is\n\n" << id << "\n\n";
  std::cout << "done edge iterating\n\n";
  std::cout << "putting in successors\n\n";
  int p = 0;
  for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
    Vertex v = *vp.first;
    val_t my_val;
    cow_trie_lookup(my_map, p, &my_val);
    boost::tie(out_i, out_end) = out_edges(v, g);
    int eindex = 0;
    for (out_i; out_i != out_end; ++out_i) {
      e = *out_i;
      Vertex src = source(e, g), targ = target(e, g);
      uint32_t id = m[index[src] << 32 | index[targ]];
      my_val._.node.succs[eindex] = id;
      ++eindex;
    }
    ++p;
  }
  vp = vertices(g);
  Vertex vt = *vp.first;
  std::cout << "begin creating walk\n\n";
  val_t v;
  cow_trie_lookup(my_map, 0, &v);
  random_walk_c(v, my_map);
  //random_walk(vt, index, g);
}
