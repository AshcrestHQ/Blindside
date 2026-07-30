#ifndef BLINDSIDE_CONFIG_HPP
#define BLINDSIDE_CONFIG_HPP

#include <string>
#include <cstdint>

namespace blindside {

enum class TriggerMode {
    SoftAlertOnly,
    HardDefenseOnly,
    Both,
    LogOnly
};

struct Config {
    // Frame sampling rates (Hz)
    double active_fps = 30.0;
    double idle_fps = 5.0;
    double idle_timeout_sec = 3.0; // Time in seconds of no secondary gaze before dropping to idle_fps

    // Camera settings
    int camera_index = 0;
    int capture_width = 640;
    int capture_height = 480;

    // Detection & Gaze Estimation Parameters
    float face_confidence_threshold = 0.55f;
    double max_allowed_yaw_deg = 30.0;   // Yaw angle relative to screen center
    double max_allowed_pitch_deg = 25.0; // Pitch angle relative to screen center
    double hysteresis_sec = 1.0;         // Secondary gaze must persist > 1s for Hard Defense

    // Primary User Calibration Box (normalized [0, 1])
    float primary_x_center = 0.5f;
    float primary_y_center = 0.5f;
    float primary_box_tolerance = 0.35f; // Distance threshold for primary vs eavesdropper

    // Trigger behavior
    TriggerMode trigger_mode = TriggerMode::Both;
    bool enable_screen_lock = true;
    bool enable_blur_overlay = true;
    bool enable_sound_alert = false;
    std::string log_file_path = "blindside_threats.log";
};

} // namespace blindside

#endif // BLINDSIDE_CONFIG_HPP
