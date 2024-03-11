#pragma once


#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>
#include <valarray>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <limits>
#include <type_traits>
#include <ranges>
#include <concepts>
#include <bit>


#include "snippet/iterations.hpp"

#include "internal/dev_env.hpp"
#include "internal/types.hpp"
#include "internal/iterator.hpp"
#include "internal/range_reference.hpp"

#include "global/constants.hpp"

#include "iterable/compressed.hpp"

#include "data_structure/bit_vector.hpp"

#include "numeric/bit.hpp"
#include "numeric/arithmetic.hpp"


namespace lib {

namespace internal {

namespace wavelet_matrix_impl {


// Thanks to: https://github.com/NyaanNyaan/library/blob/master/data-structure-2d/wavelet-matrix.hpp
template<std::unsigned_integral T, class DictType>
    requires std::same_as<T, typename DictType::key_type>
struct base {
    using size_type = internal::size_t;
    using value_type = T;

  private:
    size_type _n, _bits;
    std::vector<bit_vector> _index;
    std::vector<std::vector<value_type>> _sum;
    DictType _first_pos;

    value_type _max = 0;

  public:
    base() = default;

    template<std::ranges::input_range R>
    explicit base(R&& range) noexcept(NO_EXCEPT) : base(ALL(range)) {}

    template<std::input_iterator I, std::sentinel_for<I> S>
    base(I first, S last) noexcept(NO_EXCEPT) { this->build(first, last); }

    template<std::convertible_to<value_type> U>
    base(const std::initializer_list<U>& init_list) noexcept(NO_EXCEPT) : base(ALL(init_list)) {}

    inline size_type size() const noexcept(NO_EXCEPT) { return this->_n; }
    inline size_type bits() const noexcept(NO_EXCEPT) { return this->_bits; }

    template<std::ranges::input_range R>
    inline void build(R&& range) noexcept(NO_EXCEPT) { this->build(ALL(range)); }

    template<std::input_iterator I, std::sized_sentinel_for<I> S>
        __attribute__((optimize("O3")))
    void build(I first, S last) noexcept(NO_EXCEPT) {
        this->_n = static_cast<size_type>(std::ranges::distance(first, last));
        this->_max = first == last ? -1 : *std::ranges::max_element(first, last);
        this->_bits = std::bit_width(this->_max + 1);

        this->_index.assign(this->_bits, this->_n);

        std::vector<value_type> bit(first, last), nxt(this->_n);

        this->_sum.assign(this->_bits+1, std::vector<value_type>(this->_n+1));

        {
            size_type i = 0;
            for(auto itr=first; itr!=last; ++i, ++itr) {
                assert(*itr >= 0);
                this->_sum[this->_bits][i+1] = this->_sum[this->_bits][i] + *itr;
            }
        }

        REPD(h, this->_bits) {
            std::vector<size_type> vals;

            for(size_type i = 0; i < this->_n; ++i) {
                if((bit[i] >> h) & 1) this->_index[h].set(i);
            }
            this->_index[h].build();

            std::array<typename std::vector<value_type>::iterator,2> itrs{
                std::ranges::begin(nxt), std::ranges::next(std::ranges::begin(nxt), this->_index[h].zeros())
            };

            for(size_type i=0; i<this->_n; ++i) *itrs[this->_index[h].get(i)]++ = bit[i];

            REP(i, this->_n) this->_sum[h][i+1] = this->_sum[h][i] + nxt[i];

            std::swap(bit, nxt);
        }

        REPD(i, this->_n) this->_first_pos[bit[i]] = i;
    }

  protected:
    inline value_type get(const size_type k) const noexcept(NO_EXCEPT) {
        return this->_sum[this->_bits][k+1] - this->_sum[this->_bits][k];
    }

    size_type select(const value_type& v, const size_type rank) const noexcept(NO_EXCEPT) {
        if(v > this->_max) return this->_n;
        if(not this->_first_pos.contains(v)) return this->_n;

        size_type pos = this->_first_pos.at(v) + rank + 1;
        REP(h, this->_bits) {
            const bool bit = (v >> h) & 1;
            if(bit) pos -= this->_index[h].zeros();
            pos = this->_index[h].select(bit, pos);
        }

        return pos - 1;
    }


