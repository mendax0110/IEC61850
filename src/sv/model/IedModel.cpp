#include "sv/model/IedModel.h"

#include <memory>
#include <utility>

using namespace sv;

IedModel::Ptr IedModel::create(const std::string& name)
{
    return std::make_shared<IedModel>(name);
}

IedModel::IedModel(std::string  name)
    : name_(std::move(name))
{
}

void IedModel::addLogicalNode(LogicalNode::Ptr ln)
{
    logicalNodes_.push_back(ln);
}

const std::vector<LogicalNode::Ptr>& IedModel::getLogicalNodes() const
{
    return logicalNodes_;
}

const std::string& IedModel::getName() const
{
    return name_;
}