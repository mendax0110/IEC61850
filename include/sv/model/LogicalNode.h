#pragma once

#include <memory>
#include <string>
#include <vector>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Forward declaration of SampledValueControlBlock \class SampledValueControlBlock
    class SampledValueControlBlock;

    /// @brief Represents a Logical Node in IEC 61850. \class LogicalNode
    class LogicalNode
    {
    public:
        /**
         * @brief Shared pointer type for LogicalNode.
         */
        using Ptr = std::shared_ptr<LogicalNode>;

        /**
         * @brief Creates a new LogicalNode.
         * @param name The name of the logical node.
         * @return A shared pointer to the created LogicalNode.
         */
        static Ptr create(const std::string& name);

        /**
         * @brief Gets the name of the logical node.
         * @return The name.
         */
        const std::string& getName() const;

        /**
         * @brief Adds a SampledValueControlBlock to this logical node.
         * @param svcb The control block to add.
         */
        void addSampledValueControlBlock(const std::shared_ptr<SampledValueControlBlock>& svcb);

        /**
         * @brief Gets the list of SampledValueControlBlocks.
         * @return Vector of shared pointers to SVCBs.
         */
        const std::vector<std::shared_ptr<SampledValueControlBlock>>& getSampledValueControlBlocks() const;

    public:
        /**
         * @brief Constructor is private. Use create() method.
         * @param name The name of the logical node.
         */
        LogicalNode(std::string  name);

        std::string name_;
        std::vector<std::shared_ptr<SampledValueControlBlock>> svcbs_;
    };
}