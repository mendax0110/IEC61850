#include "sv/model/IedModel.h"
#include <memory>
#include <utility>

using namespace sv;

IedModel::Ptr IedModel::create(const std::string& name)
{
    return Ptr(new IedModel(name));
}

IedModel::IedModel(std::string name)
    : name_(std::move(name))
{
}

void IedModel::addLogicalNode(const LogicalNode::Ptr &ln)
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