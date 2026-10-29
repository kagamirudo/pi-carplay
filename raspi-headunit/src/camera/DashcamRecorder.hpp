#pragma once

#include "core/ConfigurationManager.hpp"

#include <string>

namespace headunit {

enum class RecordState { Idle, Recording, Stopping, Error };
enum class FocusBias { Auto, Distance };

struct DashcamStatus {
    RecordState state = RecordState::Idle;
    int width = 1920;
    int height = 1080;
    int fps = 30;
    int segmentSeconds = 120;
    FocusBias focus = FocusBias::Distance;
    std::string lastClip;
    std::string detail = "Recorder has not been started.";
    bool encoderStarted = false;
};

class DashcamRecorder {
public:
    explicit DashcamRecorder(DashcamConfig config);
    const DashcamStatus& status() const;
    void apply(const DashcamConfig& config);
    // Does not open the camera or write a file.
    bool start();
    void stop();

private:
    DashcamConfig config_;
    DashcamStatus status_;
};

const char* recordStateName(RecordState state);
const char* focusBiasName(FocusBias bias);

}  // namespace headunit
