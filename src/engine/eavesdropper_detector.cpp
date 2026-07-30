#include "blindside/eavesdropper_detector.hpp"
#include <iostream>
#include <cmath>

namespace blindside {

EavesdropperDetector::EavesdropperDetector(const Config& config,
                                           FaceDetector& detector,
                                           PoseEstimator& estimator)
    : config_(config), face_detector_(detector), pose_estimator_(estimator) {
    
    // Default calibration box at screen center
    primary_calibration_box_.width = 0.30f;
    primary_calibration_box_.height = 0.40f;
    primary_calibration_box_.x = 0.35f;
    primary_calibration_box_.y = 0.30f;
}

bool EavesdropperDetector::calibrate(const RawFrame& frame) {
    auto faces = face_detector_.detect(frame);
    if (faces.empty()) {
        std::cout << "[EavesdropperDetector] Calibration failed: No face detected." << std::endl;
        calibrated_ = false;
        return false;
    }

    // Pick face closest to frame center
    float frame_center_x = frame.width * 0.5f;
    float frame_center_y = frame.height * 0.5f;
    float min_dist_sq = 1e9f;
    size_t best_idx = 0;

    for (size_t i = 0; i < faces.size(); ++i) {
        float cx = faces[i].center_x();
        float cy = faces[i].center_y();
        float dist_sq = (cx - frame_center_x) * (cx - frame_center_x) + (cy - frame_center_y) * (cy - frame_center_y);
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            best_idx = i;
        }
    }

    // Normalize coordinates
    primary_calibration_box_ = faces[best_idx];
    if (frame.width > 0 && frame.height > 0) {
        primary_calibration_box_.x /= frame.width;
        primary_calibration_box_.y /= frame.height;
        primary_calibration_box_.width /= frame.width;
        primary_calibration_box_.height /= frame.height;
    }

    calibrated_ = true;
    std::cout << "[EavesdropperDetector] Primary user registered at normalized center [" 
              << primary_calibration_box_.center_x() << ", " << primary_calibration_box_.center_y() << "]" << std::endl;
    return true;
}

void EavesdropperDetector::reset_state() {
    secondary_gaze_active_ = false;
}

FrameResult EavesdropperDetector::process_frame(const RawFrame& frame) {
    FrameResult result;
    result.frame_id = frame.frame_id;
    result.timestamp = frame.timestamp;

    auto raw_faces = face_detector_.detect(frame);
    if (raw_faces.empty()) {
        reset_state();
        return result;
    }

    float norm_w = static_cast<float>(frame.width > 0 ? frame.width : 640);
    float norm_h = static_cast<float>(frame.height > 0 ? frame.height : 480);
    float cal_cx = primary_calibration_box_.center_x();
    float cal_cy = primary_calibration_box_.center_y();

    bool found_secondary_gaze = false;

    for (const auto& raw_face : raw_faces) {
        FaceDetectionResult face_res;
        face_res.box = raw_face;
        face_res.pose = pose_estimator_.estimate_pose(raw_face, frame);

        float norm_cx = raw_face.center_x() / norm_w;
        float norm_cy = raw_face.center_y() / norm_h;
        float dx = norm_cx - cal_cx;
        float dy = norm_cy - cal_cy;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= config_.primary_box_tolerance) {
            face_res.is_primary_user = true;
            result.primary_user_present = true;
        } else {
            face_res.is_eavesdropper = true;
            if (face_res.pose.is_looking_at_screen) {
                found_secondary_gaze = true;
            }
        }
        result.faces.push_back(face_res);
    }

    result.secondary_gaze_detected = found_secondary_gaze;

    // Temporal Hysteresis Filter (> 1.0 second threshold)
    auto now = std::chrono::system_clock::now();
    if (found_secondary_gaze) {
        if (!secondary_gaze_active_) {
            secondary_gaze_active_ = true;
            secondary_gaze_start_time_ = now;
            result.secondary_gaze_duration_sec = 0.0;
        } else {
            std::chrono::duration<double> elapsed = now - secondary_gaze_start_time_;
            result.secondary_gaze_duration_sec = elapsed.count();
        }

        result.trigger_soft_alert = true;
        if (result.secondary_gaze_duration_sec >= config_.hysteresis_sec) {
            result.trigger_hard_defense = true;
        }
    } else {
        reset_state();
    }

    return result;
}

} // namespace blindside
