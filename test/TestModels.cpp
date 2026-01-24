#include <gtest/gtest.h>
#include "sv/model/IedModel.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"
#include "sv/core/types.h"
#include "sv/core/mac.h"

TEST(IedModelTest, CreateModel)
{
    const auto model = sv::IedModel::create("TestModel");
    EXPECT_EQ(model->getName(), "TestModel");
}

TEST(IedModelTest, MultipleLogicalNodes)
{
    const auto model = sv::IedModel::create("TestModel");
    const auto ln1 = sv::LogicalNode::create("MU01");
    const auto ln2 = sv::LogicalNode::create("PDIS");
    const auto ln3 = sv::LogicalNode::create("XCBR");
    model->addLogicalNode(ln1);
    model->addLogicalNode(ln2);
    model->addLogicalNode(ln3);
    EXPECT_EQ(model->getLogicalNodes().size(), 3);
    EXPECT_EQ(model->getLogicalNodes()[1]->getName(), "PDIS");
}

TEST(IedModelTest, AddLogicalNode)
{
    const auto model = sv::IedModel::create("TestModel");
    const auto ln = sv::LogicalNode::create("MU01");
    model->addLogicalNode(ln);
    EXPECT_EQ(model->getLogicalNodes().size(), 1);
}

TEST(LogicalNodeTest, MultipleSVCBs)
{
    const auto ln = sv::LogicalNode::create("MU01");
    const auto svcb1 = sv::SampledValueControlBlock::create("SV01");
    const auto svcb2 = sv::SampledValueControlBlock::create("SV02");
    const auto svcb3 = sv::SampledValueControlBlock::create("SV03");
    ln->addSampledValueControlBlock(svcb1);
    ln->addSampledValueControlBlock(svcb2);
    ln->addSampledValueControlBlock(svcb3);
    EXPECT_EQ(ln->getSampledValueControlBlocks().size(), 3);
}

TEST(LogicalNodeTest, CreateNode)
{
    const auto ln = sv::LogicalNode::create("MU01");
    EXPECT_EQ(ln->getName(), "MU01");
}

TEST(LogicalNodeTest, AddSVCB)
{
    const auto ln = sv::LogicalNode::create("MU01");
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    ln->addSampledValueControlBlock(svcb);
    EXPECT_EQ(ln->getSampledValueControlBlocks().size(), 1);
}

TEST(SampledValueControlBlockTest, CreateSVCB)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    EXPECT_EQ(svcb->getName(), "SV01");
}

TEST(SampledValueControlBlockTest, SetAppId)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setAppId(0x4000);
    EXPECT_EQ(svcb->getAppId(), 0x4000);
}

TEST(SampledValueControlBlockTest, SetMulticastAddress)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setMulticastAddress("01:0C:CD:01:00:01");
    EXPECT_EQ(svcb->getMulticastAddress(), "01:0C:CD:01:00:01");
}

TEST(SampledValueControlBlockTest, SetMulticastWithMacAddress)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    constexpr auto mac = sv::MacAddress::svMulticastBase();
    svcb->setMulticastAddress(mac.toString());
    EXPECT_EQ(svcb->getMulticastAddress(), "01:0C:CD:04:00:00");
}

TEST(SampledValueControlBlockTest, SetSmpRate)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setSmpRate(8000);
    EXPECT_EQ(svcb->getSmpRate(), 8000);

    svcb->setSmpRate(4000);
    EXPECT_EQ(svcb->getSmpRate(), 4000);
}

TEST(SampledValueControlBlockTest, SetDataSet)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setDataSet("DataSet1");
    EXPECT_EQ(svcb->getDataSet(), "DataSet1");
}

TEST(SampledValueControlBlockTest, AllParameters)
{
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setAppId(0x4000);
    svcb->setMulticastAddress("01:0C:CD:04:00:01");
    svcb->setSmpRate(4000);
    svcb->setDataSet("PhsMeas1");

    EXPECT_EQ(svcb->getAppId(), 0x4000);
    EXPECT_EQ(svcb->getMulticastAddress(), "01:0C:CD:04:00:01");
    EXPECT_EQ(svcb->getSmpRate(), 4000);
    EXPECT_EQ(svcb->getDataSet(), "PhsMeas1");
}

// ===== Updated Tests for New Type System =====

TEST(TypesTest, AnalogValueInt32)
{
    sv::AnalogValue av;
    av.value = static_cast<int32_t>(123456);
    av.quality = sv::Quality(0);  // Good quality
    av.quality.validity = 0;  // 00 = Good
    av.quality.overflow = false;

    EXPECT_EQ(std::get<int32_t>(av.value), 123456);
    EXPECT_EQ(av.quality.validity, 0);
    EXPECT_FALSE(av.quality.overflow);
    EXPECT_TRUE(av.quality.isGood());
}

