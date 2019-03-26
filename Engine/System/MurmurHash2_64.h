#pragma once

typedef unsigned __int64 uint64_t;

// 64-bit hash for 64-bit platforms
uint64_t MurmurHash64A( const void * key, int len, unsigned int seed );

// 64-bit hash for 32-bit platforms
uint64_t MurmurHash64B( const void * key, int len, unsigned int seed );
