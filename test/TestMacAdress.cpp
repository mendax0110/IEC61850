#include <gtest/gtest.h>
#include "sv/model/IedModel.h"
#include "sv/core/mac.h"
#include "sv/core/buffer.h"


TEST(MacAddressTest, DefaultConstructor)
{
    const sv::MacAddress mac;
    EXPECT_TRUE(mac.isZero());
    EXPECT_EQ(mac.toString(), "00:00:00:00:00:00");
}

TEST(MacAddressTest, ArrayConstructor)
{
    const std::array<uint8_t, 6> bytes = {0x01, 0x0C, 0xCD, 0x04, 0x00, 0x01};
    const sv::MacAddress mac(bytes);
    EXPECT_EQ(mac.toString(), "01:0C:CD:04:00:01");
    EXPECT_EQ(mac.toString(true), "01:0C:CD:04:00:01");
    EXPECT_EQ(mac.toString(false), "01:0c:cd:04:00:01");
}

TEST(MacAddressTest, ParseValidAddress)
{
    const auto mac = sv::MacAddress::parse("01:0C:CD:04:00:01");
    EXPECT_EQ(mac.toString(), "01:0C:CD:04:00:01");
    EXPECT_EQ(mac.toString(true), "01:0C:CD:04:00:01");
    EXPECT_EQ(mac.toString(false), "01:0c:cd:04:00:01");
}

TEST(MacAddressTest, ParseInvalidAddress)
{
    EXPECT_THROW(sv::MacAddress::parse("invalid"), std::invalid_argument);
    EXPECT_THROW(sv::MacAddress::parse("01:02:03"), std::invalid_argument);
    EXPECT_THROW(sv::MacAddress::parse("01:02:03:04:05:GG"), std::invalid_argument);
}

TEST(MacAddressTest, TryParseValid)
{
    const auto result = sv::MacAddress::tryParse("FF:FF:FF:FF:FF:FF");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().isBroadcast());
}

TEST(MacAddressTest, TryParseInvalid)
{
    const auto result = sv::MacAddress::tryParse("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST(MacAddressTest, TryParseLegacyAPI)
{
    sv::MacAddress mac;
    EXPECT_TRUE(sv::MacAddress::tryParse("01:02:03:04:05:06", mac));
    EXPECT_EQ(mac.toString(), "01:02:03:04:05:06");

    EXPECT_FALSE(sv::MacAddress::tryParse("invalid", mac));
}

TEST(MacAddressTest, IsMulticast)
{
    const auto multicast = sv::MacAddress::parse("01:00:5E:00:00:01");
    EXPECT_TRUE(multicast.isMulticast());
    EXPECT_TRUE(multicast.isUnicast() == false);

    const auto unicast = sv::MacAddress::parse("00:11:22:33:44:55");
    EXPECT_FALSE(unicast.isMulticast());
    EXPECT_TRUE(unicast.isUnicast());
}

TEST(MacAddressTest, IsBroadcast)
{
    const auto broadcast = sv::MacAddress::broadcast();
    EXPECT_TRUE(broadcast.isBroadcast());
    EXPECT_EQ(broadcast.toString(), "FF:FF:FF:FF:FF:FF");
    EXPECT_EQ(broadcast.toString(false), "ff:ff:ff:ff:ff:ff");

    const auto notBroadcast = sv::MacAddress::parse("FF:FF:FF:FF:FF:FE");
    EXPECT_FALSE(notBroadcast.isBroadcast());
}

TEST(MacAddressTest, IsZero)
{
    constexpr auto zero = sv::MacAddress::zero();
    EXPECT_TRUE(zero.isZero());

    const auto notZero = sv::MacAddress::parse("00:00:00:00:00:01");
    EXPECT_FALSE(notZero.isZero());
}

TEST(MacAddressTest, IsLocallyAdministered)
{
    const auto local = sv::MacAddress::parse("02:00:00:00:00:00");
    EXPECT_TRUE(local.isLocallyAdministered());
    EXPECT_FALSE(local.isUniversallyAdministered());

    const auto universal = sv::MacAddress::parse("00:00:00:00:00:00");
    EXPECT_FALSE(universal.isLocallyAdministered());
    EXPECT_TRUE(universal.isUniversallyAdministered());
}

TEST(MacAddressTest, SVMulticastBase)
{
    const auto svBase = sv::MacAddress::svMulticastBase();
    EXPECT_EQ(svBase.toString(), "01:0C:CD:04:00:00");
    EXPECT_EQ(svBase.toString(false), "01:0c:cd:04:00:00");
    EXPECT_TRUE(svBase.isMulticast());
}

TEST(MacAddressTest, GOOSEMulticastBase)
{
    constexpr auto gooseBase = sv::MacAddress::gooseMulticastBase();
    EXPECT_EQ(gooseBase.toString(), "01:0C:CD:01:00:00");
    EXPECT_EQ(gooseBase.toString(false), "01:0c:cd:01:00:00");
    EXPECT_TRUE(gooseBase.isMulticast());
}

TEST(MacAddressTest, Equality)
{
    const auto mac1 = sv::MacAddress::parse("01:02:03:04:05:06");
    const auto mac2 = sv::MacAddress::parse("01:02:03:04:05:06");
    const auto mac3 = sv::MacAddress::parse("01:02:03:04:05:07");

    EXPECT_EQ(mac1, mac2);
    EXPECT_NE(mac1, mac3);
}

TEST(MacAddressTest, Comparison)
{
    const auto mac1 = sv::MacAddress::parse("01:02:03:04:05:06");
    const auto mac2 = sv::MacAddress::parse("01:02:03:04:05:07");

    EXPECT_LT(mac1, mac2);
    EXPECT_LE(mac1, mac2);
    EXPECT_GT(mac2, mac1);
    EXPECT_GE(mac2, mac1);
}

TEST(MacAddressTest, ArrayAccess)
{
    auto mac = sv::MacAddress::parse("01:02:03:04:05:06");
    EXPECT_EQ(mac[0], 0x01);
    EXPECT_EQ(mac[1], 0x02);
    EXPECT_EQ(mac[5], 0x06);
}

TEST(MacAddressTest, ToStringLowercase)
{
    const auto mac = sv::MacAddress::parse("AB:CD:EF:01:02:03");
    EXPECT_EQ(mac.toString(true), "AB:CD:EF:01:02:03");
    EXPECT_EQ(mac.toString(false), "ab:cd:ef:01:02:03");
}

TEST(MacAddressTest, StreamOutput)
{
    const auto mac = sv::MacAddress::parse("01:02:03:04:05:06");
    std::ostringstream oss;
    oss << mac;
    EXPECT_EQ(oss.str(), "01:02:03:04:05:06");
}