    value_type kth_smallest(size_type *const l, size_type *const r, size_type *const k) const noexcept(NO_EXCEPT) {
        value_type val = 0;

        for(size_type h = this->_bits - 1; h >= 0; --h) {
            size_type l0 = this->_index[h].rank0(*l), r0 = this->_index[h].rank0(*r);
            if(*k < r0 - l0) {
                *l = l0, *r = r0;
            }
            else {
                *k -= r0 - l0;
                val |= value_type{1} << h;
                *l += this->_index[h].zeros() - l0;
                *r += this->_index[h].zeros() - r0;
            }
        }

        return val;
    }

    inline value_type kth_smallest(size_type l, size_type r, size_type k) const noexcept(NO_EXCEPT) {
        return this->kth_smallest(&l, &r, &k);
    }

    size_type kth_smallest_index(size_type l, size_type r, size_type k) const noexcept(NO_EXCEPT) {
       const value_type val = this->kth_smallest(&l, &r, &k);

        size_type left = 0;
        REPD(h, this->_bits) {
            const bool bit = (val >> h) & 1;
            left = this->_index[h].rank(bit, left);
            if(bit) left += this->_index[h].zeros();
        }

        return this->select(val, l + k - left);
    }

    inline value_type kth_largest(const size_type l, const size_type r, const size_type k) const noexcept(NO_EXCEPT) {
        return this->kth_smallest(l, r, r-l-k-1);
    }
    inline size_type kth_largest_index(const size_type l, const size_type r, const size_type k) const noexcept(NO_EXCEPT) {
        return this->kth_smallest_index(l, r, r-l-k-1);
    }


    inline std::pair<size_type,size_type> succ0(const size_type l, const size_type r, const size_type h) const noexcept(NO_EXCEPT) {
        return std::make_pair(this->_index[h].rank0(l), this->_index[h].rank0(r));
    }
    inline std::pair<size_type,size_type> succ1(const size_type l, const size_type r, const size_type h) const noexcept(NO_EXCEPT) {
        const size_type l0 = this->_index[h].rank0(l);
        const size_type r0 = this->_index[h].rank0(r);
        const size_type vals = this->_index[h].zeros();
        return std::make_pair(l + vals - l0, r + vals - r0);
    }


    value_type sum_in_range(
        const size_type l, const size_type r,
        const value_type& x, const value_type& y,
        const value_type& cur, const size_type bit
    ) const noexcept(NO_EXCEPT)
    {
        if(l == r) return 0;

        if(bit == -1) {
            if(x <= cur and cur <= y) return cur * (r - l);
            return 0;
        }

        const value_type& nxt = (value_type{1} << bit) | cur;
        const value_type& ones = ((value_type{1} << bit) - 1) | nxt;

        if(ones < x or y < cur) return 0;

        if(x <= cur and ones <= y) return this->_sum[bit+1][r] - this->_sum[bit+1][l];

        const size_type l0 = this->_index[bit].rank0(l), r0 = this->_index[bit].rank0(r);
        const size_type l1 = l - l0, r1 = r - r0;

        return this->sum_in_range(l0, r0, x, y, cur, bit - 1) + this->sum_in_range(this->_index[bit].zeros()+l1, this->_index[bit].zeros()+r1, x, y, nxt, bit - 1);
    }

    inline value_type sum_in_range(const size_type l, const size_type r, const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) {
        return this->sum_in_range(l, r, x, y, 0, this->_bits-1);
    }
    inline value_type sum_under(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->sum_in_range(l, r, 0, v-1);
    }
    inline value_type sum_over(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->sum_in_range(l, r, v+1, std::numeric_limits<value_type>::max());
    }
    inline value_type sum_or_under(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->sum_in_range(l, r, 0, v);
    }
    inline value_type sum_or_over(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->sum_in_range(l, r, v, std::numeric_limits<value_type>::max());
    }
    inline value_type sum(const size_type l, const size_type r) const noexcept(NO_EXCEPT) {
        return this->_sum[this->_bits][r] - this->_sum[this->_bits][l];
    }

