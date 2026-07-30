#ifndef BLINDSIDE_EAVESDROPPER_DETECTOR_HPP
#define BLINDSIDE_EAVESDROPPER_DETECTOR_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include "blindside/camera.hpp"
#include "blindside/face_detector.hpp"
#include "blindside/pose_estimator.hpp"
#include <chrono>

namespace blindside {

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
    Config config_;
    FaceDetector& face_detector_;
    PoseEstimator& pose_estimator_;

    bool calibrated_ = false;
    FaceBox primary_calibration_box_;

    // Hysteresis tracking
    bool secondary_gaze_active_ = false;
    std::chrono::system_clock::time_point secondary_gaze_start_time_;
};

} // namespace blindside

#endif // BLINDSIDE_EAVESDROPPER_DETECTOR_HPP
