#include <gtest/gtest.h>
#include "sv/model/IedModel.h"
#include "sv/model/IedServer.h"
#include "sv/model/IedClient.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"

TEST(IedServerTest, CreateServer)
{
    const auto model = sv::IedModel::create("TestModel");
    const auto server = sv::IedServer::create(model, "lo"); // Use loopback for testing
    EXPECT_NE(server, nullptr);
    // Note: Avoid starting/stopping in unit tests to prevent thread issues
}

TEST(IedClientTest, CreateClient)
{
    const auto model = sv::IedModel::create("TestModel");
    const auto client = sv::IedClient::create(model, "lo");
    EXPECT_NE(client, nullptr);
}

TEST(IntegrationTest, ModelWithServerAndClient)
{
    const auto model = sv::IedModel::create("IntegrationModel");
    const auto ln = sv::LogicalNode::create("MU01");
    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setAppId(0x4000);
    svcb->setMulticastAddress("01:0C:CD:01:00:01");
    svcb->setSmpRate(4000);
    ln->addSampledValueControlBlock(svcb);
    model->addLogicalNode(ln);

    const auto server = sv::IedServer::create(model, "lo");
    const auto client = sv::IedClient::create(model, "lo");

    EXPECT_EQ(model->getLogicalNodes().size(), 1);
    EXPECT_EQ(model->getLogicalNodes()[0]->getSampledValueControlBlocks().size(), 1);
    EXPECT_EQ(server->getModel()->getName(), "IntegrationModel");
    EXPECT_EQ(client->getModel()->getName(), "IntegrationModel");
}