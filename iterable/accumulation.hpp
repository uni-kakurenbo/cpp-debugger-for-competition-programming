#pragma once


#include <cassert>
#include <iterator>
#include <vector>
#include <functional>
#include <numeric>

#include "internal/dev_env.hpp"
#include "internal/types.hpp"
#include "internal/type_traits.hpp"

#include "adapter/valarray.hpp"


namespace lib {


template<class T, class container = valarray<T>>
struct accumulation : container {
    using size_type = internal::size_t;

  protected:
    inline size_type _positivize_index(const size_type x) const noexcept(NO_EXCEPT) {
        return x < 0 ? std::size(*this) + x : x;
    }

  public:
    accumulation() noexcept(NO_EXCEPT) {}

    template<class I, class Operator = std::plus<T>>
    accumulation(const I first, const I last, const T head = T{}, const Operator op = std::plus<T>{}) noexcept(NO_EXCEPT) {
        this->resize(std::distance(first, last) + 1);
        std::exclusive_scan(first, last, std::begin(*this), head, op);
        const auto back = std::prev(std::end(*this));
        *back = op(*std::prev(back), *std::prev(last));
    }

    template<class Operaotr = std::minus<T>>
    inline T operator()(size_type left, size_type right, Operaotr op = std::minus<T>{}) const noexcept(NO_EXCEPT) {
        left = _positivize_index(left), right = _positivize_index(right);
        assert(0 <= left and left <= right and right < (size_type)std::size(*this));
        return op((*this)[right], (*this)[left]);
    }
};

template<class I>
explicit accumulation(const I, const I) -> accumulation<typename std::iterator_traits<I>::value_type>;


template<class T, class container = valarray<valarray<T>>, class Operator = std::plus<T>>
struct accumulation_2d : container {
    using size_type = internal::size_t;

  protected:
    inline size_type _positivize_index(const size_type x) const noexcept(NO_EXCEPT) {
        return x < 0 ? std::size(*this) + x : x;
    }

    Operator _op;

  public:
    explicit accumulation_2d() noexcept(NO_EXCEPT) {}

    template<class I>
    explicit accumulation_2d(const I first, const I last, const T head = T{}, const Operator op = std::plus<T>{}) noexcept(NO_EXCEPT) : _op(op) {
        const size_type h = static_cast<size_type>(std::distance(first, last));
        const size_type w = static_cast<size_type>(std::distance(std::begin(*first), std::end(*first)));
        {
            auto row = first;
            this->assign(h+1, head);
            (*this)[0].assign(w+1, head);
            REP(i, h) {
                assert(w == std::distance(std::begin(*row), std::end(*row)));
                (*this)[i+1].assign(w+1, head);
                REP(j, w) (*this)[i+1][j+1] = first[i][j];
                ++row;
            }
        }
        FOR(i, 1, h) FOR(j, w) (*this)[i][j] = op((*this)[i][j], (*this)[i-1][j]);
        FOR(i, h) FOR(j, 1, w) (*this)[i][j] = op((*this)[i][j], (*this)[i][j-1]);
    }

    template<class Rev = std::minus<T>>
    inline T operator()(size_type a, size_type b, size_type c, size_type d, const Rev rev = std::minus<T>{}) const noexcept(NO_EXCEPT) {
        a = _positivize_index(a), b = _positivize_index(b);
        c = _positivize_index(c), d = _positivize_index(d);
        assert(0 <= a and a <= b and b < (size_type)std::size(*this));
        assert(0 <= c and c <= d and d < (size_type)std::size((*this)[0]));
        return this->_op(rev((*this)[b][d], this->_op((*this)[a][d], (*this)[b][c])), (*this)[a][c]);
    }

    template<class Rev = std::minus<T>>
    inline T operator()(const std::pair<size_type,size_type> p, const std::pair<size_type,size_type> q, const Rev rev = std::minus<T>{}) const noexcept(NO_EXCEPT) {
        return this->operator()(p.first, p.second, q.first, q.second, rev);
    }
};

#if CPP20

template<class I>
explicit accumulation_2d(const I, const I) ->
    accumulation_2d<
        typename std::iterator_traits<typename std::ranges::iterator_t<typename std::iterator_traits<I>::value_type>>::value_type
    >;

#else

template<class I>
explicit accumulation_2d(const I, const I) ->
    accumulation_2d<
        typename std::iterator_traits<typename lib::internal::iterator_t<typename std::iterator_traits<I>::value_type>>::value_type
    >;

#endif


} // namespace lib
