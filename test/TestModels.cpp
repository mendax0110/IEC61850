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

TEST(TypesTest, AnalogValueInt16)
{
    sv::AnalogValue av;
    av.value = static_cast<int16_t>(1234);
    av.quality.validity = true;
    av.quality.overflow = false;
    EXPECT_EQ(std::get<int16_t>(av.value), 1234);
    EXPECT_TRUE(av.quality.validity);
    EXPECT_FALSE(av.quality.overflow);
}

TEST(TypesTest, AnalogValueFloat)
{
    sv::AnalogValue av;
    av.value = 12.34f;
    av.quality.validity = false;
    av.quality.overflow = true;
    EXPECT_FLOAT_EQ(std::get<float>(av.value), 12.34f);
    EXPECT_FALSE(av.quality.validity);
    EXPECT_TRUE(av.quality.overflow);
}

TEST(TypesTest, AnalogValueNegative)
{
    sv::AnalogValue av;
    av.value = static_cast<int16_t>(-5000);
    EXPECT_EQ(std::get<int16_t>(av.value), -5000);
}

TEST(TypesTest, QualityFlags)
{
    sv::Quality q{};
    q.validity = true;
    q.overflow = false;
    EXPECT_TRUE(q.validity);
    EXPECT_FALSE(q.overflow);

    q.validity = false;
    q.overflow = true;
    EXPECT_FALSE(q.validity);
    EXPECT_TRUE(q.overflow);
}

TEST(TypesTest, ASDUConstruction)
{
    sv::ASDU asdu;
    asdu.svID = "TestSV";
    asdu.smpCnt = 100;
    asdu.confRev = 2;
    asdu.smpSynch = true;
    asdu.dataSet.resize(8);

    for (size_t i = 0; i < 8; ++i)
    {
        asdu.dataSet[i].value = static_cast<int16_t>(i * 100);
        asdu.dataSet[i].quality.validity = true;
        asdu.dataSet[i].quality.overflow = false;
    }

    asdu.timestamp = std::chrono::system_clock::now();

    EXPECT_EQ(asdu.svID, "TestSV");
    EXPECT_EQ(asdu.smpCnt, 100);
    EXPECT_EQ(asdu.confRev, 2);
    EXPECT_TRUE(asdu.smpSynch);
    EXPECT_EQ(asdu.dataSet.size(), 8);
    EXPECT_EQ(std::get<int16_t>(asdu.dataSet[0].value), 0);
    EXPECT_EQ(std::get<int16_t>(asdu.dataSet[7].value), 700);
}