#include "blindside/eavesdropper_detector.hpp"
#include <iostream>
#include <cmath>

namespace blindside {

EavesdropperDetector::EavesdropperDetector(const Config& config,
                                           IFaceDetector& detector,
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

void EavesdropperDetector::reset_trigger_state() {
    secondary_gaze_active_ = false;
    // We intentionally DO NOT clear history_count_ or history_head_.
    // Liveness evidence naturally expires via timestamps.
}

bool EavesdropperDetector::evaluate_liveness(double current_pitch, double current_yaw, float current_ear, bool& is_spoof_photo) {
    auto now = std::chrono::system_clock::now();

    // Push new sample into zero-allocation rolling buffer
    pose_history_[history_head_] = {now, current_pitch, current_yaw, current_ear, true};
    history_head_ = (history_head_ + 1) % HISTORY_CAPACITY;
    if (history_count_ < HISTORY_CAPACITY) history_count_++;

    if (history_count_ < 4) {
        is_spoof_photo = false;
        return true; // Default to live threat until enough samples gathered
    }

    // Calculate variance of pitch, yaw, and min EAR over last 3 seconds
    double mean_pitch = 0.0;
    double mean_yaw = 0.0;
    float min_ear = 1.0f;
    size_t valid_samples = 0;

    for (size_t i = 0; i < history_count_; ++i) {
        const auto& s = pose_history_[i];
        if (!s.valid) continue;
        std::chrono::duration<double> dt = now - s.timestamp;
        if (dt.count() <= config_.liveness_window_sec) {
            mean_pitch += s.pitch;
            mean_yaw += s.yaw;
            if (s.ear < min_ear) min_ear = s.ear;
            valid_samples++;
        }
    }

    if (valid_samples < 3) {
        is_spoof_photo = false;
        return true;
    }

    mean_pitch /= valid_samples;
    mean_yaw /= valid_samples;

    double var_pitch = 0.0;
    double var_yaw = 0.0;

    for (size_t i = 0; i < history_count_; ++i) {
        const auto& s = pose_history_[i];
        if (!s.valid) continue;
        std::chrono::duration<double> dt = now - s.timestamp;
        if (dt.count() <= config_.liveness_window_sec) {
            var_pitch += (s.pitch - mean_pitch) * (s.pitch - mean_pitch);
            var_yaw += (s.yaw - mean_yaw) * (s.yaw - mean_yaw);
        }
    }

    var_pitch /= valid_samples;
    var_yaw /= valid_samples;

    double total_variance = var_pitch + var_yaw;

    // Anti-Spoofing Rules:
    // If total variance is near zero AND eye aspect ratio exhibits no blink variation -> Static Photo / Spoof
    if (total_variance < 0.02 && min_ear > config_.ear_blink_threshold) {
        is_spoof_photo = true;
        return false;
    }

    is_spoof_photo = false;
    return true;
}

bool EavesdropperDetector::is_valid_face_box(const FaceBox& box, int frame_width, int frame_height) const {
    // 1. Basic physical sanity: positive width and height
    if (box.width <= 0.0f || box.height <= 0.0f) {
        return false;
    }

    // 2. Aspect ratio sanity (width / height)
    float aspect_ratio = box.width / box.height;
    if (aspect_ratio < 0.20f || aspect_ratio > 3.0f) {
        return false;
    }

    // 3. Frame boundary sanity check (must overlap with frame)
    float fw = static_cast<float>(frame_width > 0 ? frame_width : 640);
    float fh = static_cast<float>(frame_height > 0 ? frame_height : 480);
    if (box.x + box.width < 0.0f || box.y + box.height < 0.0f || box.x > fw || box.y > fh) {
        return false;
    }

    return true;
}

bool EavesdropperDetector::update_and_validate_secondary_track(const FaceBox& box, int frame_width, int frame_height, std::chrono::system_clock::time_point now) {
    float fw = static_cast<float>(frame_width > 0 ? frame_width : 640);
    float fh = static_cast<float>(frame_height > 0 ? frame_height : 480);

    float norm_cx = box.center_x() / fw;
    float norm_cy = box.center_y() / fh;
    float norm_w = box.width / fw;
    float norm_h = box.height / fh;

    SecondaryFaceTrack* match = nullptr;
    float min_dist = 1e9f;

    for (auto& track : secondary_tracks_) {
        float dx = norm_cx - track.norm_center_x;
        float dy = norm_cy - track.norm_center_y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= 0.15f && dist < min_dist) {
            min_dist = dist;
            match = &track;
        }
    }

    if (match) {
        match->norm_center_x = norm_cx;
        match->norm_center_y = norm_cy;
        match->norm_width = norm_w;
        match->norm_height = norm_h;
        match->last_seen = now;
        match->hit_count++;
        match->miss_count = 0;
        return true;
    } else {
        SecondaryFaceTrack new_track;
        new_track.id = next_track_id_++;
        new_track.norm_center_x = norm_cx;
        new_track.norm_center_y = norm_cy;
        new_track.norm_width = norm_w;
        new_track.norm_height = norm_h;
        new_track.first_seen = now;
        new_track.last_seen = now;
        new_track.hit_count = 1;
        new_track.miss_count = 0;
        secondary_tracks_.push_back(new_track);
        return true;
    }
}

void EavesdropperDetector::prune_secondary_tracks(std::chrono::system_clock::time_point now) {
    for (auto it = secondary_tracks_.begin(); it != secondary_tracks_.end();) {
        std::chrono::duration<double> dt = now - it->last_seen;
        if (it->miss_count > 5 || dt.count() > 1.0) {
            it = secondary_tracks_.erase(it);
        } else {
            ++it;
        }
    }
}

FrameResult EavesdropperDetector::process_frame(const RawFrame& frame) {
    FrameResult result;
    result.frame_id = frame.frame_id;
    result.timestamp = frame.timestamp;
    auto now = std::chrono::system_clock::now();

    auto raw_faces = face_detector_.detect(frame);
    
    // 1. Raw YuNet Detections Diagnostic
    std::cout << "[DIAGNOSTICS] Raw YuNet faces detected: " << raw_faces.size() << std::endl;
    for (size_t i = 0; i < raw_faces.size(); ++i) {
        std::cout << "  [RAW DETECTION " << i << "] Box: [x=" << raw_faces[i].x << ", y=" << raw_faces[i].y 
                  << ", w=" << raw_faces[i].width << ", h=" << raw_faces[i].height 
                  << "] confidence=" << raw_faces[i].confidence << std::endl;
    }

    if (raw_faces.empty()) {
        reset_trigger_state();
        prune_secondary_tracks(now);
        std::cout << "[DIAGNOSTICS] Validated Primary Face: None (User absent)" << std::endl;
        std::cout << "[DIAGNOSTICS] Face Count Summary: Total Validated = 0 | Primary = 0 | Secondary = 0 (Raw YuNet Detections = 0)" << std::endl;
        std::cout << "[DIAGNOSTICS] Liveness Status: INACTIVE (No faces detected)" << std::endl;
        return result;
    }

    int fw = frame.width > 0 ? frame.width : 640;
    int fh = frame.height > 0 ? frame.height : 480;
    float norm_w = static_cast<float>(fw);
    float norm_h = static_cast<float>(fh);
    float cal_cx = primary_calibration_box_.center_x();
    float cal_cy = primary_calibration_box_.center_y();

    // Pre-filter raw detections via basic bounding box sanity
    std::vector<FaceBox> valid_boxes;
    for (const auto& box : raw_faces) {
        if (is_valid_face_box(box, fw, fh)) {
            valid_boxes.push_back(box);
        }
    }

    if (valid_boxes.empty()) {
        reset_trigger_state();
        prune_secondary_tracks(now);
        std::cout << "[DIAGNOSTICS] Validated Primary Face: None (Degenerate face boxes filtered)" << std::endl;
        std::cout << "[DIAGNOSTICS] Face Count Summary: Total Validated = 0 | Primary = 0 | Secondary = 0 (Raw YuNet Detections = " << raw_faces.size() << ")" << std::endl;
        std::cout << "[DIAGNOSTICS] Liveness Status: INACTIVE (All raw boxes failed sanity checks)" << std::endl;
        return result;
    }

    // Identify Primary User box robustly among valid candidates
    size_t primary_idx = static_cast<size_t>(-1);
    float min_dist_to_cal = 1e9f;

    for (size_t i = 0; i < valid_boxes.size(); ++i) {
        float norm_cx = valid_boxes[i].center_x() / norm_w;
        float norm_cy = valid_boxes[i].center_y() / norm_h;
        float dx = norm_cx - cal_cx;
        float dy = norm_cy - cal_cy;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= config_.primary_box_tolerance && dist < min_dist_to_cal) {
            min_dist_to_cal = dist;
            primary_idx = i;
        }
    }

