#pragma once

#include <memory>
#include <mutex>
#include "sv/model/IedModel.h"
#include "sv/network/NetworkReceiver.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Client for receiving IEC 61850 Sampled Values. \class IedClient
    class IedClient
    {
    public:
        using Ptr = std::shared_ptr<IedClient>;

        /**
         * @brief Creates a new IedClient.
         * @param model The IED model.
         * @param interface The network interface (empty for auto-detect).
         * @return A shared pointer to the created IedClient.
         */
        static Ptr create(IedModel::Ptr model, const std::string& interface = "");

        /**
         * @brief Starts the client.
         */
        void start();

        /**
         * @brief Stops the client.
         */
        void stop();

        /**
         * @brief Receives sampled values.
         * @return A vector of received ASDUs.
         */
        std::vector<ASDU> receiveSampledValues();

        /**
         * @brief Gets the model.
         * @return The IED model.
         */
        IedModel::Ptr getModel() const;

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param model The IED model.
         * @param interface The network interface.
         */
        IedClient(IedModel::Ptr model, std::string  interface);

        IedModel::Ptr model_;
        std::string interface_;
        std::unique_ptr<NetworkReceiver> receiver_;
        std::vector<ASDU> receivedASDUs_;
        std::mutex receivedMutex_;
    };
}