TEST(TypesTest, AnalogValueUint32)
{
    sv::AnalogValue av;
    av.value = static_cast<uint32_t>(987654);
    av.quality = sv::Quality(0);
    av.quality.validity = 1;  // 01 = Invalid
    av.quality.overflow = false;

    EXPECT_EQ(std::get<uint32_t>(av.value), 987654);
    EXPECT_EQ(av.quality.validity, 1);
    EXPECT_FALSE(av.quality.isGood());
}

TEST(TypesTest, AnalogValueFloat)
{
    sv::AnalogValue av;
    av.value = 12.34f;
    av.quality = sv::Quality(0);
    av.quality.validity = 0;  // Good
    av.quality.overflow = true;

    EXPECT_FLOAT_EQ(std::get<float>(av.value), 12.34f);
    EXPECT_EQ(av.quality.validity, 0);
    EXPECT_TRUE(av.quality.overflow);
}

TEST(TypesTest, AnalogValueNegative)
{
    sv::AnalogValue av;
    av.value = static_cast<int32_t>(-5000);
    EXPECT_EQ(std::get<int32_t>(av.value), -5000);
    EXPECT_EQ(av.getScaledInt(), -5000);
}

TEST(TypesTest, AnalogValueGetters)
{
    // Test getScaledInt()
    sv::AnalogValue av1;
    av1.value = static_cast<int32_t>(1000);
    EXPECT_EQ(av1.getScaledInt(), 1000);

    av1.value = static_cast<uint32_t>(2000);
    EXPECT_EQ(av1.getScaledInt(), 2000);

    av1.value = 3.5f;
    EXPECT_EQ(av1.getScaledInt(), 3);

    // Test getFloat()
    sv::AnalogValue av2;
    av2.value = static_cast<int32_t>(1000);
    EXPECT_FLOAT_EQ(av2.getFloat(), 1000.0f);

    av2.value = static_cast<uint32_t>(2000);
    EXPECT_FLOAT_EQ(av2.getFloat(), 2000.0f);

    av2.value = 3.5f;
    EXPECT_FLOAT_EQ(av2.getFloat(), 3.5f);
}

TEST(TypesTest, QualityFlags)
{
    sv::Quality q(0);  // Initialize to all zeros

    // Test validity field (2 bits)
    q.validity = 0;  // Good
    EXPECT_TRUE(q.isGood());

    q.validity = 1;  // Invalid
    EXPECT_FALSE(q.isGood());

    q.validity = 3;  // Questionable
    EXPECT_FALSE(q.isGood());

    // Test other boolean flags
    q.overflow = true;
    EXPECT_TRUE(q.overflow);

    q.outOfRange = true;
    EXPECT_TRUE(q.outOfRange);

    q.badReference = true;
    EXPECT_TRUE(q.badReference);

    q.oscillatory = true;
    EXPECT_TRUE(q.oscillatory);

    q.failure = true;
    EXPECT_TRUE(q.failure);

    q.oldData = true;
    EXPECT_TRUE(q.oldData);

    q.inconsistent = true;
    EXPECT_TRUE(q.inconsistent);

    q.inaccurate = true;
    EXPECT_TRUE(q.inaccurate);

    q.source = true;  // Substituted
    EXPECT_TRUE(q.source);

    q.test = true;
    EXPECT_TRUE(q.test);

    q.operatorBlocked = true;
    EXPECT_TRUE(q.operatorBlocked);

    q.derived = true;
    EXPECT_TRUE(q.derived);
}

TEST(TypesTest, QualityRawConversion)
{
    // Test conversion to/from raw uint32_t
    sv::Quality q1(0x12345678);
    EXPECT_EQ(q1.toRaw(), 0x12345678);

    // Test that modifying fields updates raw value
    sv::Quality q2(0);
    q2.validity = 1;     // bit 0-1
    q2.overflow = true;  // bit 2
    q2.test = true;      // bit 11

    const uint32_t raw = q2.toRaw();
    EXPECT_NE(raw, 0);

    // Reconstruct from raw and verify
    sv::Quality q3(raw);
    EXPECT_EQ(q3.validity, 1);
    EXPECT_TRUE(q3.overflow);
    EXPECT_TRUE(q3.test);
}

TEST(TypesTest, ASDUConstruction)
{
    sv::ASDU asdu;
    asdu.svID = "TestSV";
    asdu.smpCnt = 100;
    asdu.confRev = 2;
    asdu.smpSynch = sv::SmpSynch::Local;
    asdu.dataSet.resize(sv::VALUES_PER_ASDU);

    for (size_t i = 0; i < sv::VALUES_PER_ASDU; ++i)
    {
        asdu.dataSet[i].value = static_cast<int32_t>(i * 1000);
        asdu.dataSet[i].quality = sv::Quality(0);
        asdu.dataSet[i].quality.validity = 0;
        asdu.dataSet[i].quality.overflow = false;
    }

    asdu.timestamp = std::chrono::system_clock::now();

    EXPECT_EQ(asdu.svID, "TestSV");
    EXPECT_EQ(asdu.smpCnt, 100);
    EXPECT_EQ(asdu.confRev, 2);
    EXPECT_EQ(asdu.smpSynch, sv::SmpSynch::Local);
    EXPECT_EQ(asdu.dataSet.size(), sv::VALUES_PER_ASDU);
    EXPECT_EQ(std::get<int32_t>(asdu.dataSet[0].value), 0);
    EXPECT_EQ(std::get<int32_t>(asdu.dataSet[7].value), 7000);
    EXPECT_TRUE(asdu.isValid());
}

