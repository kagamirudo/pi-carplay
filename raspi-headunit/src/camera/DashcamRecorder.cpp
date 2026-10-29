#include "camera/DashcamRecorder.hpp"

namespace headunit {

const char* recordStateName(RecordState state) {
    switch (state) {
        case RecordState::Idle:
            return "idle";
        case RecordState::Recording:
            return "recording";
        case RecordState::Stopping:
            return "stopping";
        case RecordState::Error:
            return "error";
    }
    return "idle";
}

const char* focusBiasName(FocusBias bias) {
    switch (bias) {
        case FocusBias::Auto:
            return "auto";
        case FocusBias::Distance:
            return "distance";
    }
    return "distance";
}

DashcamRecorder::DashcamRecorder(DashcamConfig config) { apply(std::move(config)); }

const DashcamStatus& DashcamRecorder::status() const { return status_; }

void DashcamRecorder::apply(const DashcamConfig& config) {
    config_ = config;
    status_.width = config.width;
    status_.height = config.height;
    status_.fps = config.fps;
    status_.segmentSeconds = config.segmentSeconds;
    status_.focus = FocusBias::Distance;
    status_.state = RecordState::Idle;
    status_.encoderStarted = false;
    status_.detail = "Recorder is idle. No encoder is running.";
}

bool DashcamRecorder::start() {
    status_.state = RecordState::Idle;
    status_.encoderStarted = false;
    status_.lastClip.clear();
    if (config_.clipsDirectory.empty() || config_.storageLimitBytes == 0) {
        status_.detail = "No footage disk is configured. Refusing to record on the OS card.";
    } else {
        status_.detail = "Recorder is not implemented. No clip is written.";
    }
    return false;
}

void DashcamRecorder::stop() {
    status_.state = RecordState::Idle;
    status_.encoderStarted = false;
    status_.detail = "Recorder is idle. No encoder is running.";
}

}  // namespace headunit
