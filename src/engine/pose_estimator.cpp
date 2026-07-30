#include "blindside/pose_estimator.hpp"
#include <cmath>
#include <algorithm>

namespace blindside {

constexpr double M_PI_VAL = 3.14159265358979323846;

PoseEstimator::PoseEstimator(const Config& config) : config_(config) {}

Point3f PoseEstimator::compute_gaze_vector(double pitch_deg, double yaw_deg) {
    double pitch_rad = pitch_deg * M_PI_VAL / 180.0;
    double yaw_rad   = yaw_deg   * M_PI_VAL / 180.0;

    Point3f gaze;
    gaze.x = static_cast<float>(std::sin(yaw_rad) * std::cos(pitch_rad));
    gaze.y = static_cast<float>(-std::sin(pitch_rad));
    gaze.z = static_cast<float>(std::cos(yaw_rad) * std::cos(pitch_rad));
    return gaze;
}

HeadPose PoseEstimator::estimate_pose(const FaceBox& face, const RawFrame& frame) {
    HeadPose pose;
    if (frame.width <= 0 || frame.height <= 0) return pose;

    // 1. Calculate eye mid-point & inter-ocular distance
    const auto& eye_r = face.landmarks[0];
    const auto& eye_l = face.landmarks[1];
    const auto& nose  = face.landmarks[2];

    float eye_dx = eye_l.x - eye_r.x;
    float eye_dy = eye_l.y - eye_r.y;
    float interocular_dist = std::sqrt(eye_dx * eye_dx + eye_dy * eye_dy);

    // Roll angle from eye tilt
    pose.roll_deg = std::atan2(eye_dy, eye_dx) * 180.0 / M_PI_VAL;

    // 2. Yaw angle estimation from nose horizontal displacement relative to eyes center
    float eye_center_x = (eye_r.x + eye_l.x) * 0.5f;
    float nose_rel_x = nose.x - eye_center_x;
    float normalized_yaw_ratio = (interocular_dist > 0.001f) ? (nose_rel_x / interocular_dist) : 0.0f;

    // Clamp ratio to [-1, 1]
    normalized_yaw_ratio = std::clamp(normalized_yaw_ratio, -1.0f, 1.0f);
    pose.yaw_deg = normalized_yaw_ratio * 60.0; // Map ratio to +/- 60 deg yaw

    // 3. Pitch angle estimation from nose vertical position relative to face height
    float eye_center_y = (eye_r.y + eye_l.y) * 0.5f;
    float nose_rel_y = nose.y - eye_center_y;
    float expected_nose_y = face.height * 0.20f;
    float pitch_ratio = (face.height > 0.001f) ? ((nose_rel_y - expected_nose_y) / face.height) : 0.0f;
    pose.pitch_deg = std::clamp(pitch_ratio * 70.0f, -45.0f, 45.0f);

    // Compute 3D gaze vector
    pose.gaze_vector = compute_gaze_vector(pose.pitch_deg, pose.yaw_deg);

    // Check if gaze points at screen
    pose.is_looking_at_screen = is_gaze_directed_at_screen(pose);

    return pose;
}

bool PoseEstimator::is_gaze_directed_at_screen(const HeadPose& pose) const {
    return (std::abs(pose.yaw_deg) <= config_.max_allowed_yaw_deg) &&
           (std::abs(pose.pitch_deg) <= config_.max_allowed_pitch_deg);
}

} // namespace blindside