    size_type count_under(size_type l, size_type r, const value_type& y) const noexcept(NO_EXCEPT) {
        if(y >= (value_type{1} << this->_bits)) return r - l;

        size_type res = 0;
        REPD(h, this->_bits) {
            bool f = (y >> h) & 1;
            size_type l0 = this->_index[h].rank0(l), r0 = this->_index[h].rank0(r);
            if(f) {
                res += r0 - l0;
                l += this->_index[h].zeros() - l0;
                r += this->_index[h].zeros() - r0;
            } else {
                l = l0;
                r = r0;
            }
        }
        return res;
    }

    inline size_type count_or_under(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->count_under(l, r, v+1);
    }
    inline size_type count_or_over(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return r - l - this->count_under(l, r, v);
    }
    inline size_type count_over(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->count_or_over(l, r, v+1);
    }
    inline size_type count_in_range(const size_type l, const size_type r, const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) {
        return this->count_or_under(l, r, y) - this->count_under(l, r, x);
    }
    inline size_type count_equal_to(const size_type l, const size_type r, const value_type& v) const noexcept(NO_EXCEPT) {
        return this->count_in_range(l, r, v, v);
    }

    inline std::optional<value_type> next(const size_type l, const size_type r, const value_type& v, const size_type k) const noexcept(NO_EXCEPT) {
        const size_type rank = this->count_under(l, r, v) + k;
        if(rank < 0 or rank >= r - l) return {};
        return { this->kth_smallest(l, r, rank) };
    }

    inline std::optional<value_type> prev(const size_type l, const size_type r, const value_type& v, const size_type k) const noexcept(NO_EXCEPT) {
        const size_type rank = this->count_over(l, r, v) + k;
        if(rank < 0 or rank >= r - l) return {};
        return this->kth_largest(l, r, rank);
    }
};


} // namespace wavelet_matrix_impl

} // namespace internal


template<std::integral T, template<class...> class DictAbstract = std::unordered_map>
struct compressed_wavelet_matrix;


template<std::integral T, template<class...> class DictAbstract = std::unordered_map>
struct wavelet_matrix : internal::wavelet_matrix_impl::base<std::make_unsigned_t<T>, DictAbstract<std::make_unsigned_t<T>, u32>> {
    using value_type = T;
    using impl_type = std::make_unsigned_t<value_type>;
    using dict_type = DictAbstract<impl_type, u32>;
    using size_type = internal::size_t;

    using compressed = compressed_wavelet_matrix<value_type, DictAbstract>;

  private:
    using base = typename internal::wavelet_matrix_impl::base<impl_type, dict_type>;

  public:
  protected:
    inline size_type _positivize_index(const size_type p) const noexcept(NO_EXCEPT) {
        return p < 0 ? this->size() + p : p;
    }

  public:
    using base::base;

    bool empty() const noexcept(NO_EXCEPT) { return this->size() == 0; }

    inline value_type get(size_type p) const noexcept(NO_EXCEPT) {
        p = this->_positivize_index(p), assert(0 <= p && p < this->size());
        return this->base::get(p);
    }
    inline value_type operator[](const size_type p) const noexcept(NO_EXCEPT) { return this->get(p); }

    inline size_type select(const value_type& v, const size_type p) const noexcept(NO_EXCEPT) {
        return this->base::select(v, p);
    }

    struct iterator;
    struct range_reference;

    template<lib::interval_notation rng = lib::interval_notation::right_open>
    inline range_reference range(const size_type l, const size_type r) const noexcept(NO_EXCEPT) {
        if constexpr(rng == lib::interval_notation::right_open) return range_reference(this, l, r);
        if constexpr(rng == lib::interval_notation::left_open) return range_reference(this, l+1, r+1);
        if constexpr(rng == lib::interval_notation::open) return range_reference(this, l+1, r);
        if constexpr(rng == lib::interval_notation::closed) return range_reference(this, l, r+1);
    }
    inline range_reference range() const noexcept(NO_EXCEPT) { return range_reference(this, 0, this->size()); }
    inline range_reference operator()(const size_type l, const size_type r) const noexcept(NO_EXCEPT) { return range_reference(this, l, r); }

