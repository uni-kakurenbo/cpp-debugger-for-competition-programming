/*
 * @uni_kakurenbo
 * https://github.com/uni-kakurenbo/competitive-programming-workspace
 *
 * CC0 1.0  http://creativecommons.org/publicdomain/zero/1.0/deed.ja
 */
/* #language C++ 20 GCC */

#define PROBLEM "https://judge.yosupo.jp/problem/range_affine_range_sum"

#include <iostream>
#include "snippet/aliases.hpp"
#include "snippet/fast_io.hpp"
#include "snippet/iterations.hpp"
#include "numeric/modular/modint.hpp"
#include "adaptor/io.hpp"
#include "data_structure/actable_dynamic_sequence.hpp"
#include "data_structure/red_black_tree.hpp"
#include "action/range_affine_range_sum.hpp"

signed main() {
    int n, q; std::cin >> n >> q;
    std::vector<lib::modint998244353> a(n); input >> a;

    lib::actable_dynamic_sequence<lib::actions::range_affine_range_sum<lib::modint998244353>, lib::red_black_tree_context<lib::i32>> data(a);

    LOOP(q) {
        int t; std::cin >> t;
        if(t == 0) {
            int l, r, b, c; std::cin >> l >> r >> b >> c;
            data.apply(l, r, { b, c });
        }
        if(t == 1) {
            int l, r; std::cin >> l >> r;
            print(data.fold(l, r));
        }
    }
}
