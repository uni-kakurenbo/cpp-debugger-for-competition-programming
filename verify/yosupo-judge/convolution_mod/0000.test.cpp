/*
 * @uni_kakurenbo
 * https://github.com/uni-kakurenbo/competitive-programming-workspace
 *
 * CC0 1.0  http://creativecommons.org/publicdomain/zero/1.0/deed.ja
 */
/* #language C++ GCC */

#define PROBLEM "https://judge.yosupo.jp/problem/convolution_mod"

#include "sneaky/enforce_int128_enable.hpp"

#include "snippet/aliases.hpp"
#include "snippet/fast_io.hpp"
#include "adaptor/io.hpp"
#include "numeric/modular/modint.hpp"
#include "adaptor/vector.hpp"
#include "convolution/sum.hpp"

using mint = uni::modint998244353;

signed main() {
    uni::i32 n, m; input >> n >> m;
    uni::vector<mint> a(n), b(m); input >> a >> b;
    print(uni::convolution<uni::vector<mint>>(a, b));
}