    inline range_reference subseq(const size_type p, const size_type c) const noexcept(NO_EXCEPT) { return range_reference(this, p, p+c); }
    inline range_reference subseq(const size_type p) const noexcept(NO_EXCEPT) { return range_reference(this, p, this->size()); }


    struct range_reference : internal::range_reference<const wavelet_matrix> {
        range_reference(const wavelet_matrix *const super, const size_type l, const size_type r) noexcept(NO_EXCEPT)
          : internal::range_reference<const wavelet_matrix>(super, super->_positivize_index(l), super->_positivize_index(r))
        {
            assert(0 <= this->_begin && this->_begin <= this->_end && this->_end <= this->_super->size());
        }

        inline value_type get(const size_type k) const noexcept(NO_EXCEPT) {
            k = this->_super->_positivize_index(k);
            assert(0 <= k and k < this->size());

            return this->_super->get(this->_begin + k);
        }
        inline value_type operator[](const size_type k) const noexcept(NO_EXCEPT) { return this->get(k); }


        inline value_type kth_smallest(const size_type k) const noexcept(NO_EXCEPT) {
            assert(0 <= k && k < this->size());
            return this->_super->base::kth_smallest(this->_begin, this->_end, k);
        }
        inline auto kth_smallest_element(const size_type k) const noexcept(NO_EXCEPT) {
            if(k == this->size()) return this->end();
            assert(0 <= k && k < this->size());
            return std::next(this->_super->begin(), this->_super->base::kth_smallest_index(this->_begin, this->_end, k));
        }

        inline value_type kth_largest(const size_type k) const noexcept(NO_EXCEPT) {
            assert(0 <= k && k < this->size());
            return this->_super->base::kth_largest(this->_begin, this->_end, k);
        }
        inline auto kth_largest_element(const size_type k) const noexcept(NO_EXCEPT) {
            if(k == this->size()) return this->end();
            assert(0 <= k && k < this->size());
            return std::next(this->_super->begin(), this->_super->base::kth_largest_index(this->_begin, this->_end, k));
        }

        inline value_type min() const noexcept(NO_EXCEPT) { return this->kth_smallest(0); }
        inline value_type max() const noexcept(NO_EXCEPT) { return this->kth_largest(0); }


        // (r-l)/2 th smallest (0-origin)
        inline value_type median() const noexcept(NO_EXCEPT) { return this->kth_smallest(this->size() / 2); }

        inline value_type sum_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->_super->base::sum_in_range(this->_begin, this->_end, x, y); }

