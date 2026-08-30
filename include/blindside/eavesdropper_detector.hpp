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

struct SecondaryFaceTrack {
    size_t id = 0;
    float norm_center_x = 0.0f;
    float norm_center_y = 0.0f;
    float norm_width = 0.0f;
    float norm_height = 0.0f;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    size_t hit_count = 0;
    size_t miss_count = 0;
};

class EavesdropperDetector {
public:
    EavesdropperDetector(const Config& config, 
                         IFaceDetector& detector, 
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
     * @brief Resets the hysteresis timer and state without clearing liveness history.
     */
    void reset_trigger_state();

    bool is_calibrated() const { return calibrated_; }
    FaceBox get_primary_calibration_box() const { return primary_calibration_box_; }

    /**
     * @brief Validates physical bounding box dimensions (frame-relative sizing, aspect ratio, landmarks).
     */
    bool is_valid_face_box(const FaceBox& box, int frame_width, int frame_height) const;

private:
    /**
     * @brief Evaluates zero-allocation rolling 3-second history for micro-motion and blink verification.
     */
    bool evaluate_liveness(double current_pitch, double current_yaw, float current_ear, bool& is_spoof_photo);

    /**
     * @brief Updates temporal persistence tracking for a candidate secondary face. Returns true if validated.
     */
    bool update_and_validate_secondary_track(const FaceBox& box, int frame_width, int frame_height, std::chrono::system_clock::time_point now);

    /**
     * @brief Prunes old or missing secondary face tracks.
     */
    void prune_secondary_tracks(std::chrono::system_clock::time_point now);

    Config config_;
    IFaceDetector& face_detector_;
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

    // Temporal persistence tracks for secondary faces
    std::vector<SecondaryFaceTrack> secondary_tracks_{};
    size_t next_track_id_ = 1;
};

} // namespace blindside

#endif // BLINDSIDE_EAVESDROPPER_DETECTOR_HPP
