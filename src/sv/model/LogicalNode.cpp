#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"

using namespace sv;

LogicalNode::Ptr LogicalNode::create(const std::string& name)
{
    return std::shared_ptr<LogicalNode>(new LogicalNode(name));
}

LogicalNode::LogicalNode(const std::string& name)
    : name_(name)
{
}

const std::string& LogicalNode::getName() const
{
    return name_;
}

void LogicalNode::addSampledValueControlBlock(std::shared_ptr<SampledValueControlBlock> svcb)
{
    svcbs_.push_back(svcb);
}

const std::vector<std::shared_ptr<SampledValueControlBlock>>& LogicalNode::getSampledValueControlBlocks() const
{
    return svcbs_;
}