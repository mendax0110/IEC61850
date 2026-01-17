#pragma once

#include <memory>
#include <string>
#include <vector>
#include "sv/model/LogicalNode.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Represents the IED Model in IEC 61850. \class IedModel
    class IedModel
    {
    public:
        /**
         * @brief Shared pointer type for IedModel.
         */
        using Ptr = std::shared_ptr<IedModel>;

        /**
         * @brief Creates a new IedModel.
         * @param name The name of the model.
         * @return A shared pointer to the created IedModel.
         */
        static Ptr create(const std::string& name);

        /**
         * @brief Adds a LogicalNode to the model.
         * @param ln The logical node to add.
         */
        void addLogicalNode(const LogicalNode::Ptr &ln);

        /**
         * @brief Gets the list of LogicalNodes.
         * @return Vector of shared pointers to LogicalNodes.
         */
        [[nodiscard]] const std::vector<LogicalNode::Ptr>& getLogicalNodes() const;

        /**
         * @brief Gets the name of the model.
         * @return The name.
         */
        [[nodiscard]] const std::string& getName() const;

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param name The name of the model.
         */
        explicit IedModel(std::string name);

        std::string name_;
        std::vector<LogicalNode::Ptr> logicalNodes_;
    };
}