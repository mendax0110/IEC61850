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
        double arcVoltageV{50.0};
        double arcResistanceOhm{0.1};
        double contactGapMm{10.0};
        double dielectricStrengthKVpm{3.0};

        /// @brief Validates the breaker definition
        bool isValid() const
        {
            return openTimeSec > 0.0 && closeTimeSec > 0.0 &&
                   resistanceOhm >= 0.0 && maxCurrentA > 0.0 &&
                   voltageRatingV > 0.0 && powerRatingW > 0.0 &&
                   arcVoltageV >= 0.0 && arcResistanceOhm >= 0.0 &&
                   contactGapMm > 0.0 && dielectricStrengthKVpm > 0.0;
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
         * @return Shared pointer to BreakerModel
         */
        static BreakerPtr create();

        /**
         * @brief Creates a new BreakerModel with custom definition
         * @param definition BreakerDefinition struct
         * @return Shared pointer to BreakerModel
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

        /**
         * @brief Gets the breaker definition
         * @return BreakerDefinition struct
         */
        [[nodiscard]] BreakerDefinition getDefinition() const;

        /**
         * @brief Sets the breaker definition
         * @param definition BreakerDefinition struct
         */
        void setDefinition(const BreakerDefinition& definition);

        /**
         * @brief Gets current resistance (very low when closed, infinite when open)
         * @return Resistance in Ohms
         */
        [[nodiscard]] double getResistance() const;

        /**
         * @brief Gets arc voltage during opening/closing transition
         * @return Arc voltage in Volts
         */
        [[nodiscard]] double getArcVoltage() const;

        /**
         * @brief Gets current flowing through breaker
         * @return Current in Amperes
         */
        [[nodiscard]] double getCurrent() const;

        /**
         * @brief Sets current flowing through breaker
         * @param current Current in Amperes
         */
        void setCurrent(double current);

        /**
         * @brief Checks if breaker is overloaded
         * @return true if current exceeds max rating
         */
        [[nodiscard]] bool isOverloaded() const;

        /**
         * @brief Registers callback for state changes
         * @param callback Function to call on state change
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

        /**
         * @brief Runs a automatic simulation of the Breaker.
         * @param voltageV The voltage across the breaker in Volts.
         * @param nominalCurrentA The nominal current through the breaker in Amperes.
         * @param faultCurrentA The fault current through the breaker in Amperes.
         * @param faultTimeS The time at which the fault occurs in seconds.
         * @param durationS The total duration of the simulation in seconds.
         * @param timeStepS The time step for the simulation in seconds (default is 0.001s).
         * @return A struct representing the Simulationresult.
         */
        SimulationResult runSimulation(
            double voltageV,
            double nominalCurrentA,
            double faultCurrentA,
            double faultTimeS,
            double durationS,
            double timeStepS = 0.001
        );

    private:

        /**
         * @brief Private constructor to enforce use of factory methods
         */
        BreakerModel();

        /**
         * @brief Private constructor with definition
         * @param definition BreakerDefinition struct
         */
        explicit BreakerModel(const BreakerDefinition& definition);

        /// @brief Deleted copy/move constructors and assignment operators
        BreakerModel(const BreakerModel&) = delete;
        BreakerModel& operator=(const BreakerModel&) = delete;
        BreakerModel(BreakerModel&&) noexcept = delete;
        BreakerModel& operator=(BreakerModel&&) noexcept = delete;

        /**
         * @brief Main simulation loop that updates breaker state over time
         */
        void simulationLoop();

        /**
         * @brief Transitions the breaker to a new state and starts timing the transition
         */
        void transitionToState(BreakerState newState);

        /**
         * @brief Updates the breaker state based on elapsed time and current conditions
         */
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