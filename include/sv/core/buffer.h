#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <span>
#include <algorithm>
#include <bit>
#include <arpa/inet.h>
#include <endian.h>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Buffer writer for serialization \class BufferWriter
    class BufferWriter
    {
    public:
        /**
         * @brief Constructor with optional initial capacity.
         * @param capacity The initial capacity of the buffer.
         */
        explicit BufferWriter(const size_t capacity = 1500)
        {
            buffer_.reserve(capacity);
        }

        /**
         * @brief Writes a uint8_t to the buffer.
         * @param value The uint8_t value to write.
         */
        void writeUint8(const uint8_t value)
        {
            buffer_.push_back(value);
        }

        /**
         * @brief Writes a uint16_t to the buffer in big-endian order.
         * @param value The uint16_t value to write.
         */
        void writeUint16(const uint16_t value)
        {
            const uint16_t net_value = htobe16(value);
            const auto bytes = std::bit_cast<std::array<uint8_t, 2>>(net_value);
            buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        }

        /**
         * @brief Writes a uint32_t to the buffer in big-endian order.
         * @param value The uint32_t value to write.
         */
        void writeUint32(const uint32_t value)
        {
            const uint32_t net_value = htobe32(value);
            const auto bytes = std::bit_cast<std::array<uint8_t, 4>>(net_value);
            buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        }

        /**
         * @brief Writes a uint64_t to the buffer in big-endian order.
         * @param value The uint64_t value to write.
         */
        void writeUint64(const uint64_t value)
        {
            const uint64_t net_value = htobe64(value);
            const auto bytes = std::bit_cast<std::array<uint8_t, 8>>(net_value);
            buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        }

        /**
         * @brief Writes an int16_t to the buffer in big-endian order.
         * @param value The int16_t value to write.
         */
        void writeInt16(const int16_t value)
        {
            writeUint16(static_cast<uint16_t>(value));
        }

        /**
         * @brief Writes a float to the buffer in IEEE 754 format.
         * @param value The float value to write.
         */
        void writeFloat(const float value)
        {
            const uint32_t asInt = std::bit_cast<uint32_t>(value);
            writeUint32(asInt);
        }

        /**
         * @brief Writes raw bytes to the buffer.
         * @param data The pointer to the byte data.
         * @param length The number of bytes to write.
         */
        void writeBytes(const uint8_t* data, const size_t length)
        {
            buffer_.insert(buffer_.end(), data, data + length);
        }

        /**
         * @brief Writes a span of bytes to the buffer.
         * @param data The span of bytes to write.
         */
        void writeBytes(std::span<const uint8_t> data)
        {
            buffer_.insert(buffer_.end(), data.begin(), data.end());
        }

        /**
         * @brief Writes a fixed-size array of bytes to the buffer.
         * @tparam N The size of the array.
         * @param data The array of bytes to write.
         */
        template<size_t N>
        void writeBytes(const std::array<uint8_t, N>& data)
        {
            buffer_.insert(buffer_.end(), data.begin(), data.end());
        }

        /**
         * @brief Writes a fixed-size string to the buffer, padding with null bytes if necessary.
         * @param str The string to write.
         * @param fixedSize The fixed size of the string field.
         */
        void writeFixedString(const std::string& str, const size_t fixedSize)
        {
            const size_t copyLen = std::min(str.size(), fixedSize);
            buffer_.insert(buffer_.end(), str.begin(), str.begin() + copyLen);
            if (copyLen < fixedSize)
            {
                buffer_.insert(buffer_.end(), fixedSize - copyLen, 0);
            }
        }

        /**
         * @brief Writes a uint16_t at a specific position in the buffer in big-endian order.
         * @param pos The position to write the uint16_t.
         * @param value The uint16_t value to write.
         */
        void writeUint16At(const size_t pos, const uint16_t value)
        {
            if (pos + 2 > buffer_.size())
            {
                throw std::out_of_range("writeUint16At: position out of range");
            }

            const uint16_t net_value = htobe16(value);
            const auto bytes = std::bit_cast<std::array<uint8_t, 2>>(net_value);
            std::copy(bytes.begin(), bytes.end(), buffer_.begin() + pos);
        }

        /**
         * @brief Reserves space in the buffer and returns the starting index.
         * @param bytes The number of bytes to reserve.
         * @return A size_t index where the reserved space starts.
         */
        size_t reserve(const size_t bytes)
        {
            const size_t currentSize = buffer_.size();
            buffer_.resize(currentSize + bytes);
            return currentSize;
        }

        /**
         * @brief Getter for the size of the buffer.
         * @return A size_t representing the current size of the buffer.
         */
        [[nodiscard]] size_t size() const
        {
            return buffer_.size();
        }

        /**
         * @brief Getter for the raw data pointer of the buffer.
         * @return A pointer to the buffer's data.
         */
        [[nodiscard]] const uint8_t* data() const
        {
            return buffer_.data();
        }

        /**
         * @brief Getter for the raw data pointer of the buffer (non-const).
         * @return A pointer to the buffer's data.
         */
        [[nodiscard]] uint8_t* data()
        {
            return buffer_.data();
        }

        /**
         * @brief Gets a span view of the buffer.
         * @return A std::span of the buffer data.
         */
        [[nodiscard]] std::span<const uint8_t> span() const
        {
            return {buffer_.data(), buffer_.size()};
        }

        /**
         * @brief Clears the buffer.
         */
        void clear()
        {
            buffer_.clear();
        }

    private:
        std::vector<uint8_t> buffer_;
    };

    /// @brief Buffer reader for deserialization \class BufferReader
    class BufferReader
    {
    public:
        /**
         * @brief Constructs a BufferReader with given data and length.
         * @param data The pointer to the byte data.
         * @param length The length of the data.
         */
        BufferReader(const uint8_t* data, const size_t length)
            : data_(data)
            , length_(length)
            , pos_(0)
        {

        }

        /**
         * @brief Constructs a BufferReader from a span.
         * @param data The span of byte data.
         */
        explicit BufferReader(std::span<const uint8_t> data)
            : data_(data.data())
            , length_(data.size())
            , pos_(0)
        {

        }

        /**
         * @brief Constructs a BufferReader from a vector.
         * @param data The vector of byte data.
         */
        explicit BufferReader(const std::vector<uint8_t>& data)
            : data_(data.data())
            , length_(data.size())
            , pos_(0)
        {

        }

        /**
         * @brief Reads a uint16_t from the buffer in big-endian order.
         * @return The uint16_t value read.
         */
        uint8_t readUint8()
        {
            if (pos_ >= length_)
            {
                return 0;
            }
            return data_[pos_++];
        }

        /**
         * @brief Reads a uint16_t from the buffer in big-endian order.
         * @return The uint16_t value read.
         */
        uint16_t readUint16()
        {
            if (pos_ + 2 > length_)
            {
                return 0;
            }
            std::array<uint8_t, 2> bytes{};
            std::copy_n(data_ + pos_, 2, bytes.begin());
            pos_ += 2;
            const uint16_t net_value = std::bit_cast<uint16_t>(bytes);
            return be16toh(net_value);
        }

        /**
         * @brief Reads a uint32_t from the buffer in big-endian order.
         * @return The uint32_t value read.
         */
        uint32_t readUint32()
        {
            if (pos_ + 4 > length_)
            {
                return 0;
            }
            std::array<uint8_t, 4> bytes{};
            std::copy_n(data_ + pos_, 4, bytes.begin());
            pos_ += 4;
            const uint32_t net_value = std::bit_cast<uint32_t>(bytes);
            return be32toh(net_value);
        }

        /**
         * @brief Reads a uint64_t from the buffer in big-endian order.
         * @return The uint64_t value read.
         */
        uint64_t readUint64()
        {
            if (pos_ + 8 > length_)
            {
                return 0;
            }
            std::array<uint8_t, 8> bytes{};
            std::copy_n(data_ + pos_, 8, bytes.begin());
            pos_ += 8;
            const uint64_t net_value = std::bit_cast<uint64_t>(bytes);
            return be64toh(net_value);
        }

        /**
         * @brief Reads an int16_t from the buffer in big-endian order.
         * @return A int16_t value read.
         */
        int16_t readInt16()
        {
            return static_cast<int16_t>(readUint16());
        }

        /**
         * @brief Reads a 32-bit float in network byte order.
         * @return Value read.
         */
        float readFloat()
        {
            const uint32_t bits = readUint32();
            return std::bit_cast<float>(bits);
        }

        /**
         * @brief Reads a fixed-size string from the buffer, trimming at the first null byte.
         * @param fixedSize The fixed size of the string field.
         * @return The string read.
         */
        std::string readFixedString(size_t fixedSize)
        {
            if (pos_ + fixedSize > length_)
            {
                fixedSize = length_ - pos_;
            }

            std::string str(reinterpret_cast<const char*>(data_ + pos_), fixedSize);
            pos_ += fixedSize;

            const size_t null_pos = str.find('\0');
            if (null_pos != std::string::npos)
            {
                str.resize(null_pos);
            }

            return str;
        }

        /**
         * @brief Reads raw bytes into a buffer.
         * @param dest Destination buffer.
         * @param length Number of bytes to read.
         * @return Number of bytes actually read.
         */
        size_t readBytes(uint8_t* dest, const size_t length)
        {
            const size_t toRead = std::min(length, remaining());
            std::copy_n(data_ + pos_, toRead, dest);
            pos_ += toRead;
            return toRead;
        }

        /**
         * @brief Reads raw bytes into a vector.
         * @param length Number of bytes to read.
         * @return Vector containing the bytes read.
         */
        [[nodiscard]] std::vector<uint8_t> readBytes(const size_t length)
        {
            const size_t toRead = std::min(length, remaining());
            std::vector<uint8_t> result(data_ + pos_, data_ + pos_ + toRead);
            pos_ += toRead;
            return result;
        }

        /**
         * @brief Skips a number of bytes in the buffer.
         * @param bytes The number of bytes to skip.
         */
        void skip(const size_t bytes)
        {
            pos_ = std::min(pos_ + bytes, length_);
        }

        /**
         * @brief Getter for the current position in the buffer.
         * @return A size_t representing the current position.
         */
        [[nodiscard]] size_t position() const
        {
            return pos_;
        }

        /**
         * @brief Getter for the remaining bytes in the buffer.
         * @return A size_t representing the number of remaining bytes.
         */
        [[nodiscard]] size_t remaining() const
        {
            return length_ > pos_ ? length_ - pos_ : 0;
        }

        /**
         * @brief Checks if there are more bytes to read in the buffer.
         * @return true if more bytes are available, false otherwise.
         */
        [[nodiscard]] bool hasMore() const
        {
            return pos_ < length_;
        }

        /**
         * @brief Resets the reader position to the beginning.
         */
        void reset()
        {
            pos_ = 0;
        }

        /**
         * @brief Sets the reader position.
         * @param pos The new position.
         * @throws std::out_of_range if position is invalid.
         */
        void seek(const size_t pos)
        {
            if (pos > length_)
            {
                throw std::out_of_range("BufferReader::seek: position out of range");
            }
            pos_ = pos;
        }

    private:
        const uint8_t* data_;
        size_t length_;
        size_t pos_;
    };
}