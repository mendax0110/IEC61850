#include <gtest/gtest.h>
#include "sv/protection/Protection.h"
#include "sv/network/PcapWriter.h"
#include <fstream>
#include <filesystem>
#include <cmath>
#include <thread>
#include <chrono>

using namespace sv;

TEST(OvercurrentProtectionSettings, DefaultValues)
{
    OvercurrentProtectionSettings settings;
    EXPECT_DOUBLE_EQ(settings.pickupCurrentA, 100.0);
    EXPECT_DOUBLE_EQ(settings.timeMultiplier, 1.0);
    EXPECT_EQ(settings.curveType, OvercurrentCurveType::StandardInverse);
    EXPECT_EQ(settings.definiteTimeDelay, std::chrono::milliseconds(500));
    EXPECT_TRUE(settings.enabled);
    EXPECT_TRUE(settings.isValid());
}

TEST(OvercurrentProtectionSettings, InvalidPickupCurrent)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 0.0;
    EXPECT_FALSE(settings.isValid());

    settings.pickupCurrentA = -1.0;
    EXPECT_FALSE(settings.isValid());
}

TEST(OvercurrentProtectionSettings, InvalidTimeMultiplier)
{
    OvercurrentProtectionSettings settings;
    settings.timeMultiplier = 0.0;
    EXPECT_FALSE(settings.isValid());

    settings.timeMultiplier = -0.5;
    EXPECT_FALSE(settings.isValid());
}

TEST(OvercurrentProtection, CreateWithDefaultSettings)
{
    auto prot = OvercurrentProtection::create();
    ASSERT_NE(prot, nullptr);
    EXPECT_TRUE(prot->isEnabled());

    auto settings = prot->getSettings();
    EXPECT_DOUBLE_EQ(settings.pickupCurrentA, 100.0);
    EXPECT_DOUBLE_EQ(settings.timeMultiplier, 1.0);
    EXPECT_EQ(settings.curveType, OvercurrentCurveType::StandardInverse);
}

TEST(OvercurrentProtection, CreateWithCustomSettings)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 50.0;
    settings.timeMultiplier = 0.3;
    settings.curveType = OvercurrentCurveType::VeryInverse;

    auto prot = OvercurrentProtection::create(settings);
    ASSERT_NE(prot, nullptr);

    auto got = prot->getSettings();
    EXPECT_DOUBLE_EQ(got.pickupCurrentA, 50.0);
    EXPECT_DOUBLE_EQ(got.timeMultiplier, 0.3);
    EXPECT_EQ(got.curveType, OvercurrentCurveType::VeryInverse);
}

TEST(OvercurrentProtection, CreateWithInvalidSettingsThrows)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 0.0;
    EXPECT_THROW(OvercurrentProtection::create(settings), std::invalid_argument);
}

TEST(OvercurrentProtection, StandardInverseCurve)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.timeMultiplier = 1.0;
    settings.curveType = OvercurrentCurveType::StandardInverse;

    auto prot = OvercurrentProtection::create(settings);

    // Below pickup - should return infinity
    double t = prot->calculateOperatingTimeMs(50.0);
    EXPECT_TRUE(std::isinf(t));

    // At pickup - should return infinity (M = 1.0 -> division by zero)
    t = prot->calculateOperatingTimeMs(100.0);
    EXPECT_TRUE(std::isinf(t));

    // SI curve: t = TMS * 0.14 / (M^0.02 - 1) * 1000 ms
    // At 2x pickup (M=2): t = 1.0 * 0.14 / (2^0.02 - 1) = 0.14 / 0.01396 ≈ 10.029 s
    t = prot->calculateOperatingTimeMs(200.0);
    EXPECT_GT(t, 9000.0);   // > 9 seconds
    EXPECT_LT(t, 11000.0);  // < 11 seconds

    // At 10x pickup (M=10): t = 0.14 / (10^0.02 - 1) ≈ 0.14 / 0.04713 ≈ 2.970 s
    t = prot->calculateOperatingTimeMs(1000.0);
    EXPECT_GT(t, 2500.0);
    EXPECT_LT(t, 3500.0);
}

TEST(OvercurrentProtection, VeryInverseCurve)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.timeMultiplier = 1.0;
    settings.curveType = OvercurrentCurveType::VeryInverse;

    auto prot = OvercurrentProtection::create(settings);

    // VI curve: t = TMS * 13.5 / (M - 1) * 1000 ms
    // At 2x pickup (M=2): t = 13.5 / 1.0 = 13.5 s
    double t = prot->calculateOperatingTimeMs(200.0);
    EXPECT_NEAR(t, 13500.0, 10.0);

    // At 5x pickup (M=5): t = 13.5 / 4.0 = 3.375 s
    t = prot->calculateOperatingTimeMs(500.0);
    EXPECT_NEAR(t, 3375.0, 10.0);

    // At 10x pickup (M=10): t = 13.5 / 9.0 = 1.5 s
    t = prot->calculateOperatingTimeMs(1000.0);
    EXPECT_NEAR(t, 1500.0, 10.0);
}

