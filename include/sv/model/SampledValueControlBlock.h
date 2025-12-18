#pragma once

#include <memory>
#include <string>
#include <vector>
#include "sv/core/types.h"

/// @brief sv namespace \namespace sv
namespace sv
{

    /// @brief Represents a Sampled Value Control Block in IEC 61850. \class SampledValueControlBlock
    class SampledValueControlBlock
    {
    public:
        /**
         * @brief Shared pointer type for SampledValueControlBlock.
         */
        using Ptr = std::shared_ptr<SampledValueControlBlock>;

        /**
         * @brief Creates a new SampledValueControlBlock.
         * @param name The name of the control block.
         * @return A shared pointer to the created SVCB.
         */
        static Ptr create(const std::string& name);

        /**
         * @brief Sets the multicast address.
         * @param address The MAC address as string (e.g., "01:0C:CD:01:00:01").
         */
        void setMulticastAddress(const std::string& address);

        /**
         * @brief Gets the multicast address.
         * @return The MAC address.
         */
        const std::string& getMulticastAddress() const;

        /**
         * @brief Sets the AppID.
         * @param appId The 16-bit AppID.
         */
        void setAppId(uint16_t appId = DEFAULT_APP_ID);

        /**
         * @brief Gets the AppID.
         * @return The AppID.
         */
        uint16_t getAppId() const;

        /**
         * @brief Sets the sampling rate.
         * @param rate The sampling rate in Hz.
         */
        void setSmpRate(uint16_t rate);

        /**
         * @brief Gets the sampling rate.
         * @return The sampling rate.
         */
        uint16_t getSmpRate() const;

        /**
         * @brief Sets the DataSet reference.
         * @param dataSet The name of the DataSet.
         */
        void setDataSet(const std::string& dataSet);

        /**
         * @brief Gets the DataSet reference.
         * @return The DataSet name.
         */
        const std::string& getDataSet() const;

    public:
        /**
         * @brief Constructor is private. Use create() method.
         * @param name The name of the control block.
         */
        SampledValueControlBlock(const std::string& name);

        std::string name_;
        std::string multicastAddress_;
        uint16_t appId_;
        uint16_t smpRate_;
        std::string dataSet_;
    };
}