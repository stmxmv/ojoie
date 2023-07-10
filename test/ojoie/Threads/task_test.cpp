//
// Created by aojoie on 4/12/2023.
//

#include <gtest/gtest.h>
#include <ojoie/Core/Name.hpp>
#include <ojoie/Threads/Task.hpp>

#include <iostream>

using namespace std;
using namespace AN;

TEST(Threads, Task) {

    std::string str("some string content");

    struct LargeStruct {
        char buffer[256];
    } largeStruct;

    TaskItem item1([largeStruct, str] {
        (void)str;
        (void)largeStruct;
        cout << "task run " << str << endl;
    });

    TaskInterface task1 = std::move(item1);

    TaskInterface task2(std::move(task1));

    item1.run();
    task1.run();

    task2.run();

}