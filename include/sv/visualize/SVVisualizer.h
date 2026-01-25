#pragma once

#include "sv/core/types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <chrono>

/// @brief sv namespace \namespace sv
namespace sv
{
    class SVVisualizer
    {
    public:

        /// @brief Visualizer mode \enum Mode
        enum class Mode
        {
            RealTime,
            Statistics,
            Table,
            CSV
        };

        /**
         * @brief Creates a new SVVisualizer.
         * @param mode The visualization mode.
         * @param csvFile The CSV file path (used only in CSV mode).
         * @return A unique pointer to the created SVVisualizer.
         */
        static std::unique_ptr<SVVisualizer> create(Mode mode = Mode::RealTime, const std::string& csvFile = "");

        /**
         * @brief Destructorf for the SVVisualizer.
         */
        ~SVVisualizer();

        /**
         * @brief Updates the visualizer with a new ASDU.
         * @param asdu The ASDU to visualize.
         */
        void update(const ASDU& asdu);

        /**
         * @brief Clears the visualizer data.
         */
        static void clear();

        /**
         * @brief Prints the collected statistics.
         */
        void printStatistics() const;

        /**
         * @brief Closes the open csv files.
         */
        void close();

    private:

        /**
         * @brief Constructor is private. Use create() method.
         * @param mode The visualization mode.
         * @param csvFile The CSV file path.
         */
        explicit SVVisualizer(Mode mode, const std::string& csvFile);

        /**
         * @brief Updates the realtime
         * @param asdu The ASDU
         */
        void updateRealTime(const ASDU& asdu);

        /**
         * @brief Updates the statistics
         * @param asdu The ASDU
         */
        void updateStatistics(const ASDU& asdu);

        /**
         * @brief Updates the table
         * @param asdu The ASDU
         */
        void updateTable(const ASDU& asdu) const;

        /**
         * @brief Updates the CSV file
         * @param asdu The ASDU
         */
        void updateCSV(const ASDU& asdu);

        /**
         * @brief Creates a waveform string for visualization.
         * @param value The current value.
         * @param min The minimum value.
         * @param max The maximum value.
         * @param width The width of the waveform.
         * @return A string representing the waveform.
         */
        static std::string createWaveform(float value, float min, float max, int width);

        /**
         * @brief Creates a bar string for visualization.
         * @param value The current value.
         * @param min The minimum value.
         * @param max The maximum value.
         * @param width The width of the bar.
         * @return A string representing the bar.
         */
        static std::string createBar(float value, float min, float max, int width);

        Mode mode_;
        std::ofstream csvFile_;

        size_t frameCount_{0};
        uint16_t lastSmpCnt_{0};
        size_t missingFrames_{0};
        std::chrono::time_point<std::chrono::steady_clock> startTime_;

        std::vector<float> currentHistory_;
        std::vector<float> voltageHistory_;
        static constexpr size_t HISTORY_SIZE = 80;
    };
}