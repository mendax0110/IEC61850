#include <iostream>
#include <thread>

#include "sv/model/IedModel.h"
#include "sv/model/IedServer.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"

int main()
{
    auto model = sv::IedModel::create("SVModel");

    auto ln = sv::LogicalNode::create("MU01");
    model->addLogicalNode(ln);

    auto svcb = sv::SampledValueControlBlock::create("SV01");
    svcb->setMulticastAddress("01:0C:CD:01:00:01");
    svcb->setAppId(0x4000);
    svcb->setSmpRate(4000);
    ln->addSampledValueControlBlock(svcb);

    auto server = sv::IedServer::create(model, "veth0");
    server->start();

    //start simulation...
    std::vector<sv::AnalogValue> values(8);
    for (int i = 0; i < 8; ++i)
    {
        values[i].value = static_cast<int16_t>(i * 100);
        values[i].quality.validity = true;
        values[i].quality.overflow = false;
    }

    for (int i = 0; i < 10; ++i)
    {
        server->updateSampledValue(svcb, values);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    server->stop();

    return 0;
}