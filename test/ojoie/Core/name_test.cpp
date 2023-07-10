//
// Created by aojoie on 4/12/2023.
//

#include <gtest/gtest.h>
#include <ojoie/Core/Name.hpp>
#include <ojoie/Threads/Dispatch.hpp>

using namespace AN;

TEST(Core, Name) {

    Name name("123");
    Name name1("123");
    Name name2("123");
    Name name3("123");
    Name name4("123");
    Name name5("123");
    Name name6("123");
    Name temp = name;
    EXPECT_EQ(temp.c_str(), name.c_str());
    EXPECT_EQ(name, name1);
    EXPECT_EQ(name.c_str(), name1.c_str());

    EXPECT_TRUE(name == name1 && name == name2 && name == name3 && name == name4 && name == name5 && name == name6);

    EXPECT_EQ(name, "123");
    EXPECT_EQ(name, std::string_view{"123"});

    name2 = "321";
    EXPECT_NE(name, name2);
    EXPECT_NE(name1, name2);
    EXPECT_NE(name.c_str(), name2.c_str());
    EXPECT_NE(name2, "123");
    EXPECT_NE(name2, std::string_view{"123"});

    name2 = name;
    EXPECT_EQ(name, name2);
    std::hash<Name> hasher;
    EXPECT_EQ(hasher(name), hasher(name2));

    ThreadID id  = GetCurrentThreadID();
    ThreadID id1 = GetCurrentThreadID();
    ThreadID id2 = GetCurrentThreadID();
    ThreadID id3 = GetCurrentThreadID();
    ThreadID id4 = GetCurrentThreadID();

    EXPECT_EQ(id, id1);
    EXPECT_EQ(id2, id1);
    EXPECT_EQ(id3, id1);
    EXPECT_EQ(id4, id1);
    EXPECT_EQ(id4, id);
}