        inline value_type sum_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::sum_under(this->_begin, this->_end, v); }
        inline value_type sum_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::sum_over(this->_begin, this->_end, v); }
        inline value_type sum_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::sum_or_under(this->_begin, this->_end, v); }
        inline value_type sum_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::sum_or_over(this->_begin, this->_end, v); }

        inline value_type sum(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->_super->base::sum_in_range(this->_begin, this->_end, x, y); }
        inline value_type sum() const noexcept(NO_EXCEPT) { return this->_super->base::sum(this->_begin, this->_end); }

        template<comparison com>
        inline size_type sum(const value_type& v) const noexcept(NO_EXCEPT) {
            if constexpr(com == comparison::under) return this->sum_under(v);
            if constexpr(com == comparison::over) return this->sum_over(v);
            if constexpr(com == comparison::or_under) return this->sum_or_under(v);
            if constexpr(com == comparison::or_over) return this->sum_or_over(v);
            assert(false);
        }


        inline size_type count_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) {
            return this->_super->base::count_in_range(this->_begin, this->_end, x, y);
        }

        inline size_type count_equal_to(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::count_equal_to(this->_begin, this->_end, v); }
        inline size_type count_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::count_under(this->_begin, this->_end, v); }
        inline size_type count_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::count_over(this->_begin, this->_end, v); }
        inline size_type count_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::count_or_under(this->_begin, this->_end, v); }
        inline size_type count_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_super->base::count_or_over(this->_begin, this->_end, v); }

        template<comparison com = comparison::equal_to>
        inline size_type count(const value_type& v) const noexcept(NO_EXCEPT) {
            if constexpr(com == comparison::equal_to) return this->count_equal_to(v);
            if constexpr(com == comparison::under) return this->count_under(v);
            if constexpr(com == comparison::over) return this->count_over(v);
            if constexpr(com == comparison::or_under) return this->count_or_under(v);
            if constexpr(com == comparison::or_over) return this->count_or_over(v);
            assert(false);
        }

        inline auto next_element(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            return this->kth_smallest_element(std::clamp(this->count_under(v) + k, size_type{ 0 }, this->size()));
        }
        inline auto prev_element(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            return this->kth_largest_element(std::clamp(this->count_over(v) - k, size_type{ 0 }, this->size()));
        }

        inline std::optional<value_type> next(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->_super->base::next(this->_begin, this->_end, v, k); }
        inline std::optional<value_type> prev(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->_super->base::prev(this->_begin, this->_end, v, k); }
    };

    inline value_type kth_smallest(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_smallest(k); }
    inline value_type kth_smallest_element(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_smallest_element(k); }

    inline value_type kth_largest(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_largest(k); }
    inline value_type kth_largest_element(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_largest_element(k); }

    inline value_type min() const noexcept(NO_EXCEPT) { return this->range().kth_smallest(0); }
    inline value_type max() const noexcept(NO_EXCEPT) { return this->range().kth_largest(0); }

    // (size)/2 th smallest (0-origin)
    inline value_type median() const noexcept(NO_EXCEPT) { return this->range().median(); }

    inline value_type sum_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->range().sum_in_range(x, y); }

    inline value_type sum_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().sum_under(v); }
    inline value_type sum_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().sum_over(v); }
    inline value_type sum_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().sum_or_under(v); }
    inline value_type sum_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().sum_or_over(v); }

    inline value_type sum(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->range().sum_in_range(x, y); }
    inline value_type sum() const noexcept(NO_EXCEPT) { return this->range().sum(); }

    template<comparison com>
    inline size_type sum(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().template sum<com>(v); }


    inline size_type count_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->range().count_in_range(x, y); }

    inline size_type count_qeual_to(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_qeual_to(v); }
    inline size_type count_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_under(v); }
    inline size_type count_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_over(v); }
    inline size_type count_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_or_under(v); }
    inline size_type count_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_or_over(v); }

    template<comparison com = comparison::equal_to>
    inline size_type count(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().template count<com>(v); }


    inline auto next_element(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().next_element(v); }
    inline auto prev_element(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().prev_element(v); }

    inline std::optional<value_type> next(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->range().next(v, k); }
    inline std::optional<value_type> prev(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->range().prev(v, k); }

    struct iterator;

  protected:
    using iterator_interface = internal::container_iterator_interface<value_type, const wavelet_matrix, const iterator>;

  public:
    struct iterator : iterator_interface {
        iterator() noexcept = default;
        iterator(const wavelet_matrix *const ref, const size_type pos) noexcept(NO_EXCEPT) : iterator_interface(ref, pos) {}

        inline value_type operator*() const noexcept(NO_EXCEPT) { return this->ref()->get(this->pos()); }
    };

    inline iterator begin() const noexcept(NO_EXCEPT) { return iterator(this, 0); }
    inline iterator end() const noexcept(NO_EXCEPT) { return iterator(this, this->size()); }
};


template<std::integral T, template<class...> class DictAbstract>
struct compressed_wavelet_matrix : protected wavelet_matrix<u32, DictAbstract> {
    using value_type = T;
    using size_type = internal::size_t;

  protected:
    using core = wavelet_matrix<u32, DictAbstract>;
    using compresser = compressed<value_type, valarray<u32>>;

    compresser _comp;

  public:
    using impl_type = typename core::impl_type;

    compressed_wavelet_matrix() = default;

    template<std::ranges::input_range R>
    explicit compressed_wavelet_matrix(R&& range) noexcept(NO_EXCEPT) : compressed_wavelet_matrix(ALL(range)) {}

    template<std::input_iterator I, std::sentinel_for<I> S>
    compressed_wavelet_matrix(I first, S last) noexcept(NO_EXCEPT) { this->build(first, last); }

    template<std::input_iterator I, std::sentinel_for<I> S>
    inline void build(I first, S last) noexcept(NO_EXCEPT) {
        this->_comp = compresser(first, last);
        this->core::build(ALL(this->_comp));
    }

