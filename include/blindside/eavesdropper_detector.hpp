#ifndef BLINDSIDE_EAVESDROPPER_DETECTOR_HPP
#define BLINDSIDE_EAVESDROPPER_DETECTOR_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include "blindside/camera.hpp"
#include "blindside/face_detector.hpp"
#include "blindside/pose_estimator.hpp"
#include <chrono>
#include <array>

namespace blindside {

struct PoseHistorySample {
    std::chrono::system_clock::time_point timestamp;
    double pitch = 0.0;
    double yaw = 0.0;
    float ear = 0.30f;
    bool valid = false;
};

class EavesdropperDetector {
public:
    EavesdropperDetector(const Config& config, 
                         FaceDetector& detector, 
                         PoseEstimator& estimator);
    ~EavesdropperDetector() = default;

    /**
     * @brief Calibrates the primary user's baseline face position (center of webcam field of view).
     */
    bool calibrate(const RawFrame& frame);

    /**
     * @brief Analyzes a frame for primary user presence and background shoulder surfers.
     */
    FrameResult process_frame(const RawFrame& frame);

    /**
     * @brief Resets the hysteresis timer and state.
     */
    void reset_state();

    bool is_calibrated() const { return calibrated_; }
    FaceBox get_primary_calibration_box() const { return primary_calibration_box_; }

private:
    /**
     * @brief Evaluates zero-allocation rolling 3-second history for micro-motion and blink verification.
     */
    bool evaluate_liveness(double current_pitch, double current_yaw, float current_ear, bool& is_spoof_photo);

    Config config_;
    FaceDetector& face_detector_;
    PoseEstimator& pose_estimator_;

    bool calibrated_ = false;
    FaceBox primary_calibration_box_;

    // Hysteresis tracking
    bool secondary_gaze_active_ = false;
    std::chrono::system_clock::time_point secondary_gaze_start_time_;

    // Fixed zero-allocation rolling history for liveness micro-motion check
    static constexpr size_t HISTORY_CAPACITY = 45;
    std::array<PoseHistorySample, HISTORY_CAPACITY> pose_history_{};
    size_t history_head_ = 0;
    size_t history_count_ = 0;
};

} // namespace blindside

#endif // BLINDSIDE_EAVESDROPPER_DETECTOR_HPP