TEST(TypesTest, ASDUSynchronizationTypes)
{
    sv::ASDU asdu;
    asdu.svID = "SyncTest";
    asdu.smpCnt = 0;
    asdu.confRev = 1;
    asdu.dataSet.resize(sv::VALUES_PER_ASDU);

    // Test None
    asdu.smpSynch = sv::SmpSynch::None;
    EXPECT_EQ(static_cast<uint8_t>(asdu.smpSynch), 0);

    // Test Local
    asdu.smpSynch = sv::SmpSynch::Local;
    EXPECT_EQ(static_cast<uint8_t>(asdu.smpSynch), 1);

    // Test Global
    asdu.smpSynch = sv::SmpSynch::Global;
    EXPECT_EQ(static_cast<uint8_t>(asdu.smpSynch), 2);
}

TEST(TypesTest, ASDUValidation)
{
    sv::ASDU asdu;

    // Invalid: empty svID
    asdu.svID = "";
    asdu.dataSet.resize(sv::VALUES_PER_ASDU);
    EXPECT_FALSE(asdu.isValid());

    // Invalid: svID too short
    asdu.svID = "X";
    EXPECT_FALSE(asdu.isValid());

    // Invalid: wrong number of values
    asdu.svID = "Valid";
    asdu.dataSet.resize(4);
    EXPECT_FALSE(asdu.isValid());

    // Valid
    asdu.svID = "ValidSVID";
    asdu.dataSet.resize(sv::VALUES_PER_ASDU);
    EXPECT_TRUE(asdu.isValid());
}

TEST(TypesTest, ASDUWithGrandmasterIdentity)
{
    sv::ASDU asdu;
    asdu.svID = "TestWithGM";
    asdu.smpCnt = 0;
    asdu.confRev = 1;
    asdu.smpSynch = sv::SmpSynch::Global;
    asdu.dataSet.resize(sv::VALUES_PER_ASDU);

    // Add grandmaster identity
    asdu.gmIdentity = std::array<uint8_t, 8>{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    EXPECT_TRUE(asdu.gmIdentity.has_value());
    EXPECT_EQ(asdu.gmIdentity.value()[0], 0x01);
    EXPECT_EQ(asdu.gmIdentity.value()[7], 0x08);
}

TEST(TypesTest, SVMessageValidation)
{
    sv::SVMessage msg;

    // Invalid: no ASDUs
    EXPECT_FALSE(msg.isValid());

    // Invalid: too many ASDUs
    msg.asdus.resize(sv::MAX_ASDUS_PER_MESSAGE + 1);
    EXPECT_FALSE(msg.isValid());

    // Invalid: APPID out of range
    msg.asdus.resize(1);
    msg.appID = 0x3FFF;  // Below APP_ID_MIN
    EXPECT_FALSE(msg.isValid());

    msg.appID = 0x8000;  // Above APP_ID_MAX
    EXPECT_FALSE(msg.isValid());

    // Valid message
    msg.appID = sv::DEFAULT_APP_ID;
    msg.asdus[0].svID = "TestSV";
    msg.asdus[0].dataSet.resize(sv::VALUES_PER_ASDU);
    EXPECT_TRUE(msg.isValid());
}

TEST(TypesTest, ScalingFactors)
{
    EXPECT_EQ(sv::ScalingFactors::CURRENT_DEFAULT, 1000);
    EXPECT_EQ(sv::ScalingFactors::VOLTAGE_DEFAULT, 100);
}

TEST(TypesTest, PublisherConfig)
{
    sv::PublisherConfig config;

    // Check defaults
    EXPECT_EQ(config.userPriority, 4);
    EXPECT_EQ(config.vlanID, 0);
    EXPECT_EQ(config.appID, sv::DEFAULT_APP_ID);
    EXPECT_FALSE(config.simulate);
    EXPECT_EQ(config.smpSynch, sv::SmpSynch::None);
    EXPECT_EQ(config.currentScaling, sv::ScalingFactors::CURRENT_DEFAULT);
    EXPECT_EQ(config.voltageScaling, sv::ScalingFactors::VOLTAGE_DEFAULT);
    EXPECT_EQ(config.dataType, sv::DataType::INT32);
}

TEST(TypesTest, SubscriberConfig)
{
    sv::SubscriberConfig config;

    // Check defaults
    EXPECT_EQ(config.appID, sv::DEFAULT_APP_ID);
    EXPECT_EQ(config.currentScaling, sv::ScalingFactors::CURRENT_DEFAULT);
    EXPECT_EQ(config.voltageScaling, sv::ScalingFactors::VOLTAGE_DEFAULT);
    EXPECT_EQ(config.dataType, sv::DataType::INT32);
}