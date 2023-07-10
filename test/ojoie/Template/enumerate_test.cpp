//
// Created by aojoie on 4/17/2023.
//

#include <gtest/gtest.h>

#include <iostream>
#include <ojoie/Math/LRUCache.hpp>
#include <ojoie/Template/Enumerate.hpp>
#include <ranges>
#include <vector>

using namespace AN;
using std::cout, std::endl;
TEST(Template, enumerate) {

    std::vector vec{7, 6, 5, 4, 3, 2, 1, 34, 344, 0, 0, -1, 23, 1, 2, 034, 23};

    auto a = vec | views::Enumerate;
    std::begin(a);
    std::ranges::begin(a);
    std::ranges::end(a);

    static_assert(std::ranges::view<decltype(a)>);
    for (auto &&[i, val] : vec | views::Enumerate |
                                   std::views::filter([](auto &&val) { return val.second > 3; })) {
        cout << i << ' ' << val << '\n';
        EXPECT_EQ(vec[i], val);
        EXPECT_TRUE(val > 3);
    }


    LRUCache<int, char> cache;
    cache.set(1, 'a');
    cache.set(2, 'b');
    cache.set(3, 'c');

    char ch;
    cache.get(1, ch);
    EXPECT_EQ(ch, 'a');
    cache.get(2, ch);
    EXPECT_EQ(ch, 'b');
    cache.get(3, ch);
    EXPECT_EQ(ch, 'c');

    for (const auto &[k, v] : cache.mapView()) {
        cout << k << ' ' << v << '\n';
    }

    cout << '\n';

    for (const auto &[k, v] : cache.mapLRUView()) {
        cout << k << ' ' << v << '\n';
    }

    auto willDeleteView = cache.mapView() |
                          std::views::take(3) |
                      std::views::filter([](auto &&pair) {
                          return pair.second >= 'b';
                      }) |
                      std::views::transform([](auto &&pair) {
                          return pair.first;
                      });

    std::vector<int> willDelete;
    std::ranges::copy(willDeleteView, std::back_inserter(willDelete));

    for (const auto &k : willDelete) {
        cache.erase(k);
    }

    cout << "after delete" << '\n';

    for (const auto &[k, v] : cache.mapView()) {
        cout << k << ' ' << v << '\n';
    }

    cout.flush();
}