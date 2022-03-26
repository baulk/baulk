//////////////////////
#ifndef BELA_PHMAP_HPP
#define BELA_PHMAP_HPP
#include "__phmap/phmap.h"

namespace bela {
using phmap::flat_hash_map;
using phmap::flat_hash_set;
using phmap::node_hash_map;
using phmap::node_hash_set;
using phmap::parallel_flat_hash_map;
using phmap::parallel_flat_hash_set;
using phmap::parallel_node_hash_map;
using phmap::parallel_node_hash_set;
} // namespace bela

#endif