#pragma once


#include <queue>
#include <utility>
#include <functional>

#include "snippet/iterations.hpp"

#include "internal/dev_env.hpp"

#include "numeric/limits.hpp"

#include "structure/graph.hpp"
#include "internal/auto_holder.hpp"


template<class Graph>
template<class Cost, class Dist, class Prev>
void lib::internal::graph_impl::mixin<Graph>::distances_with_cost(
    const node_type& s, Dist *const dist, Prev *const prev,
    const node_type& unreachable, const node_type& root
) const noexcept(NO_EXCEPT) {
    using state = std::pair<Cost,node_type>;
    std::priority_queue<state,std::vector<state>,std::greater<state>> que;

    dist->assign(this->size(), lib::numeric_limits<Cost>::arithmetic_infinity());
    if constexpr(not std::is_same_v<Prev,std::nullptr_t>) prev->assign(this->size(), unreachable);

    que.emplace(0, s), dist->operator[](s) = 0;
    if constexpr(not std::is_same_v<Prev,std::nullptr_t>) prev->operator[](s) = root;

    while(not que.empty()) {
        const auto [d, u] = que.top(); que.pop();

        if(dist->operator[](u) < d) continue;

        ITR(e, this->operator[](u)) {
            const node_type v = e.to; const auto cost = e.cost;

            if(dist->operator[](v) <= d + cost) continue;

            dist->operator[](v) = d + cost;
            if constexpr(not std::is_same_v<Prev,std::nullptr_t>) prev->operator[](v) = u;

            que.emplace(dist->operator[](v), v);
        }
    }
}

template<class Graph>
template<class Cost>
lib::auto_holder<typename lib::internal::graph_impl::mixin<Graph>::node_type,Cost> lib::internal::graph_impl::mixin<Graph>::distances_with_cost(const node_type s) const noexcept(NO_EXCEPT) {
    lib::auto_holder<typename lib::internal::graph_impl::mixin<Graph>::node_type,Cost> dist;
    this->distances_with_cost<Cost>(s, &dist);
    return dist;
}
