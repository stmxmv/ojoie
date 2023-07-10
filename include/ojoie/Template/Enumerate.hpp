//
// Created by aojoie on 4/17/2023.
//

#ifndef OJOIE_ENUMERATE_HPP
#define OJOIE_ENUMERATE_HPP

#include <iterator>
#include <ranges>
#include <type_traits>

namespace AN {

template<std::ranges::input_range _Range>
    requires std::ranges::view<_Range>
class enumerate_view : public std::ranges::view_interface<enumerate_view<_Range>> {
    _Range _range;

public:
    class iterator {
    public:
        using base              = std::ranges::iterator_t<_Range>;
        using value_type        = std::pair<size_t, std::ranges::range_value_t<_Range>>;
        using difference_type   = std::ranges::range_difference_t<_Range>;
        using iterator_category = typename std::iterator_traits<std::ranges::iterator_t<_Range>>::iterator_category;

        iterator() : t_(), i_() {}
        iterator(base b, size_t i) : t_(std::move(b)), i_{i} {}

        bool operator== (const iterator &other) const {
            return i_ == other.i_;
        }

        bool operator!= (const iterator &other) const {
            return !(*this == other);
        }

        auto operator* () const {
            return std::pair{ i_, *t_ };
        }

        iterator &operator++ () {
            ++t_;
            ++i_;
            return *this;
        }

        iterator operator++ (int) {
            iterator ret(*this);
            ++(*this);
            return ret;
        }

    private:
        base   t_;
        size_t i_;
    };

    constexpr enumerate_view(_Range range) : _range(std::move(range)) {}

    iterator begin() const {
        return iterator{std::begin(_range), 0};
    }

    iterator end() const {
        return iterator{std::end(_range), std::size(_range)};
    }
};


struct enumerate_fn {
    template<typename Rng>
    auto operator() (Rng &&rng) const {
        return enumerate_view{std::ranges::views::all(std::forward<Rng>(rng))};
    }

    template<typename Rng>
    friend auto operator| (Rng &&rng, enumerate_fn const &) {
        return enumerate_view{std::ranges::views::all(std::forward<Rng>(rng))};
    }
};

namespace views {

inline constexpr enumerate_fn Enumerate;

}// namespace views


}// namespace AN

#endif//OJOIE_ENUMERATE_HPP
