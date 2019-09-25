//
// Created by moriya on 7/31/19.
//

#ifndef OBLIVIOUSDICTIONARY_HASHER_H
#define OBLIVIOUSDICTIONARY_HASHER_H

#include <cstdlib>
#include <cinttypes>
#include "xxhash.h"

using namespace std;
class Hasher {
private:

    unsigned long long seed;

public:

    Hasher() {
        seed = 0;
    }

    Hasher(unsigned long long seed) : seed(seed){    }

    Hasher(const Hasher & hash){
        seed = hash.seed;
    }

    size_t operator() (uint64_t const key) const
    {
        unsigned long long const hash = XXH64(&key, 8, seed);
        return hash;
    }

};

#endif //OBLIVIOUSDICTIONARY_HASHER_H
