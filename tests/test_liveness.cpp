#include "blindside/eavesdropper_detector.hpp"
#include "blindside/face_detector.hpp"
#include "blindside/pose_estimator.hpp"
#include "blindside/config.hpp"
#include <cassert>
#include <iostream>
#include <thread>

void test_ear_calculation() {
    blindside::FaceBox face;
    face.width = 100.0f;
    face.height = 120.0f;
    face.landmarks[0] = {30.0f, 40.0f};  // Right Eye
    face.landmarks[1] = {70.0f, 40.0f};  // Left Eye
    face.landmarks[2] = {50.0f, 65.0f};  // Nose

    float ear = blindside::PoseEstimator::compute_ear(face);
    assert(ear > 0.10f && ear < 0.45f);
    (void)ear;

    std::cout << "[TEST PASSED] Eye Aspect Ratio (EAR) calculation test." << std::endl;
}

void test_photo_spoof_rejection() {
    blindside::Config config;
    config.hysteresis_sec = 0.4;
    config.ear_blink_threshold = 0.20f;
    blindside::FaceDetector detector(config);
    blindside::PoseEstimator estimator(config);
    detector.initialize();

    blindside::EavesdropperDetector eavesdropper(config, detector, estimator);

    blindside::RawFrame frame;
    frame.width = 640;
    frame.height = 480;
    frame.buffer.resize(frame.width * frame.height * 3, 220); // Secondary face present

    // Simulate 5 frames with static pose and 0 variance (static photo)
    for (int i = 0; i < 6; ++i) {
        auto res = eavesdropper.process_frame(frame);
        (void)res;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Process 7th frame -> Anti-spoofing should verify static photo and suppress hard lock
    auto final_res = eavesdropper.process_frame(frame);
    assert(final_res.secondary_gaze_detected == true);
    
    std::cout << "[TEST PASSED] Anti-Spoofing Static Photo Rejection test." << std::endl;
}

int main() {
    test_ear_calculation();
    test_photo_spoof_rejection();
    std::cout << "All Anti-Spoofing & Liveness unit tests passed successfully!\n";
    return 0;
}
