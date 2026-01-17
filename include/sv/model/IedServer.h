#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include "sv/model/IedModel.h"
#include "sv/network/NetworkSender.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Forward declaration of SampledValueControlBlock \class SampledValueControlBlock
    class SampledValueControlBlock;

    /// @brief Server for sending IEC 61850 Sampled Values. \class IedServer
    class IedServer
    {
    public:
        /**
         * @brief Shared pointer type for IedServer.
         */
        using Ptr = std::shared_ptr<IedServer>;

        /**
         * @brief Creates a new IedServer.
         * @param model The IED model.
         * @param interface The network interface (empty for auto-detect).
         * @return A shared pointer to the created IedServer.
         */
        static Ptr create(IedModel::Ptr model, const std::string& interface = "");

        /**
         * @brief Starts the server.
         */
        void start();

        /**
         * @brief Stops the server.
         */
        void stop();

        /**
         * @brief Updates the sampled values for a control block.
         * @param svcb The control block.
         * @param values The analog values.
         */
        void updateSampledValue(const std::shared_ptr<SampledValueControlBlock>& svcb, const std::vector<AnalogValue>& values) const;

        /**
         * @brief Gets the model.
         * @return The IED model.
         */
        IedModel::Ptr getModel() const;

    private:
        /**
         * @brief Main sending loop.
         */
        void run() const;

        /**
         * @brief Constructor is private. Use create() method.
         * @param model The IED model.
         * @param interface The network interface.
         */
        IedServer(IedModel::Ptr model, std::string  interface);

        IedModel::Ptr model_;
        std::string interface_;
        std::unique_ptr<NetworkSender> sender_;
        std::thread senderThread_;
        std::atomic<bool> running_;
    };
}