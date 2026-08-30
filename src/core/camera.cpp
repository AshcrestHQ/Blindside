#include "blindside/camera.hpp"
#include <iostream>
#include <thread>
#include <cmath>

#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#define BLINDSIDE_HAS_OPENCV 1
#else
#define BLINDSIDE_HAS_OPENCV 0
#endif

namespace blindside {

struct CameraCapture::Impl {
#if BLINDSIDE_HAS_OPENCV
    cv::VideoCapture cap;
#endif
};

CameraCapture::CameraCapture(const Config& config)
    : config_(config), current_fps_(config.active_fps), impl_(std::make_unique<Impl>()) {
}

CameraCapture::~CameraCapture() {
    close();
}

bool CameraCapture::open() {
    if (synthetic_mode_) {
        is_open_ = true;
        std::cout << "[CameraCapture] Initialized in Synthetic / Emulation Mode." << std::endl;
        return true;
    }

#if BLINDSIDE_HAS_OPENCV
    if (impl_->cap.open(config_.camera_index)) {
        impl_->cap.set(cv::CAP_PROP_FRAME_WIDTH, config_.capture_width);
        impl_->cap.set(cv::CAP_PROP_FRAME_HEIGHT, config_.capture_height);
        impl_->cap.set(cv::CAP_PROP_FPS, current_fps_);
        is_open_ = true;
        std::cout << "[CameraCapture] Hardware camera index " << config_.camera_index 
                  << " opened successfully." << std::endl;
        return true;
    }
#endif

    std::cout << "[CameraCapture] Hardware camera unavailable. Falling back to synthetic stream." << std::endl;
    synthetic_mode_ = true;
    is_open_ = true;
    return true;
}

void CameraCapture::close() {
#if BLINDSIDE_HAS_OPENCV
    if (impl_->cap.isOpened()) {
        impl_->cap.release();
    }
#endif
    is_open_ = false;
}

bool CameraCapture::is_opened() const {
    return is_open_;
}

void CameraCapture::set_fps(double target_fps) {
    if (current_fps_.load() != target_fps) {
        current_fps_.store(target_fps);
    }
}

void CameraCapture::set_synthetic_eavesdropper(bool present, double gaze_yaw) {
    synthetic_eavesdropper_present_ = present;
    synthetic_eavesdropper_yaw_ = gaze_yaw;
}

bool CameraCapture::grab_frame(RawFrame& frame) {
    if (!is_open_) return false;

    frame.frame_id = ++frame_counter_;
    frame.timestamp = std::chrono::system_clock::now();
    frame.width = config_.capture_width;
    frame.height = config_.capture_height;
    frame.channels = 3;

    if (synthetic_mode_) {
        // Generate synthetic frame buffer (BGR gradient image)
        frame.buffer.resize(frame.width * frame.height * 3);
        uint8_t base_color = static_cast<uint8_t>((frame_counter_ * 5) % 256);
        
        for (int i = 0; i < frame.width * frame.height * 3; i += 3) {
            frame.buffer[i]     = base_color;       // Blue
            frame.buffer[i + 1] = 120;              // Green
            frame.buffer[i + 2] = 200 - base_color; // Red
        }

        // Simulate frame timing based on current target FPS
        double frame_delay_ms = 1000.0 / std::max(1.0, current_fps_.load());
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(frame_delay_ms)));
        return true;
    }

#if BLINDSIDE_HAS_OPENCV
    if (impl_->cap.isOpened()) {
        auto now = std::chrono::steady_clock::now();
        double target_interval = 1.0 / current_fps_.load();
        std::chrono::duration<double> elapsed = now - last_frame_time_;
        
        if (elapsed.count() < target_interval) {
            // Buffer drain to prevent stale frames, without decoding/processing
            if (!impl_->cap.grab()) {
                std::cerr << "[CameraCapture] Hardware frame read failed or pipeline halted. Closing camera to recover..." << std::endl;
                impl_->cap.release();
                is_open_.store(false);
                return false;
            }
            frame.buffer.clear();
            return true; // Successfully skipped
        }

        cv::Mat cv_frame;
        if (impl_->cap.read(cv_frame) && !cv_frame.empty()) {
            last_frame_time_ = std::chrono::steady_clock::now();
            frame.width = cv_frame.cols;
            frame.height = cv_frame.rows;
            size_t data_size = cv_frame.total() * cv_frame.elemSize();
            frame.buffer.assign(cv_frame.data, cv_frame.data + data_size);
            return true;
        } else {
            std::cerr << "[CameraCapture] Hardware frame read failed or pipeline halted. Closing camera to recover..." << std::endl;
            impl_->cap.release();
            is_open_.store(false);
            return false;
        }
    }
#endif

    // Fallback if not opened or synthetic mode
    frame.buffer.resize(frame.width * frame.height * 3, 0);
    return false;
}

} // namespace blindside