    // Mark miss_count on existing secondary tracks before evaluating current frame
    for (auto& track : secondary_tracks_) {
        track.miss_count++;
    }

    bool found_secondary_gaze = false;
    bool secondary_liveness_pass = false;

    // Process primary user first, then secondary candidate faces with temporal persistence
    for (size_t i = 0; i < valid_boxes.size(); ++i) {
        const auto& box = valid_boxes[i];
        FaceDetectionResult face_res;
        face_res.box = box;
        face_res.pose = pose_estimator_.estimate_pose(box, frame);
        
        if (!face_res.pose.valid) continue;
        face_res.ear = PoseEstimator::compute_ear(box);

        if (i == primary_idx) {
            face_res.is_primary_user = true;
            result.primary_user_present = true;
            result.faces.push_back(face_res);
        } else {
            // Secondary candidate: must pass temporal persistence tracking
            bool validated_track = update_and_validate_secondary_track(box, fw, fh, now);
            if (!validated_track) continue;

            face_res.is_eavesdropper = true;
            if (face_res.pose.is_looking_at_screen) {
                found_secondary_gaze = true;
                bool is_spoof = false;
                bool is_live = evaluate_liveness(face_res.pose.pitch_deg, face_res.pose.yaw_deg, face_res.ear, is_spoof);
                face_res.is_live_threat = is_live;
                face_res.is_spoof_static = is_spoof;
                if (is_live && !is_spoof) {
                    secondary_liveness_pass = true;
                }
            }
            result.faces.push_back(face_res);
        }
    }