    inline value_type get(const size_type k) const noexcept(NO_EXCEPT) { return this->_comp(this->core::get(k)); }
    inline size_type operator[](const size_type k) const noexcept(NO_EXCEPT) { return this->core::get(k); }


    struct iterator;
    struct range_reference;

    template<lib::interval_notation rng = lib::interval_notation::right_open>
    inline range_reference range(const size_type l, const size_type r) const noexcept(NO_EXCEPT) {
        if constexpr(rng == lib::interval_notation::right_open) return range_reference(this, l, r);
        if constexpr(rng == lib::interval_notation::left_open) return range_reference(this, l+1, r+1);
        if constexpr(rng == lib::interval_notation::open) return range_reference(this, l+1, r);
        if constexpr(rng == lib::interval_notation::closed) return range_reference(this, l, r+1);
    }
    inline range_reference range() const noexcept(NO_EXCEPT) { return range_reference(this, 0, this->size()); }
    inline range_reference operator()(const size_type l, const size_type r) const noexcept(NO_EXCEPT) { return range_reference(this, l, r); }

    inline range_reference subseq(const size_type p, const size_type c) const noexcept(NO_EXCEPT) { return range_reference(this, p, p+c); }
    inline range_reference subseq(const size_type p) const noexcept(NO_EXCEPT) { return range_reference(this, p, this->size()); }


    struct range_reference : internal::range_reference<const compressed_wavelet_matrix> {
        range_reference(const compressed_wavelet_matrix *const super, const size_type l, const size_type r) noexcept(NO_EXCEPT)
          : internal::range_reference<const compressed_wavelet_matrix>(super, super->_positivize_index(l), super->_positivize_index(r))
        {
            assert(0 <= this->_begin && this->_begin <= this->_end && this->_end <= this->_super->size());
        }

      private:
        inline auto _range() const noexcept(NO_EXCEPT) { return this->_super->core::range(this->_begin, this->_end); }

      public:
        inline value_type get(const size_type k) const noexcept(NO_EXCEPT) { return this->_super->_comp(this->_range().get(k)); }
        inline value_type operator[](const size_type k) const noexcept(NO_EXCEPT) { return this->get(k); }


        inline value_type kth_smallest(const size_type k) const noexcept(NO_EXCEPT) { return this->_super->_comp(this->_range().kth_smallest(k)); }
        inline auto kth_smallest_element(const size_type k) const noexcept(NO_EXCEPT) {
            return std::next(this->_super->begin(), std::distance(this->_super->core::begin(), this->_range().kth_smallest_element(k)));
        }

        inline value_type kth_largest(const size_type k) const noexcept(NO_EXCEPT) { return this->_super->_comp(this->_range().kth_largest(k));}
        inline auto kth_largest_element(const size_type k) const noexcept(NO_EXCEPT) {
            return std::next(this->_super->begin(), std::distance(this->_super->core::begin(), this->_range().kth_largest_element(k)));
        }

        inline value_type min() const noexcept(NO_EXCEPT) { return this->kth_smallest(0); }
        inline value_type max() const noexcept(NO_EXCEPT) { return this->kth_largest(0); }


        // (r-l)/2 th smallest (0-origin)
        inline value_type median() const noexcept(NO_EXCEPT) { return this->kth_smallest(this->size() / 2); }


        inline size_type count_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) {
            return this->_range().count_in_range(this->_super->_comp.rank(x), this->_super->_comp.rank2(y));
        }

        inline size_type count_equal_to(const value_type& v) const noexcept(NO_EXCEPT) {
            const auto p = this->_super->_comp.rank(v);
            const auto q = this->_super->_comp.rank2(v);
            if(p != q) return 0;
            return this->_range().count_equal_to(p);
        }