TEST(OvercurrentProtection, ExtremelyInverseCurve)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.timeMultiplier = 1.0;
    settings.curveType = OvercurrentCurveType::ExtremelyInverse;

    auto prot = OvercurrentProtection::create(settings);

    // EI curve: t = TMS * 80.0 / (M^2 - 1) * 1000 ms
    // At 2x pickup (M=2): t = 80.0 / (4 - 1) = 80/3 ≈ 26.667 s
    double t = prot->calculateOperatingTimeMs(200.0);
    EXPECT_NEAR(t, 26666.7, 10.0);

    // At 5x pickup (M=5): t = 80.0 / (25 - 1) = 80/24 ≈ 3.333 s
    t = prot->calculateOperatingTimeMs(500.0);
    EXPECT_NEAR(t, 3333.3, 10.0);

    // At 10x pickup (M=10): t = 80 / (100-1) ≈ 0.808 s
    t = prot->calculateOperatingTimeMs(1000.0);
    EXPECT_NEAR(t, 808.1, 10.0);
}

TEST(OvercurrentProtection, LongTimeInverseCurve)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.timeMultiplier = 1.0;
    settings.curveType = OvercurrentCurveType::LongTimeInverse;

    auto prot = OvercurrentProtection::create(settings);

    // LTI curve: t = TMS * 120.0 / (M - 1) * 1000 ms
    // At 2x pickup (M=2): t = 120.0 / 1.0 = 120 s
    double t = prot->calculateOperatingTimeMs(200.0);
    EXPECT_NEAR(t, 120000.0, 10.0);

    // At 5x pickup (M=5): t = 120.0 / 4.0 = 30 s
    t = prot->calculateOperatingTimeMs(500.0);
    EXPECT_NEAR(t, 30000.0, 10.0);
}

TEST(OvercurrentProtection, DefiniteTimeCurve)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.timeMultiplier = 1.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(300);

    auto prot = OvercurrentProtection::create(settings);

    // DT returns the fixed delay regardless of multiple
    double t = prot->calculateOperatingTimeMs(200.0);
    EXPECT_NEAR(t, 300.0, 0.1);

    t = prot->calculateOperatingTimeMs(1000.0);
    EXPECT_NEAR(t, 300.0, 0.1);

    // Below pickup still returns infinity
    t = prot->calculateOperatingTimeMs(50.0);
    EXPECT_TRUE(std::isinf(t));
}

TEST(OvercurrentProtection, TimeMultiplierScaling)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::VeryInverse;

    // TMS = 1.0: t = 13.5 / (M-1) at M=2 -> 13.5s
    settings.timeMultiplier = 1.0;
    auto prot = OvercurrentProtection::create(settings);
    double t1 = prot->calculateOperatingTimeMs(200.0);

    // TMS = 0.5: t should be half
    settings.timeMultiplier = 0.5;
    prot->setSettings(settings);
    double t2 = prot->calculateOperatingTimeMs(200.0);

    EXPECT_NEAR(t2, t1 / 2.0, 1.0);

    // TMS = 2.0: t should be double
    settings.timeMultiplier = 2.0;
    prot->setSettings(settings);
    double t3 = prot->calculateOperatingTimeMs(200.0);

    EXPECT_NEAR(t3, t1 * 2.0, 1.0);
}

TEST(OvercurrentProtection, NoBelowPickup)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(10);

    auto prot = OvercurrentProtection::create(settings);

    auto result = prot->update(50.0);
    EXPECT_FALSE(result.trip);
    EXPECT_DOUBLE_EQ(result.measuredCurrentA, 50.0);
}

TEST(OvercurrentProtection, TripOnDefiniteTime)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(50);

    auto prot = OvercurrentProtection::create(settings);

    auto result = prot->update(200.0);
    EXPECT_FALSE(result.trip);
    EXPECT_NEAR(result.operatingTimeMs, 50.0, 0.1);

    std::this_thread::sleep_for(std::chrono::milliseconds(70));

    result = prot->update(200.0);
    EXPECT_TRUE(result.trip);
    EXPECT_NEAR(result.measuredCurrentA, 200.0, 0.1);
}