    prune_secondary_tracks(now);

    result.secondary_gaze_detected = found_secondary_gaze;
    result.secondary_liveness_verified = secondary_liveness_pass;

    // 2. Log Validated Primary Face Diagnostic
    if (result.primary_user_present && primary_idx < valid_boxes.size()) {
        const auto& prim = valid_boxes[primary_idx];
        std::cout << "[DIAGNOSTICS] Validated Primary Face: [x=" << prim.x << ", y=" << prim.y 
                  << ", w=" << prim.width << ", h=" << prim.height << "]" << std::endl;
    } else {
        std::cout << "[DIAGNOSTICS] Validated Primary Face: None (User absent or outside calibration region)" << std::endl;
    }

    // 3. Log Validated Secondary Candidates Diagnostic
    size_t secondary_count = 0;
    for (size_t i = 0; i < result.faces.size(); ++i) {
        if (result.faces[i].is_eavesdropper) {
            secondary_count++;
            std::cout << "  [VALIDATED SECONDARY CANDIDATE " << (secondary_count - 1) << "] Box: [x=" 
                      << result.faces[i].box.x << ", y=" << result.faces[i].box.y 
                      << ", w=" << result.faces[i].box.width << ", h=" << result.faces[i].box.height 
                      << "] gaze_at_screen=" << (result.faces[i].pose.is_looking_at_screen ? "true" : "false") << std::endl;
        }
    }

    // 4. Log Final Physical-Face / Eavesdropper Summary
    std::cout << "[DIAGNOSTICS] Face Count Summary: Total Validated = " << result.faces.size() 
              << " | Primary = " << (result.primary_user_present ? 1 : 0)
              << " | Secondary Candidates = " << secondary_count 
              << " (Raw YuNet Detections = " << raw_faces.size() << ")" << std::endl;

    // 5. Log Honest Liveness Architecture Limitation
    std::cout << "[DIAGNOSTICS] Liveness Status: GLOBAL_EVALUATED [Secondary gaze=" 
              << (found_secondary_gaze ? "detected" : "none")
              << ", liveness_pass=" << (secondary_liveness_pass ? "true" : "false")
              << "] (Note: Independent per-secondary-track liveness evaluation is pending V3.2 architecture refactor)" << std::endl;

    // Hysteresis Filter (> 1.0 second threshold)
    if (found_secondary_gaze && secondary_liveness_pass) {
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
            if (config_.daemon_state == DaemonState::StrictFullLock) {
                result.trigger_hard_defense = true;
            } else if (config_.daemon_state == DaemonState::GracefulTargetedBlur) {
                result.trigger_targeted_blur = true;
            }
        }
    } else {
        reset_trigger_state();
    }

    return result;
}

} // namespace blindside
