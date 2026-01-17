#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <stdexcept>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief MAC Address representation \class MacAddress
    class MacAddress
    {
    public:
        static constexpr size_t LENGTH = 6;

        /**
         * @brief Default constructor initializes MAC address zero.
         */
        MacAddress()
        {
            bytes_.fill(0);
        }

        /**
         * @brief Constructor from raw bytes.
         * @param bytes The array of 6 bytes representing the MAC address.
         */
        explicit MacAddress(const std::array<uint8_t, LENGTH>& bytes)
            : bytes_(bytes)
        {
        }

        /**
         * @brief Constructor from pointer to bytes.
         * @param data The pointer to the byte data representing the MAC address.
         */
        explicit MacAddress(const uint8_t* data)
        {
            std::memcpy(bytes_.data(), data, LENGTH);
        }

        /**
         * @brief Parses a MAC address from a string in the format "xx:xx:xx:xx:xx:xx".
         * @param str The string representation of the MAC address.
         * @return A MacAddress object.
         */
        static MacAddress parse(const std::string& str)
        {
            MacAddress mac;
            if (std::sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                           &mac.bytes_[0], &mac.bytes_[1], &mac.bytes_[2],
                           &mac.bytes_[3], &mac.bytes_[4], &mac.bytes_[5]) != 6)
            {
                throw std::invalid_argument("Invalid MAC address format: " + str);
            }
            return mac;
        }

        /**
         * @brief Tries to parse a MAC address from a string.
         * @param str The string representation of the MAC address.
         * @param result The output MacAddress object if parsing is successful.
         * @return True if parsing was successful, false otherwise.
         */
        static bool tryParse(const std::string& str, MacAddress& result)
        {
            return std::sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                              &result.bytes_[0], &result.bytes_[1], &result.bytes_[2],
                              &result.bytes_[3], &result.bytes_[4], &result.bytes_[5]) == 6;
        }

        /**
         * @brief Converts to string representation.
         * @return MAC address as string (e.g., "01:0C:CD:01:00:01").
         */
        std::string toString() const
        {
            char buf[18];
            std::snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                         bytes_[0], bytes_[1], bytes_[2],
                         bytes_[3], bytes_[4], bytes_[5]);
            return std::string(buf);
        }

        /**
         * @brief Gets the raw bytes.
         * @return Pointer to 6 bytes.
         */
        const uint8_t* data() const
        {
            return bytes_.data();
        }

        /**
         * @brief Gets mutable raw bytes.
         * @return Pointer to 6 bytes.
         */
        uint8_t* data()
        {
            return bytes_.data();
        }

        /**
         * @brief Checks if this is a multicast address.
         * @return True if multicast.
         */
        bool isMulticast() const
        {
            return (bytes_[0] & 0x01) != 0;
        }

        /**
         * @brief Checks if this is a broadcast address.
         * @return True if broadcast.
         */
        bool isBroadcast() const
        {
            return bytes_[0] == 0xFF && bytes_[1] == 0xFF && bytes_[2] == 0xFF &&
                   bytes_[3] == 0xFF && bytes_[4] == 0xFF && bytes_[5] == 0xFF;
        }

        /**
         * @brief Checks if this is a zero address.
         * @return True if all zeros.
         */
        bool isZero() const
        {
            return bytes_[0] == 0 && bytes_[1] == 0 && bytes_[2] == 0 &&
                   bytes_[3] == 0 && bytes_[4] == 0 && bytes_[5] == 0;
        }

        /**
         * @brief Equality comparison.
         * @param other Other MAC address.
         * @return True if equal.
         */
        bool operator==(const MacAddress& other) const
        {
            return bytes_ == other.bytes_;
        }

        /**
         * @brief Inequality comparison.
         * @param other Other MAC address.
         * @return True if not equal.
         */
        bool operator!=(const MacAddress& other) const
        {
            return bytes_ != other.bytes_;
        }

        /**
         * @brief IEC 61850 SV multicast base address.
         * @return Base multicast address for SV.
         */
        static MacAddress svMulticastBase()
        {
            return MacAddress({{0x01, 0x0C, 0xCD, 0x01, 0x00, 0x00}});
        }

        /**
         * @brief IEC 61850 GOOSE multicast base address.
         * @return Base multicast address for GOOSE.
         */
        static MacAddress gooseMulticastBase()
        {
            return MacAddress({{0x01, 0x0C, 0xCD, 0x01, 0x00, 0x00}});
        }


    private:
        std::array<uint8_t, LENGTH> bytes_;
    };
}