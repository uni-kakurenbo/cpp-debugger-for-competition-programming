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
#include "adaptor/io.hpp"
#include "numeric/modular/modint.hpp"
#include "adaptor/vector.hpp"
#include "data_structure/dynamic_sequence.hpp"
#include "data_structure/treap.hpp"
#include "action/null.hpp"

using mint = uni::modint998244353;

signed main() {
    uni::dynamic_sequence<uni::i64> data;

    uni::i32 q; input >> q;
    REP(q) {
        uni::i32 t; input >> t;
        if(t == 0) {
            uni::i64 i, v; input >> i >> v;
            data[i] = v;
            debug(data, data.size());
        }
        if(t == 1) {
            uni::i64 i; input >> i;
            print(data[i]);
        }
    }
}
