#include <gtest/gtest.h>
#include <postgres/Result.h>
#include "Migration.h"

namespace postgres {

struct TestResult : Migration, testing::Test {};

TEST_F(TestResult, NullPtr) {
    Result res{nullptr};
    res.status();
    res.statusName();
    res.errorMessage();
    ASSERT_FALSE(res.isOk());
    ASSERT_FALSE(res);
    ASSERT_TRUE(res.isDone());
    ASSERT_EQ(0, res.size());
    ASSERT_EQ(0, res.affected());
    ASSERT_TRUE(res.empty());
    ASSERT_THROW(res.begin(), std::exception);
    ASSERT_THROW(res.end(), std::exception);
    ASSERT_THROW(res.front(), std::exception);
    ASSERT_THROW(res[0], std::exception);
}

TEST_F(TestResult, Invalid) {
    const auto res = conn_->execute("BAD STATEMENT");
    ASSERT_FALSE(res.isOk());
    ASSERT_FALSE(res);
    ASSERT_EQ(0, res.size());
    ASSERT_EQ(0, res.affected());
    ASSERT_TRUE(res.empty());
    ASSERT_THROW(res.begin(), std::exception);
    ASSERT_THROW(res.end(), std::exception);
    ASSERT_THROW(res.front(), std::exception);
    ASSERT_THROW(res[0], std::exception);
}

TEST_F(TestResult, Empty) {
    const auto res = conn_->execute("SELECT * FROM test");
    ASSERT_TRUE(res.isOk());
    ASSERT_EQ(0, res.size());
    ASSERT_EQ(0, res.affected());
    ASSERT_TRUE(res.empty());
    for (const auto tuple : res) {
        // Must not reach here.
        ASSERT_FALSE((bool)"Iterating empty result");
    }
}

TEST_F(TestResult, Survival) {
    const auto res = conn_->execute("SELECT 123::INTEGER");
    {
        conn_->~Connection();
    }
    ASSERT_TRUE(res.isOk());
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(123, (int) res[0][0]);
}

TEST_F(TestResult, Iteration) {
    std::vector<int32_t> data{};
    conn_->execute("INSERT INTO test(int4) VALUES(1), (2), (3)");
    for (auto tuple : conn_->execute("SELECT int4 FROM test")) {
        data.emplace_back();
        tuple >> data.back();
    }
    ASSERT_EQ(3u, data.size());
    ASSERT_EQ(1, data[0]);
    ASSERT_EQ(2, data[1]);
    ASSERT_EQ(3, data[2]);
}

TEST_F(TestResult, Cast) {
    const auto f = [](const int n, const double d, const bool b, const std::string& s) {
        ASSERT_EQ(4, n);
        ASSERT_DOUBLE_EQ(8.88, d);
        ASSERT_EQ(true, b);
        ASSERT_EQ("INFO", s);
    };
    const auto res = conn_->execute("SELECT 4::INTEGER, 8.88::FLOAT, TRUE, 'INFO'");
    f(res[0][0], res[0][1], res[0][2], res[0][3]);
}

TEST_F(TestResult, ColumnName) {
    const auto res = conn_->execute("SELECT 1 AS one, 2 AS two");
    ASSERT_STREQ("one", res[0][0].name());
    ASSERT_STREQ("two", res[0][1].name());
}

}  // namespace postgres