#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"
#include <memory>
#include <utility>

using namespace sv;

LogicalNode::Ptr LogicalNode::create(const std::string& name)
{
    return Ptr(new LogicalNode(name));
}

LogicalNode::LogicalNode(std::string name)
    : name_(std::move(name))
{
}

const std::string& LogicalNode::getName() const
{
    return name_;
}

void LogicalNode::addSampledValueControlBlock(const std::shared_ptr<SampledValueControlBlock>& svcb)
{
    svcbs_.push_back(svcb);
}

const std::vector<std::shared_ptr<SampledValueControlBlock>>& LogicalNode::getSampledValueControlBlocks() const
{
    return svcbs_;
}