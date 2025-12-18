#include <gtest/gtest.h>
#include "sv/model/IedModel.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"
#include "sv/core/types.h"

TEST(IedModelTest, CreateModel)
{
    auto model = sv::IedModel::create("TestModel");
    EXPECT_EQ(model->getName(), "TestModel");
}

TEST(IedModelTest, MultipleLogicalNodes)
{
    auto model = sv::IedModel::create("TestModel");
    auto ln1 = sv::LogicalNode::create("MU01");
    auto ln2 = sv::LogicalNode::create("PDIS");
    model->addLogicalNode(ln1);
    model->addLogicalNode(ln2);
    EXPECT_EQ(model->getLogicalNodes().size(), 2);
    EXPECT_EQ(model->getLogicalNodes()[1]->getName(), "PDIS");
}

TEST(LogicalNodeTest, MultipleSVCBs)
{
    auto ln = sv::LogicalNode::create("MU01");
    auto svcb1 = sv::SampledValueControlBlock::create("SV01");
    auto svcb2 = sv::SampledValueControlBlock::create("SV02");
    ln->addSampledValueControlBlock(svcb1);
    ln->addSampledValueControlBlock(svcb2);
    EXPECT_EQ(ln->getSampledValueControlBlocks().size(), 2);
}

TEST(LogicalNodeTest, CreateNode)
{
    auto ln = sv::LogicalNode::create("MU01");
    EXPECT_EQ(ln->getName(), "MU01");
}

TEST(LogicalNodeTest, AddSVCB)
{
    auto ln = sv::LogicalNode::create("MU01");
    auto svcb = sv::SampledValueControlBlock::create("SV01");
    ln->addSampledValueControlBlock(svcb);
    EXPECT_EQ(ln->getSampledValueControlBlocks().size(), 1);
}

TEST(SampledValueControlBlockTest, CreateSVCB)
{
    auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setAppId(0x4000);
    EXPECT_EQ(svcb->getAppId(), 0x4000);
}

TEST(SampledValueControlBlockTest, SetMulticastAddress)
{
    auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setMulticastAddress("01:0C:CD:01:00:01");
    EXPECT_EQ(svcb->getMulticastAddress(), "01:0C:CD:01:00:01");
}

TEST(SampledValueControlBlockTest, SetSmpRate)
{
    auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setSmpRate(8000);
    EXPECT_EQ(svcb->getSmpRate(), 8000);
}

TEST(SampledValueControlBlockTest, SetDataSet)
{
    auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setDataSet("DataSet1");
    EXPECT_EQ(svcb->getDataSet(), "DataSet1");
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

TEST(TypesTest, ASDUConstruction)
{
    sv::ASDU asdu;
    asdu.svID = "TestSV";
    asdu.smpCnt = 100;
    asdu.confRev = 2;
    asdu.smpSynch = true;
    asdu.dataSet.resize(4);
    asdu.dataSet[0].value = static_cast<int16_t>(1);
    asdu.timestamp = std::chrono::system_clock::now();
    EXPECT_EQ(asdu.svID, "TestSV");
    EXPECT_EQ(asdu.smpCnt, 100);
    EXPECT_EQ(asdu.confRev, 2);
    EXPECT_TRUE(asdu.smpSynch);
    EXPECT_EQ(asdu.dataSet.size(), 4);
}