TEST(OvercurrentProtection, ResetWhenBelowPickup)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(50);

    auto prot = OvercurrentProtection::create(settings);

    prot->update(200.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    auto result = prot->update(50.0);
    EXPECT_FALSE(result.trip);

    prot->update(200.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    result = prot->update(200.0);
    EXPECT_FALSE(result.trip);
}

TEST(OvercurrentProtection, DisabledDoesNotTrip)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(10);

    auto prot = OvercurrentProtection::create(settings);
    prot->setEnabled(false);

    prot->update(500.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    auto result = prot->update(500.0);

    EXPECT_FALSE(result.trip);
    EXPECT_FALSE(prot->isEnabled());
}

TEST(OvercurrentProtection, TripCallbackInvoked)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(30);

    auto prot = OvercurrentProtection::create(settings);

    bool callbackFired = false;
    double callbackCurrent = 0.0;
    prot->onTrip([&](const OvercurrentProtectionResult& result)
    {
        callbackFired = true;
        callbackCurrent = result.measuredCurrentA;
    });

    prot->update(300.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    prot->update(300.0);

    EXPECT_TRUE(callbackFired);
    EXPECT_NEAR(callbackCurrent, 300.0, 0.1);
}

TEST(OvercurrentProtection, ResetClearsTimer)
{
    OvercurrentProtectionSettings settings;
    settings.pickupCurrentA = 100.0;
    settings.curveType = OvercurrentCurveType::DefiniteTime;
    settings.definiteTimeDelay = std::chrono::milliseconds(30);

    auto prot = OvercurrentProtection::create(settings);

    prot->update(200.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    prot->reset();

    prot->update(200.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    auto result = prot->update(200.0);
    EXPECT_FALSE(result.trip);
}

TEST(OvercurrentProtection, SetSettingsValidation)
{
    auto prot = OvercurrentProtection::create();

    OvercurrentProtectionSettings invalid;
    invalid.pickupCurrentA = -5.0;
    EXPECT_THROW(prot->setSettings(invalid), std::invalid_argument);

    OvercurrentProtectionSettings valid;
    valid.pickupCurrentA = 50.0;
    valid.timeMultiplier = 0.2;
    valid.curveType = OvercurrentCurveType::ExtremelyInverse;
    EXPECT_NO_THROW(prot->setSettings(valid));

    auto got = prot->getSettings();
    EXPECT_DOUBLE_EQ(got.pickupCurrentA, 50.0);
    EXPECT_EQ(got.curveType, OvercurrentCurveType::ExtremelyInverse);
}

TEST(OvercurrentProtection, EnableDisable)
{
    auto prot = OvercurrentProtection::create();
    EXPECT_TRUE(prot->isEnabled());

    prot->setEnabled(false);
    EXPECT_FALSE(prot->isEnabled());

    prot->setEnabled(true);
    EXPECT_TRUE(prot->isEnabled());
}

TEST(OvercurrentCurveType, ToString)
{
    EXPECT_EQ(overcurrentCurveToString(OvercurrentCurveType::StandardInverse), "Standard Inverse (SI)");
    EXPECT_EQ(overcurrentCurveToString(OvercurrentCurveType::VeryInverse), "Very Inverse (VI)");
    EXPECT_EQ(overcurrentCurveToString(OvercurrentCurveType::ExtremelyInverse), "Extremely Inverse (EI)");
    EXPECT_EQ(overcurrentCurveToString(OvercurrentCurveType::LongTimeInverse), "Long Time Inverse (LTI)");
    EXPECT_EQ(overcurrentCurveToString(OvercurrentCurveType::DefiniteTime), "Definite Time (DT)");
}

TEST(PcapWriter, CreateAndWriteFrames)
{
    const std::string testFile = "/tmp/test_capture.pcap";

    {
        auto writer = sv::PcapWriter::create(testFile);
        ASSERT_TRUE(writer->isOpen());
        EXPECT_EQ(writer->getPacketCount(), 0);

        std::vector<uint8_t> frame = {
            0x01, 0x0C, 0xCD, 0x04, 0x00, 0x01,
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
            0x88, 0xBA,
            0x00, 0x01, 0x02, 0x03
        };
        writer->writeFrame(frame.data(), frame.size());
        EXPECT_EQ(writer->getPacketCount(), 1);

        writer->writeFrame(frame.data(), frame.size());
        EXPECT_EQ(writer->getPacketCount(), 2);

        writer->close();
        EXPECT_FALSE(writer->isOpen());
    }

    std::ifstream file(testFile, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    sv::PcapGlobalHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    EXPECT_EQ(header.magicNumber, 0xA1B2C3D4);
    EXPECT_EQ(header.versionMajor, 2);
    EXPECT_EQ(header.versionMinor, 4);
    EXPECT_EQ(header.network, 1);

    sv::PcapPacketHeader pktHeader;
    file.read(reinterpret_cast<char*>(&pktHeader), sizeof(pktHeader));
    EXPECT_EQ(pktHeader.inclLen, 18);
    EXPECT_EQ(pktHeader.origLen, 18);
    EXPECT_GT(pktHeader.tsSec, 0u);

    file.close();
    std::filesystem::remove(testFile);
}

TEST(PcapWriter, InvalidPathThrows)
{
    EXPECT_THROW(sv::PcapWriter::create("/nonexistent/dir/test.pcap"), std::runtime_error);
}

TEST(PcapWriter, CloseIsIdempotent)
{
    const std::string testFile = "/tmp/test_close.pcap";
    auto writer = sv::PcapWriter::create(testFile);
    writer->close();
    writer->close();
    EXPECT_FALSE(writer->isOpen());
    std::filesystem::remove(testFile);
}

TEST(PcapWriter, WriteAfterCloseIsNoop)
{
    const std::string testFile = "/tmp/test_noop.pcap";
    auto writer = sv::PcapWriter::create(testFile);
    writer->close();

    std::vector<uint8_t> frame(14, 0x00);
    writer->writeFrame(frame.data(), frame.size());
    EXPECT_EQ(writer->getPacketCount(), 0);

    std::filesystem::remove(testFile);
}
