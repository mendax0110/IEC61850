#include "sv/model/IedModel.h"

using namespace sv;

IedModel::Ptr IedModel::create(const std::string& name)
{
    return std::shared_ptr<IedModel>(new IedModel(name));
}

IedModel::IedModel(const std::string& name)
    : name_(name)
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