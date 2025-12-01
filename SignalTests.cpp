
#include "Signal.h"
#include <gtest/gtest.h>

using namespace std;
using namespace testing;

TEST(Signal, AsyncHandler) {

   bool asyncBool = false;
    bool syncBool  = false;

   Signal<int> s;
   auto id = s += AsyncConnection<float> { [&] (int) {
         std::cout << "Async Call" << std::endl;
         std::string str1="0987654321", str2;

         for(int i=0;i<10024;++i) {
             str2=str1;
             str1=str2;
         }
         asyncBool = true;
      }, true
   };
   auto id2 = s += AsyncConnection<float> { [&] (int) {
         std::cout << "Sync Call" << std::endl;
         syncBool = true;
      }, false
   };

   ASSERT_TRUE(id<id2);

   s(9);

   ASSERT_TRUE(syncBool == true);
   ASSERT_TRUE(asyncBool == false);

   s.TryJoinOnAll();

   ASSERT_TRUE(asyncBool == true);
}

TEST(Signal, InlineParamPlacement) {

    struct test {
        int id;
    };

    Signal<test> s;
    auto id = s += AsyncConnection<test>{[] (test t) {
        ASSERT_EQ(t.id, 1);
    }};

    s(test{1});
}


TEST(Signal, TestSignalMove) {
    Signal<> s;
    int invokeCount = 0;
    auto id = s += AsyncConnection<>{[&] () {
        invokeCount++;
    }, false };

    Signal<> s2 = std::move(s);

    s();
    ASSERT_EQ(invokeCount, 0);
    s2();
    ASSERT_EQ(invokeCount, 1);
}

TEST(Signal, TestSignalCopy) {

    int invokeCount = 0;
    Signal<> s;

    auto id = s += AsyncConnection<>{[&]() {
        invokeCount++;
    }, false };
    auto id2 = s += AsyncConnection<>{[&]() {
        invokeCount++;
    }, false };

    Signal<> s2 = s;

    s();
    ASSERT_EQ(invokeCount, 2);
    s2();
    ASSERT_EQ(invokeCount, 4);
}


TEST(Signal, ConnectDisconnectProperOrder) {

    Signal<> s;
    int invokeCount = 0;

    auto id = s += AsyncConnection<>{[&]() {
        invokeCount++;
    },false};
    auto id2 = s += AsyncConnection<>{[&] () {
        invokeCount++;
        ASSERT_EQ(invokeCount, 1);
    },false};
    auto id3 = s += AsyncConnection<>{[&] () {
        invokeCount++;
        ASSERT_EQ(invokeCount, 2);
    },false};

    auto id4 = s.RemoveHandler(id);

    auto id5 = s += AsyncConnection<>{[&] () {
        invokeCount++;
        ASSERT_EQ(invokeCount, 3);
    },false};

    s();
}

TEST(Signal, TestRemoveAllConnections) {
    int invokeCount = 0;
    Signal<> s;
    auto id = s += AsyncConnection<>{[&] () {
        invokeCount++;
    }, false };
    auto id2 = s+= AsyncConnection<>{[&] () {
        invokeCount++;
    }, false };
    auto id3 = s+= AsyncConnection<>{[&] () {
        invokeCount++;
    }, false };

    s();
    ASSERT_EQ(invokeCount, 3);

    s.ClearHandlers();

    s();
    ASSERT_EQ(invokeCount, 3);
}

TEST(Signal, TestNoConnectionsInvoke) {
    Signal<> s;
    s();
}

TEST(Signal, TestRemoveConnection) {
    int invokeCount = 0;
    Signal<> s;

    auto id = s += AsyncConnection<>{[] () {
        ASSERT_TRUE(false);
    }, false };
    auto id2 = s += AsyncConnection<>{[&] () {
        invokeCount ++;
    }, false };
    auto id3 = s += AsyncConnection<>{[] () {
        ASSERT_TRUE(false);
    }, false };
    auto id4 = s += AsyncConnection<>{[] () {
        ASSERT_TRUE(false);
    }, false };

    auto id5 = s.RemoveHandler(id);
    auto id6 = s.RemoveHandler(id3);
    auto id7 = s.RemoveHandler(id4);

    s();

    auto id8 = s += AsyncConnection<>{[] () {
        ASSERT_TRUE(false);
    }};

    auto id9 = s.RemoveHandler(id8);
    s();

    ASSERT_EQ(invokeCount, 2);
}

TEST(Signal, TestPrimitiveTypeConstReference) {
    int payload = 1629;
    Signal<const int&> s;

    auto id = s += AsyncConnection<const int&> {[&] (const int& t) {
        ASSERT_EQ(t, 1629);
    }};
    auto id2 = s += AsyncConnection<const int&> {[&] (const int& t) {
        ASSERT_EQ(t, 1629);
    }};

    s(payload);
}

TEST(Signal, TestPrimitiveTypeBoolByValue) {
    bool payload = true;
    Signal<bool> s;

    auto id = s += AsyncConnection<int> {[&] (int t) {
        ASSERT_EQ(t, true);
    }};
    auto id2 = s += AsyncConnection<int> {[&] (int t) {
        ASSERT_EQ(t, true);
    }};

    s(payload);
}



TEST(Signal, TestPrimitiveTypeFloatByValue) {
    float payload = 16.f;
    Signal<float> s;

    auto id = s += AsyncConnection<int> {[&] (int t) {
        ASSERT_EQ(t, 16.f);
    }};
    auto id2 = s += AsyncConnection<int> {[&] (int t) {
        ASSERT_EQ(t, 16.f);
    }};
    s(payload);
}


