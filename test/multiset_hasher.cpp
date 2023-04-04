#include "template.hpp"

#include <cassert>
#include <random>

#include "hash/multiset_hasher.hpp"

signed main() {
    lib::multiset_hasher<unsigned>
        hash0 = { 5, 1, 4, 2, 3, 4 },
        hash1 = { 1, 2, 3, 4, 4, 5 };

    debug(hash0(), hash1());

    std::mt19937 rnd;

    REP(1000) {
        unsigned op = rnd() % 2;
        unsigned e = rnd();
        unsigned count = rnd();
        if(op == 0) hash0.insert(e, count), hash1.insert(e, count);
        if(op == 1) hash0.erase(e, count), hash1.erase(e, count);
        print(hash0(), hash1());
        assert(hash0() == hash1());
    }

    REP(1000) {
        bool op = rnd() % 2;
        unsigned e = rnd();
        unsigned count = rnd() % 1000;

        if(op == 0) {
            hash0.insert(e, count);
            REP(count) hash1.insert(e);
        }
        if(op == 1) {
            hash0.erase(e, count);
            REP(count) hash1.erase(e);
        }
        print(hash0(), hash1());
        assert(hash0() == hash1());
    }
}
