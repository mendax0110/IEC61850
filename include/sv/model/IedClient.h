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
        /**
         * @brief Shared pointer type for IedClient.
         */
        using Ptr = std::shared_ptr<IedClient>;

        /**
         * @brief Callback type for received ASDUs.
         */
        using ASDUCallback = std::function<void(const ASDU&)>;

        /**
         * @brief Creates a new IedClient.
         * @param model The IED model.
         * @param interface The network interface (empty for auto-detect).
         * @return A shared pointer to the created IedClient.
         */
        static Ptr create(IedModel::Ptr model, const std::string& interface = "");

        /**
         * @brief Starts the client with a custom callback.
         * @param callback Function to call for each received ASDU.
         */
        void start(const ASDUCallback& callback);

        /**
         * @brief Starts the client with default callback (stores ASDUs).
         */
        void start();

        /**
         * @brief Stops the client.
         */
        void stop() const;

        /**
         * @brief Receives sampled values.
         * @return A vector of received ASDUs.
         */
        std::vector<ASDU> receiveSampledValues();

        /**
         * @brief Gets the model.
         * @return The IED model.
         */
        [[nodiscard]] IedModel::Ptr getModel() const;

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