TEST(Signal, TestPrimitiveTypeIntByValue) {
    int payload = 1024;
    Signal<int> s;

    auto id = s += AsyncConnection<int> {[] (int t) {
        ASSERT_EQ(t, 1024);
    }};
    auto id2 = s += AsyncConnection<int> {[] (int t) {
        ASSERT_EQ(t, 1024);
    }};
    s(payload);
}

TEST(Signal, TestPrimitiveTypeBoolByReference) {

    bool payload = true;
    Signal<bool&> s;

    auto id = s += AsyncConnection<bool&> {[&] (auto& t) {
        ASSERT_EQ(t, true);
        payload = false;
    }};
    auto id2 = s += AsyncConnection<bool&> {[&] (auto t) {
        ASSERT_EQ(t, false);
    }};

    ASSERT_TRUE(id2 > id);
    s(payload);
}

TEST(Signal, TestPrimitiveTypeFloatByReference) {

    float payload = 16.f;
    Signal<float&> s;

    auto id = s += AsyncConnection<float&> {[&] (auto& t) {
        ASSERT_EQ(t, 16.f);
        payload = 18.f;
    }};
    auto id2 = s += AsyncConnection<float&> { [] (auto t) {
        ASSERT_EQ(t, 18.f);
    }};

    ASSERT_TRUE(id < id2);
    s(payload);
}

TEST(Signal, TestPrimitiveTypeIntByReference) {
    int payload = 1024;
    Signal<int&> s;

    auto id = s += AsyncConnection<int&> { [&] (auto& t) {
        ASSERT_EQ(t, 1024);
        payload = 2048;
    }};
    auto id2 = s += AsyncConnection<int&> {[&] (auto& t) {
        ASSERT_EQ(t, 2048);
    }};

    s(payload);
}


TEST(Signal, TestMultipleConnectionsStructByReference) {

    struct test {
        int id;
        string message;
    };

    test t = {123, "message1"};
    Signal<test&> s;

    int invokeCount = 0;

    auto id = s += AsyncConnection<test&> { [&] (test& t) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(invokeCount, 0);
    }};
    auto id2 = s += AsyncConnection<test&> { [&] (test& t) {
        invokeCount ++;
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(invokeCount, 1);
    }};

    s(t);
}

TEST(Signal, TestMultipleConnectionsStructCopy) {

    struct test {
        int id;
        string message;
        string errorDescription;
    };

    test t = {123, "message1", "description1"};

    Signal<test> s;

    int invokeCount = 0;

 auto id = s += AsyncConnection<test> {[&](test t){
    ASSERT_EQ(t.id, 123);
    ASSERT_EQ(t.message, "message1");
    ASSERT_EQ(t.errorDescription, "description1");
    ASSERT_EQ(invokeCount, 0);
}, false};

auto id2 = s += AsyncConnection<test> {[&](test t){
    invokeCount++;
    ASSERT_EQ(t.id, 123);
    ASSERT_EQ(t.message, "message1");
    ASSERT_EQ(t.errorDescription, "description1");
    ASSERT_EQ(invokeCount, 1);
}, false};

    s(t);
    invokeCount = 0;

    s(t);
}

TEST(Signal, TestMultipleTypesCopy) {

    struct test {
        int id;
        string message;
        string errorDescription;
    };
    struct test2 {
        string payload;
    };

    test t = {123, "message1", "description1"};
    test2 t2 = {"empty_payload"};

    Signal<test, test2> s;

    auto id = s += AsyncConnection<test,test2> { [] (test t, test2 t2) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t.errorDescription, "description1");
        ASSERT_EQ(t2.payload, "empty_payload");
    }};
    auto id2 = s += AsyncConnection<test,test2> { [] (test t, test2 t2) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t.errorDescription, "description1");
        ASSERT_EQ(t2.payload, "empty_payload");
    }};

    s(t, t2);
}

TEST(Signal, TestMultipleUserTypesByValueAndReference) {

    struct test {
        int id;
        string message;
    };
    struct test2 {
        string payload;
    };

    test t = {123, "message1"};
    test2 t2 = {"empty_payload"};

    Signal<test&, test2> s;

    auto id = s += AsyncConnection<test&,test2> { [] (test& t, test2 t2) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t2.payload, "empty_payload");
    }};
    auto id2 = s += AsyncConnection<test&,test2> {[] (test& t, test2 t2) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t2.payload, "empty_payload");
    }};

    s(t, t2);
}

TEST(Signal, TestMultipleUserTypesAndPrimitiveTypeByValue) {

    struct test {
        int id;
        string message;
    };
    struct test2 {
        string payload;
    };

    test t = {123, "message1"};
    test2 t2 = {"empty_payload"};

    Signal<test, test2, int> s;

    auto ids = s += AsyncConnection<test&,test2, int> {[] (test& t, test2 t2, auto i) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t2.payload, "empty_payload");
        ASSERT_EQ(i, 22);
    }};
    auto id = s += AsyncConnection<test&,test2,int> {[] (test& t, test2 t2, auto i) {
        ASSERT_EQ(t.id, 123);
        ASSERT_EQ(t.message, "message1");
        ASSERT_EQ(t2.payload, "empty_payload");
        ASSERT_EQ(i, 22);
    }};

    s(t, t2, 22);
}

int main(int argc, const char * argv[]) {

    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
