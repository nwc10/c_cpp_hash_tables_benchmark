// c_cpp_hash_tables_benchmark/blueprints/uint32_uint32_murmur/blueprint.h
// Copyright (c) 2024 Jackson L. Allan.
// Distributed under the MIT License (see the accompanying LICENSE file).

#include <numeric>

#define UINT64_UINT64_MURMUR_ENABLED

struct uint64_uint64_murmur
{
  using key_type = uint64_t;
  using value_type = uint64_t;
  static constexpr const char *label = "64-bit integer key, 64-bit value";
  
  // MurmurHash3’s 64-bit finalizer.
  static uint64_t hash_key( const key_type &key )
  {
    uint64_t result = key;
    result ^= result >> 33;
    result *= 0xff51afd7ed558ccdull;
    result ^= result >> 33;
    result *= 0xc4ceb9fe1a85ec53ull;
    result ^= result >> 33;
    return result;
  }

  static bool cmpr_keys( const key_type &key_1, const key_type &key_2 )
  {
    return key_1 == key_2;
  }

  static void fill_unique_keys( std::vector<key_type> &keys )
  {
    std::iota( keys.begin(), keys.end(), 0 );
  }
};
