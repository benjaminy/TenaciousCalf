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
extern "C" {
    #include "c_t_2.h"
    #include "tester.h"
}
#include <map>
#include <sys/time.h>

#ifdef USE_UINT64_T
typedef struct two_ids_thing two_ids_thing, *two_ids_thing_p;

struct two_ids_thing {
    uint64_t source;
    uint64_t target;
    bool operator<(two_ids_thing other) const {
        return source > other.source;
    }
};
#endif


typedef boost::adjacency_list<> Graph;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, Graph> ERGen;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;
typedef boost::graph_traits<Graph> GraphTraits;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;

uint32_t size_walk = 10;
int_type *id_array = (int_type *)malloc(sizeof(id_array[0])*size_walk);
uint32_t node_count = 0;
long long n_count = 50000;
Vertex vt;

float random_walk(Vertex vt, IndexMap index, Graph g, std::map<int_type, int_type> self_map) {
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);
    for (int i=0;i<size_walk;i++) {
        boost::graph_traits<Graph>::out_edge_iterator out_i, out_end;
        GraphTraits::edge_descriptor e;
        int e_degree = out_degree(vt, g);
        while (e_degree == 0) {
            int_type r = self_map[random_int(0, n_count-1)];
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
    }
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);
    printf("Time for theirs: %ld.%06d\n", tval_result.tv_sec, tval_result.tv_usec);
    float time_elapsed = ((long int)tval_result.tv_usec + (long int)tval_result.tv_sec/1000000)/1000000.0;
    return time_elapsed;
}
float random_walk_c(val_t vt, cow_trie_p g, std::map<int_type, int_type> map) {
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL);
    for (int i=0;i<size_walk;i++) {
        uint32_t e_degree = vt._.node.succ_ct;
        while (e_degree == 0) {
            int_type random_vertex = map[random_int(0, n_count-1)];
            cow_trie_lookup(g, random_vertex, &vt);
            e_degree = vt._.node.succ_ct;
        }
        int random_edge = random_int(0, e_degree-1);
        int_type key = edge_list_values(vt._.node.succs)[random_edge];
        cow_trie_lookup(g, key, &vt);
        cow_trie_lookup(g, vt._.edge.succ, &vt);
    }
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);
    printf("Time for ours: %ld.%06d\n", tval_result.tv_sec, tval_result.tv_usec);
    float time_elapsed = ((long int)tval_result.tv_usec + (long int)tval_result.tv_sec/1000000)/1000000.0;
    return time_elapsed;
}

size_t traverse(cow_trie_p map) {
    size_t count = 0;
    count += map->size;
    for (int i=0;i<count_one_bits(map->child_bitmap);i++) {
        count += traverse(cow_trie_children(map)[i]);
    }
    return count;
}

int main() {
    GraphTraits::out_edge_iterator out_i, out_end;
    GraphTraits::edge_descriptor e;
    boost::graph_traits<Graph>::adjacency_iterator ai;
    boost::graph_traits<Graph>::adjacency_iterator ai_end;
    boost::minstd_rand gen;
    
    #ifdef USE_UINT64_T
    std::map<two_ids_thing, uint32_t> m;
    #else
    std::map<uint64_t, uint32_t> m;
    #endif
    std::map<int_type, int_type> id_map;
    std::map<int_type, int_type> self_map;

    size_t count = 0;

    uint64_t *v_array = (uint64_t *)malloc(sizeof(uint64_t)*(n_count + n_count*3));
    // Create graph with 100 nodes and edges with probability 0.05
    double prob = n_count*n_count;
    prob = n_count*3/prob;
    Graph g(ERGen(gen, n_count, prob), ERGen(), n_count);

    std::cout << "done generating\n\n";

    IndexMap index = get(boost::vertex_index, g);
    cow_trie_p my_map = cow_trie_alloc(0, 0);
    my_map->ref_count = 1;
    my_map->child_bitmap = 0;
    my_map->value_bitmap = 0;
    int_type id = 0;

    std::cout << "beginning vertex iteration\n\n";
    std::pair<vertex_iter, vertex_iter> vp;
    for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
        Vertex v = *vp.first;
        boost::tie(out_i, out_end) = out_edges(v, g);
        uint32_t deg = out_degree(v, g);
        int_type *succs = new int_type[deg];
        val_p my_val = cow_trie_create_val(0, deg, succs, 0, 0);
        int_type my_id = cow_trie_insert(my_map, *my_val, &my_map);
        free(my_val);
        id_map[id] = my_id;
        self_map[id] = id;
        ++id;
    }

    std::cout << "vertex iteration done, beginning edge iteration\n\n";
    boost::graph_traits<Graph>::edge_iterator ei, ei_end;
    boost::tie(ei, ei_end) = edges(g);
    std::cout << "id before is" << id << "\n\n"; 
    for (ei; ei != ei_end; ++ei) {
        val_p my_val = cow_trie_create_val(1, 0, 0, id_map[index[source(*ei, g)]], id_map[index[target(*ei, g)]]);
        int_type my_id = cow_trie_insert(my_map, *my_val, &my_map);
        free(my_val);
        #ifdef USE_UINT64_T
        two_ids_thing t;
        t.source = index[source(*ei, g)];
        t.target = index[target(*ei, g)];
        m[t] = my_id;
        #else
        m[index[source(*ei, g)] << 32 | index[target(*ei, g)]] = my_id;
        #endif
        id_map[id] = my_id;
        self_map[id] = id;
        ++id;
    }
    std::cout << "id is\n\n" << id << "\n\n";
    std::cout << "done edge iterating\n\n";
    std::cout << "putting in successors\n\n";
    int p = 0;
    for (vp = vertices(g); vp.first != vp.second; ++vp.first) {
        Vertex v = *vp.first;
        val_t my_val;
        cow_trie_lookup(my_map, id_map[p], &my_val);
        int_type *succs = edge_list_values(my_val._.node.succs);
        boost::tie(out_i, out_end) = out_edges(v, g);
        int eindex = 0;
        for (out_i; out_i != out_end; ++out_i) {
            e = *out_i;
            Vertex src = source(e, g), targ = target(e, g);
            #ifdef USE_UINT64_T
            two_ids_thing t;
            t.source = index[src];
            t.target = index[targ];
            #else
            uint64_t t = index[src] << 32 | index[targ];
            #endif
            int_type id = m[t];
            succs[eindex] = id;
            ++eindex;
        }
        p++;
    }
    val_t v;
    cow_trie_lookup(my_map, 0, &v);
    Vertex vtest = *vertices(g).first;
    std::cout << "beginning random traversal\n\n";
    float t2 = random_walk_c(v, my_map, id_map);
    float t1 = random_walk(vtest, index, g, self_map);
    std::cout << "end random traversal\n\n";
}