        inline size_type count_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_range().count_under(this->_super->_comp.rank(v)); }
        inline size_type count_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_range().count_over(this->_super->_comp.rank2(v)); }
        inline size_type count_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->_range().count_or_under(this->_super->_comp.rank2(v)); }
        inline size_type count_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->_range().count_or_over(this->_super->_comp.rank(v)); }

        template<comparison com = comparison::equal_to>
        inline size_type count(const value_type& v) const noexcept(NO_EXCEPT) {
            if constexpr(com == comparison::equal_to) return this->count_equal_to(v);
            if constexpr(com == comparison::under) return this->count_under(v);
            if constexpr(com == comparison::over) return this->count_over(v);
            if constexpr(com == comparison::or_under) return this->count_or_under(v);
            if constexpr(com == comparison::or_over) return this->count_or_over(v);
            assert(false);
        }


        inline auto next_element(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            return this->kth_smallest_element(std::clamp(this->_range().count_under(this->_super->_comp.rank(v) + k), 0, this->size()));
        }
        inline auto prev_element(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            return this->kth_largest_element(std::clamp(this->_range().count_over(this->_super->_comp.rank2(v) + k), 0, this->size()));
        }

        inline std::optional<value_type> next(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            const std::optional<size_type> res = this->_range().next(this->_super->_comp.rank(v), k);
            if(res.has_value()) return this->_super->_comp(res.value());
            return {};
        }
        inline std::optional<value_type> prev(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) {
            const std::optional<size_type> res = this->_range().prev(this->_super->_comp.rank2(v), k);
            if(res.has_value()) return this->_super->_comp(res.value());
            return {};
        }
    };

    inline value_type kth_smallest(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_smallest(k); }
    inline auto kth_smallest_element(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_smallest_element(k); }

    inline value_type kth_largest(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_largest(k); }
    inline auto kth_largest_element(const size_type k) const noexcept(NO_EXCEPT) { return this->range().kth_largest_element(k); }

    inline value_type min() const noexcept(NO_EXCEPT) { return this->range().kth_smallest(0); }
    inline value_type max() const noexcept(NO_EXCEPT) { return this->range().kth_largest(0); }

    inline value_type median() const noexcept(NO_EXCEPT) { return this->range().median(); }

    inline size_type count_in_range(const value_type& x, const value_type& y) const noexcept(NO_EXCEPT) { return this->range().count_in_range(x, y); }
    inline size_type count_equal_to(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_equal_to(v); }
    inline size_type count_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_under(v); }
    inline size_type count_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_over(v); }
    inline size_type count_or_under(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_or_under(v); }
    inline size_type count_or_over(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().count_or_over(v); }

    template<comparison com = comparison::equal_to>
    inline size_type count(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().template count<com>(v); }

    inline auto next_element(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().next_element(v); }
    inline auto prev_element(const value_type& v) const noexcept(NO_EXCEPT) { return this->range().prev_element(v); }

    inline std::optional<value_type> next(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->range().next(v, k); }
    inline std::optional<value_type> prev(const value_type& v, const size_type k = 0) const noexcept(NO_EXCEPT) { return this->range().prev(v, k); }

    struct iterator;

  protected:
    using iterator_interface = internal::container_iterator_interface<value_type, const compressed_wavelet_matrix, const iterator>;

  public:
    struct iterator : iterator_interface {
        iterator() noexcept = default;
        iterator(const compressed_wavelet_matrix *const ref, const size_type pos) noexcept(NO_EXCEPT) : iterator_interface(ref, pos) {}

        inline value_type operator*() const noexcept(NO_EXCEPT) { return this->ref()->get(this->pos()); }
        inline value_type operator[](const typename iterator_interface::difference_type count) const noexcept(NO_EXCEPT) { return *(*this + count); }
    };

    inline iterator begin() const noexcept(NO_EXCEPT) { return iterator(this, 0); }
    inline iterator end() const noexcept(NO_EXCEPT) { return iterator(this, this->size()); }
};


template<std::ranges::input_range R>
explicit wavelet_matrix(R&&) -> wavelet_matrix<std::ranges::range_value_t<R>>;

template<std::input_iterator I, std::sentinel_for<I> S>
explicit wavelet_matrix(I, S) -> wavelet_matrix<std::iter_value_t<I>>;


template<std::ranges::input_range R>
explicit compressed_wavelet_matrix(R&&) -> compressed_wavelet_matrix<std::ranges::range_value_t<R>>;

template<std::input_iterator I, std::sentinel_for<I> S>
explicit compressed_wavelet_matrix(I, S) -> compressed_wavelet_matrix<std::iter_value_t<I>>;


} // namespace lib
