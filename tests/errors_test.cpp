#include <gtest/gtest.h>
#include "gocxx/errors/errors.h"

using namespace gocxx::errors;

struct CustomError : public Error {
    std::string detail;
    explicit CustomError(std::string d) : detail(std::move(d)) {}
    std::string error() const noexcept override { return "custom: " + detail; }
};

TEST(ErrorsTest, New) {
    auto err = New("something went wrong");
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->error(), "something went wrong");
}

TEST(ErrorsTest, Wrap) {
    auto base = New("base error");
    auto wrapped = Wrap("context", base);

    ASSERT_NE(wrapped, nullptr);
    EXPECT_EQ(wrapped->error(), "context");
    EXPECT_EQ(wrapped->Unwrap(), base);
}

TEST(ErrorsTest, Unwrap) {
    auto root = New("root");
    auto mid = Wrap("mid", root);
    auto top = Wrap("top", mid);

    EXPECT_EQ(Unwrap(top), mid);
    EXPECT_EQ(Unwrap(Unwrap(top)), root);
}

TEST(ErrorsTest, Is) {
    auto target = New("target");
    auto wrapped = Wrap("ctx1", target);
    auto top = Wrap("ctx2", wrapped);

    EXPECT_TRUE(Is(top, target));
    EXPECT_FALSE(Is(top, New("different")));
}

TEST(ErrorsTest, As_BuiltInTypes) {
    auto base = New("simple");
    auto wrapped = Wrap("wrapped", base);

    std::shared_ptr<wrappedError> we;
    EXPECT_TRUE(As(wrapped, we));
    ASSERT_NE(we, nullptr);
    EXPECT_EQ(we->error(), "wrapped");

    std::shared_ptr<simpleError> se;
    EXPECT_TRUE(As(base, se));
    ASSERT_NE(se, nullptr);
    EXPECT_EQ(se->error(), "simple");
}

TEST(ErrorsTest, As_CustomType) {
    auto custom = std::make_shared<CustomError>("detail");
    auto wrapped = Wrap("outer", custom);

    std::shared_ptr<CustomError> ce;
    EXPECT_TRUE(As(wrapped, ce));
    ASSERT_NE(ce, nullptr);
    EXPECT_EQ(ce->detail, "detail");
}

TEST(ErrorsTest, Join) {
    auto e1 = New("e1");
    auto e2 = New("e2");
    auto joined = Join({ e1, nullptr, e2 });

    ASSERT_NE(joined, nullptr);
    std::string errStr = joined->error();
    EXPECT_NE(errStr.find("e1"), std::string::npos);
    EXPECT_NE(errStr.find("e2"), std::string::npos);

    auto joined_single = Join({ nullptr, e1 });
    EXPECT_EQ(joined_single, e1);

    EXPECT_EQ(Join({}), nullptr);
    EXPECT_EQ(Join({ nullptr }), nullptr);
}
