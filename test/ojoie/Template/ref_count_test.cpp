//
// Created by aojoie on 4/17/2023.
//

#include <gtest/gtest.h>

#include <ojoie/Template/RC.hpp>

#include <iostream>
#include <ranges>
#include <vector>

using namespace AN;
using std::cout, std::endl;


class TestObject : public RefCounted<TestObject> {
    int someValue = 14;
public:

    void operator delete(void *mem) {}

};

TEST(Template, RefCount) {

    TestObject testMem;
    TestObject *test = &testMem;
    {
        EXPECT_TRUE(test->getRetainCount() == 1);
        RefCountedPtr<TestObject> ptr = ref_transfer(test);

        ptr = ptr;

        ptr = RefCountedPtr<TestObject>(ptr);

        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1(ptr);
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1(test);
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1 = ptr;
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1;
            ptr1 = test;
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1;
            ptr1 = ptr;
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);

        {
            RefCountedPtr<TestObject> ptr1{RefCountedPtr<TestObject>(ptr)};
            EXPECT_TRUE(test->getRetainCount() == 2);
        }
        EXPECT_TRUE(test->getRetainCount() == 1);
    }

    EXPECT_TRUE(test->getRetainCount() == 0);

    TestObject test2Mem;
    TestObject *test2 = &test2Mem;

    {
        RefCountedPtr<TestObject> ptr(test2);
        EXPECT_TRUE(test2->getRetainCount() == 2);

        ptr = nullptr;
        EXPECT_TRUE(test2->getRetainCount() == 1);

        ptr = test2;
        EXPECT_TRUE(test2->getRetainCount() == 2);

        ptr.reset();
        EXPECT_TRUE(test2->getRetainCount() == 1);

        ptr = test2;
        EXPECT_TRUE(test2->getRetainCount() == 2);
        TestObject *obj = ptr.detach();
        EXPECT_TRUE(test2->getRetainCount() == 2);
        obj->release();
        EXPECT_TRUE(test2->getRetainCount() == 1);

        {
            auto task = [self = RefCountedPtr(test2)]() {
                EXPECT_TRUE(self->getRetainCount() == 2);
            };
            task();
        }
        EXPECT_TRUE(test2->getRetainCount() == 1);

    }

    EXPECT_TRUE(test2->getRetainCount() == 1);

    test2->release();
}