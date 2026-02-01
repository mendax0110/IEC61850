#pragma once

#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <limits>

/// @brief sv namespace \namespace sv::sim
namespace sv::sim
{
    /// @brief Breaker State Enumeration \enum BreakerState
    enum class BreakerState
    {
        OPEN,
        CLOSED,
        OPENING,
        CLOSING,
        LOCKED_OPEN,
        LOCKED_CLOSED
    };

    /// @brief Convert BreakerState to string
    inline const char* toString(const BreakerState state)
    {
        switch (state)
        {
            case BreakerState::OPEN: return "OPEN";
            case BreakerState::CLOSED: return "CLOSED";
            case BreakerState::OPENING: return "OPENING";
            case BreakerState::CLOSING: return "CLOSING";
            case BreakerState::LOCKED_OPEN: return "LOCKED_OPEN";
            case BreakerState::LOCKED_CLOSED: return "LOCKED_CLOSED";
            default: return "UNKNOWN";
        }
    }

    /// @brief Breaker physical characteristics and ratings \struct BreakerDefinition
    struct BreakerDefinition
    {
        double openTimeSec{0.050};
        double closeTimeSec{0.100};
        double resistanceOhm{0.001};
        double maxCurrentA{1000.0};
        double voltageRatingV{400.0};
        double powerRatingW{400000.0};
        double arcDurationSec{0.020};

        /// @brief Validates the breaker definition
        bool isValid() const
        {
            return openTimeSec > 0.0 && closeTimeSec > 0.0 &&
                   resistanceOhm >= 0.0 && maxCurrentA > 0.0 &&
                   voltageRatingV > 0.0 && powerRatingW > 0.0;
        }
    };

    /// @brief Simulation scenario results \struct SimulationResult
    struct SimulationResult
    {
        std::vector<double> timePoints;
        std::vector<double> currentValues;
        std::vector<BreakerState> stateHistory;
        bool tripOccurred{false};
        double tripTime{0.0};
        std::string summary;
    };

    /// @brief Breaker state change callback
    using BreakerCallback = std::function<void(BreakerState oldState, BreakerState newState)>;

    /// @brief Circuit Breaker Simulation Model
    class BreakerModel
    {
    public:
        using BreakerPtr = std::shared_ptr<BreakerModel>;

        /**
         * @brief Creates a new BreakerModel with default definition
         */
        static BreakerPtr create();

        /**
         * @brief Creates a new BreakerModel with custom definition
         */
        static BreakerPtr create(const BreakerDefinition& definition);

        /**
         * @brief Destructor - stops simulation thread
         */
        ~BreakerModel();

        [[nodiscard]] bool isClosed() const;
        [[nodiscard]] bool isOpen() const;
        [[nodiscard]] bool isOpening() const;
        [[nodiscard]] bool isClosing() const;
        [[nodiscard]] bool isLocked() const;
        [[nodiscard]] bool isInTransition() const;
        [[nodiscard]] BreakerState getState() const;

        /**
         * @brief Commands the breaker to open
         * @return true if command accepted, false if locked or already opening
         */
        bool open();

        /**
         * @brief Commands the breaker to close
         * @return true if command accepted, false if locked or already closing
         */
        bool close();

        /**
         * @brief Locks the breaker in current position
         */
        void lock();

        /**
         * @brief Unlocks the breaker
         */
        void unlock();

        /**
         * @brief Emergency trip (immediate opening)
         */
        void trip();

        [[nodiscard]] BreakerDefinition getDefinition() const;
        void setDefinition(const BreakerDefinition& definition);

        /**
         * @brief Gets current resistance (very low when closed, infinite when open)
         */
        [[nodiscard]] double getResistance() const;

        /**
         * @brief Gets current flowing through breaker
         */
        [[nodiscard]] double getCurrent() const;
        void setCurrent(double current);

        /**
         * @brief Checks if breaker is overloaded
         */
        [[nodiscard]] bool isOverloaded() const;

        /**
         * @brief Registers callback for state changes
         */
        void onStateChange(BreakerCallback callback);

        /**
         * @brief Starts the breaker simulation thread
         */
        void startSimulation();

        /**
         * @brief Stops the breaker simulation thread
         */
        void stopSimulation();

        SimulationResult runSimulation(
            double voltageV,
            double nominalCurrentA,
            double faultCurrentA,
            double faultTimeS,
            double durationS,
            double timeStepS = 0.001
        );

    private:
        BreakerModel();
        explicit BreakerModel(const BreakerDefinition& definition);

        BreakerModel(const BreakerModel&) = delete;
        BreakerModel& operator=(const BreakerModel&) = delete;
        BreakerModel(BreakerModel&&) = delete;
        BreakerModel& operator=(BreakerModel&&) = delete;

        void simulationLoop();
        void transitionToState(BreakerState newState);
        void updateState();

        std::atomic<BreakerState> state_{BreakerState::OPEN};
        std::atomic<bool> locked_{false};
        std::atomic<double> currentA_{0.0};

        std::chrono::steady_clock::time_point transitionStartTime_;
        double transitionDuration_{0.0};
        BreakerState targetState_{BreakerState::OPEN};

        BreakerDefinition definition_;
        mutable std::mutex definitionMutex_;

        std::atomic<bool> running_{false};
        std::thread simulationThread_;

        BreakerCallback callback_;
        std::mutex callbackMutex_;
    };
}