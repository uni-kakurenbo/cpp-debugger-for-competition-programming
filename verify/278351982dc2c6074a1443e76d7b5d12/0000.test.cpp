/*
 * @uni_kakurenbo
 * https://github.com/uni-kakurenbo/competitive-programming-workspace
 *
 * CC0 1.0  http://creativecommons.org/publicdomain/zero/1.0/deed.ja
 */
/* #language C++ GCC */

#define PROBLEM "https://judge.yosupo.jp/problem/associative_array"

#include "sneaky/enforce_int128_enable.hpp"

#include <iostream>
#include "snippet/aliases.hpp"
#include "snippet/fast_io.hpp"
#include "snippet/iterations.hpp"
#include "adapter/io.hpp"
#include "numeric/modular/modint.hpp"
#include "adapter/vector.hpp"
#include "data_structure/dynamic_sequence.hpp"
#include "data_structure/treap.hpp"
#include "action/null.hpp"

using mint = lib::modint998244353;

signed main() {
    lib::dynamic_sequence<lib::actions::null<lib::i64>> data;

    int q; input >> q;
    REP(q) {
        int t; input >> t;
        if(t == 0) {
            lib::i64 i, v; input >> i >> v;
            data[i] = v;
            debug(data, data.size());
        }
        if(t == 1) {
            lib::i64 i; input >> i;
            print(data[i]);
        }
    }
}
