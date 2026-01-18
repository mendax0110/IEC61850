#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <optional>
#include <span>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief MAC Address representation \class MacAddress
    class MacAddress
    {
    public:
        static constexpr size_t LENGTH = 6;

        /**
         * @brief Default constructor initializes MAC address to zero.
         */
        constexpr MacAddress() : bytes_{} {}

        /**
         * @brief Constructor from raw bytes.
         * @param bytes The array of 6 bytes representing the MAC address.
         */
        constexpr explicit MacAddress(const std::array<uint8_t, LENGTH>& bytes)
            : bytes_(bytes)
        {
        }

        /**
         * @brief Constructor from span of bytes.
         * @param data Span of bytes (must be exactly 6 bytes).
         * @throws std::invalid_argument if span size is not 6.
         */
        explicit MacAddress(std::span<const uint8_t> data)
        {
            if (data.size() != LENGTH)
            {
                throw std::invalid_argument("MacAddress requires exactly 6 bytes, got " +
                                          std::to_string(data.size()));
            }
            std::copy(data.begin(), data.end(), bytes_.begin());
        }

        /**
         * @brief Constructor from pointer to bytes (unsafe - prefer span version).
         * @param data The pointer to the byte data representing the MAC address.
         * @deprecated Use span constructor for better safety.
         */
        explicit MacAddress(const uint8_t* data)
        {
            if (data == nullptr)
            {
                throw std::invalid_argument("MacAddress data pointer cannot be null");
            }
            std::copy_n(data, LENGTH, bytes_.begin());
        }

        /**
         * @brief Parses a MAC address from a string in the format "xx:xx:xx:xx:xx:xx".
         * @param str The string representation of the MAC address.
         * @return A MacAddress object.
         * @throws std::invalid_argument if the format is invalid.
         */
        [[nodiscard]] static MacAddress parse(std::string_view str)
        {
            auto result = tryParse(str);
            if (!result.has_value())
            {
                throw std::invalid_argument("Invalid MAC address format: " + std::string(str));
            }
            return result.value();
        }

        /**
         * @brief Tries to parse a MAC address from a string.
         * @param str The string representation of the MAC address.
         * @return Optional containing MacAddress if parsing was successful, std::nullopt otherwise.
         */
        [[nodiscard]] static std::optional<MacAddress> tryParse(std::string_view str)
        {
            std::array<uint8_t, LENGTH> bytes{};
            std::istringstream iss{std::string(str)};
            std::string segment;
            size_t index = 0;

            while (std::getline(iss, segment, ':') && index < LENGTH)
            {
                try
                {
                    const unsigned long value = std::stoul(segment, nullptr, 16);
                    if (value > 0xFF)
                    {
                        return std::nullopt;
                    }
                    bytes[index++] = static_cast<uint8_t>(value);
                }
                catch (const std::exception&)
                {
                    return std::nullopt;
                }
            }

            if (index != LENGTH)
            {
                return std::nullopt;
            }

            return MacAddress(bytes);
        }

        /**
         * @brief Tries to parse a MAC address from a string (legacy API).
         * @param str The string representation of the MAC address.
         * @param result The output MacAddress object if parsing is successful.
         * @return True if parsing was successful, false otherwise.
         * @deprecated Use the std::optional version for better C++ style.
         */
        [[nodiscard]] static bool tryParse(std::string_view str, MacAddress& result)
        {
            auto opt = tryParse(str);
            if (opt.has_value())
            {
                result = opt.value();
                return true;
            }
            return false;
        }

        /**
         * @brief Converts to string representation.
         * @param uppercase If true, uses uppercase hex digits (default: true).
         * @return MAC address as string (e.g., "01:0C:CD:01:00:01" or "01:0c:cd:01:00:01").
         */
        [[nodiscard]] std::string toString(bool uppercase = true) const
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            if (uppercase)
            {
                oss << std::uppercase;
            }

            for (size_t i = 0; i < LENGTH; ++i)
            {
                if (i > 0) oss << ':';
                oss << std::setw(2) << static_cast<int>(bytes_[i]);
            }

            return oss.str();
        }

        /**
         * @brief Gets the raw bytes as a const array reference.
         * @return Reference to the underlying byte array.
         */
        [[nodiscard]] constexpr const std::array<uint8_t, LENGTH>& bytes() const
        {
            return bytes_;
        }

        /**
         * @brief Gets a span view of the bytes.
         * @return Span of the MAC address bytes.
         */
        [[nodiscard]] constexpr std::span<const uint8_t> span() const
        {
            return bytes_;
        }

        /**
         * @brief Gets the raw bytes pointer (const).
         * @return Pointer to 6 bytes.
         */
        [[nodiscard]] constexpr const uint8_t* data() const
        {
            return bytes_.data();
        }

        /**
         * @brief Gets mutable raw bytes pointer.
         * @return Pointer to 6 bytes.
         */
        [[nodiscard]] constexpr uint8_t* data()
        {
            return bytes_.data();
        }

        /**
         * @brief Array subscript operator (const).
         * @param index Index of the byte (0-5).
         * @return The byte at the specified index.
         */
        [[nodiscard]] constexpr uint8_t operator[](size_t index) const
        {
            return bytes_[index];
        }

        /**
         * @brief Array subscript operator (mutable).
         * @param index Index of the byte (0-5).
         * @return Reference to the byte at the specified index.
         */
        [[nodiscard]] constexpr uint8_t& operator[](size_t index)
        {
            return bytes_[index];
        }

        /**
         * @brief Checks if this is a multicast address.
         * @return True if the least significant bit of the first octet is set.
         */
        [[nodiscard]] constexpr bool isMulticast() const
        {
            return (bytes_[0] & 0x01) != 0;
        }

        /**
         * @brief Checks if this is a broadcast address.
         * @return True if all bytes are 0xFF.
         */
        [[nodiscard]] constexpr bool isBroadcast() const
        {
            return std::all_of(bytes_.begin(), bytes_.end(),
                             [](uint8_t b) { return b == 0xFF; });
        }

        /**
         * @brief Checks if this is a zero address.
         * @return True if all bytes are zero.
         */
        [[nodiscard]] constexpr bool isZero() const
        {
            return std::all_of(bytes_.begin(), bytes_.end(),
                             [](uint8_t b) { return b == 0; });
        }

        /**
         * @brief Checks if this is a unicast address.
         * @return True if not multicast.
         */
        [[nodiscard]] constexpr bool isUnicast() const
        {
            return !isMulticast();
        }

        /**
         * @brief Checks if this is a locally administered address.
         * @return True if the second least significant bit of the first octet is set.
         */
        [[nodiscard]] constexpr bool isLocallyAdministered() const
        {
            return (bytes_[0] & 0x02) != 0;
        }

        /**
         * @brief Checks if this is a universally administered address.
         * @return True if not locally administered.
         */
        [[nodiscard]] constexpr bool isUniversallyAdministered() const
        {
            return !isLocallyAdministered();
        }

        /**
         * @brief Equality comparison.
         * @param other Other MAC address.
         * @return True if equal.
         */
        [[nodiscard]] constexpr bool operator==(const MacAddress& other) const = default;

        /**
         * @brief Three-way comparison operator (C++20).
         * @param other Other MAC address.
         * @return Comparison result.
         */
        [[nodiscard]] constexpr auto operator<=>(const MacAddress& other) const = default;

        /**
         * @brief IEC 61850-9-2 Sampled Values multicast base address.
         * @return Base multicast address for SV (01-0C-CD-04-00-00).
         */
        [[nodiscard]] static constexpr MacAddress svMulticastBase()
        {
            return MacAddress(std::array<uint8_t, LENGTH>{0x01, 0x0C, 0xCD, 0x04, 0x00, 0x00});
        }

        /**
         * @brief IEC 61850-8-1 GOOSE multicast base address.
         * @return Base multicast address for GOOSE (01-0C-CD-01-00-00).
         */
        [[nodiscard]] static constexpr MacAddress gooseMulticastBase()
        {
            return MacAddress(std::array<uint8_t, LENGTH>{0x01, 0x0C, 0xCD, 0x01, 0x00, 0x00});
        }

        /**
         * @brief Creates a broadcast MAC address.
         * @return Broadcast address (FF:FF:FF:FF:FF:FF).
         */
        [[nodiscard]] static constexpr MacAddress broadcast()
        {
            return MacAddress(std::array<uint8_t, LENGTH>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
        }

        /**
         * @brief Creates a zero MAC address.
         * @return Zero address (00:00:00:00:00:00).
         */
        [[nodiscard]] static constexpr MacAddress zero()
        {
            return MacAddress();
        }

    private:
        std::array<uint8_t, LENGTH> bytes_;
    };

    /**
     * @brief Stream output operator for MacAddress.
     * @param os Output stream.
     * @param mac MAC address to output.
     * @return Reference to the output stream.
     */
    inline std::ostream& operator<<(std::ostream& os, const MacAddress& mac)
    {
        return os << mac.toString();
    }
}