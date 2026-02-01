#include <gtest/gtest.h>
#include "../include/sv/core/ptp.h"
#include <thread>
#include <chrono>

using namespace sv;

TEST(PtpTimestampTest, DefaultConstructor)
{
    constexpr PtpTimestamp ts;
    EXPECT_FALSE(ts.isValid());
    EXPECT_EQ(ts.getSeconds(), 0);
    EXPECT_EQ(ts.getNanoseconds(), 0);
}

TEST(PtpTimestampTest, ConstructorValid)
{
    constexpr PtpTimestamp ts(1234567890, 123456789);
    EXPECT_TRUE(ts.isValid());
    EXPECT_EQ(ts.getSeconds(), 1234567890);
    EXPECT_EQ(ts.getNanoseconds(), 123456789);
}

TEST(PtpTimestampTest, ConstructorInvalidNanoseconds)
{
    constexpr PtpTimestamp ts(1234567890, 1'500'000'000);
    EXPECT_FALSE(ts.isValid());
}

TEST(PtpTimestampTest, NowCreatesValidTimestamp)
{
    const auto ts = PtpTimestamp::now();
    EXPECT_TRUE(ts.isValid());
    EXPECT_GT(ts.getSeconds(), 1'000'000'000ULL);
    EXPECT_LT(ts.getNanoseconds(), 1'000'000'000U);
}

TEST(PtpTimestampTest, NowIncreases)
{
    const auto ts1 = PtpTimestamp::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto ts2 = PtpTimestamp::now();

    EXPECT_TRUE(ts2 > ts1);
}

TEST(PtpTimestampTest, ToTaiFormat)
{
    constexpr PtpTimestamp ts(0x12345678, 500'000'000);
    const auto tai = ts.toTai();

    EXPECT_EQ(tai[0], 0x12);
    EXPECT_EQ(tai[1], 0x34);
    EXPECT_EQ(tai[2], 0x56);
    EXPECT_EQ(tai[3], 0x78);
}

TEST(PtpTimestampTest, FromTaiFormat)
{
    std::array<uint8_t, 8> data = {0x12, 0x34, 0x56, 0x78, 0x80, 0x00, 0x00, 0x00};

    const auto ts = PtpTimestamp::fromTai(data);
    ASSERT_TRUE(ts.has_value());
    EXPECT_TRUE(ts->isValid());
    EXPECT_EQ(ts->getSeconds(), 0x12345678);
}

TEST(PtpTimestampTest, RoundTripTaiConversion)
{
    constexpr PtpTimestamp original(1234567890, 500'000'000);
    const auto tai = original.toTai();
    const auto reconstructed = PtpTimestamp::fromTai(tai);

    ASSERT_TRUE(reconstructed.has_value());
    EXPECT_EQ(original.getSeconds(), reconstructed->getSeconds());

    constexpr uint32_t tolerance = 1;
    EXPECT_NEAR(original.getNanoseconds(), reconstructed->getNanoseconds(), tolerance);
}

TEST(PtpTimestampTest, EqualityComparison)
{
    constexpr PtpTimestamp ts1(1000, 500);
    constexpr PtpTimestamp ts2(1000, 500);
    constexpr PtpTimestamp ts3(1000, 501);
    constexpr PtpTimestamp ts4(1001, 500);

    EXPECT_TRUE(ts1 == ts2);
    EXPECT_FALSE(ts1 == ts3);
    EXPECT_FALSE(ts1 == ts4);

    EXPECT_FALSE(ts1 != ts2);
    EXPECT_TRUE(ts1 != ts3);
    EXPECT_TRUE(ts1 != ts4);
}

TEST(PtpTimestampTest, LessThanComparison)
{
    constexpr PtpTimestamp ts1(1000, 500);
    constexpr PtpTimestamp ts2(1000, 600);
    constexpr PtpTimestamp ts3(1001, 400);

    EXPECT_TRUE(ts1 < ts2);
    EXPECT_TRUE(ts1 < ts3);
    EXPECT_TRUE(ts2 < ts3);
    EXPECT_FALSE(ts2 < ts1);
    EXPECT_FALSE(ts3 < ts1);
}

TEST(PtpTimestampTest, GreaterThanComparison)
{
    constexpr PtpTimestamp ts1(1000, 500);
    constexpr PtpTimestamp ts2(1000, 600);
    constexpr PtpTimestamp ts3(1001, 400);

    EXPECT_TRUE(ts2 > ts1);
    EXPECT_TRUE(ts3 > ts1);
    EXPECT_TRUE(ts3 > ts2);
    EXPECT_FALSE(ts1 > ts2);
    EXPECT_FALSE(ts1 > ts3);
}

TEST(PtpTimestampTest, LessOrEqualComparison)
{
    constexpr PtpTimestamp ts1(1000, 500);
    constexpr PtpTimestamp ts2(1000, 500);
    constexpr PtpTimestamp ts3(1000, 600);

    EXPECT_TRUE(ts1 <= ts2);
    EXPECT_TRUE(ts1 <= ts3);
    EXPECT_FALSE(ts3 <= ts1);
}

TEST(PtpTimestampTest, GreaterOrEqualComparison)
{
    constexpr PtpTimestamp ts1(1000, 500);
    constexpr PtpTimestamp ts2(1000, 500);
    constexpr PtpTimestamp ts3(1000, 600);

    EXPECT_TRUE(ts1 >= ts2);
    EXPECT_TRUE(ts3 >= ts1);
    EXPECT_FALSE(ts1 >= ts3);
}

TEST(PtpTimestampTest, ToTimePoint)
{
    const auto ts = PtpTimestamp::now();
    const auto tp = ts.toTimePoint();

    const auto now = std::chrono::system_clock::now();
    const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - tp);

    EXPECT_LT(std::abs(diff.count()), 100);
}

TEST(PtpTimestampTest, BoundaryNanoseconds)
{
    constexpr PtpTimestamp ts1(1000, 0);
    EXPECT_TRUE(ts1.isValid());

    constexpr PtpTimestamp ts2(1000, 999'999'999);
    EXPECT_TRUE(ts2.isValid());

    constexpr PtpTimestamp ts3(1000, 1'000'000'000);
    EXPECT_FALSE(ts3.isValid());
}

TEST(PtpTimestampTest, ZeroTimestamp)
{
    constexpr PtpTimestamp ts(0, 0);
    EXPECT_TRUE(ts.isValid());
    EXPECT_EQ(ts.getSeconds(), 0);
    EXPECT_EQ(ts.getNanoseconds(), 0);
}

TEST(PtpTimestampTest, MaxSeconds)
{
    constexpr uint64_t maxSeconds = 0xFFFFFFFFFFFFFFFFULL;
    constexpr PtpTimestamp ts(maxSeconds, 999'999'999);
    EXPECT_TRUE(ts.isValid());
    EXPECT_EQ(ts.getSeconds(), maxSeconds);
}
