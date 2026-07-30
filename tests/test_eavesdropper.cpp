#include "blindside/eavesdropper_detector.hpp"
#include "blindside/face_detector.hpp"
#include "blindside/pose_estimator.hpp"
#include "blindside/config.hpp"
#include <cassert>
#include <iostream>
#include <thread>

void test_primary_user_calibration() {
    blindside::Config config;
    blindside::FaceDetector detector(config);
    blindside::PoseEstimator estimator(config);
    detector.initialize();

    blindside::EavesdropperDetector eavesdropper(config, detector, estimator);

    blindside::RawFrame frame;
    frame.width = 640;
    frame.height = 480;
    frame.buffer.resize(frame.width * frame.height * 3, 100);

    bool cal_ok = eavesdropper.calibrate(frame);
    assert(cal_ok == true);
    assert(eavesdropper.is_calibrated() == true);
    (void)cal_ok;

    std::cout << "[TEST PASSED] EavesdropperDetector Primary User Calibration test." << std::endl;
}

void test_eavesdropper_gaze_hysteresis() {
    blindside::Config config;
    config.hysteresis_sec = 0.5; // Short hysteresis threshold for unit test
    blindside::FaceDetector detector(config);
    blindside::PoseEstimator estimator(config);
    detector.initialize();

    blindside::EavesdropperDetector eavesdropper(config, detector, estimator);

    blindside::RawFrame frame;
    frame.width = 640;
    frame.height = 480;
    // High blue brightness triggers secondary face detection in sample frame
    frame.buffer.resize(frame.width * frame.height * 3, 220);

    // First frame detects secondary gaze -> Soft alert triggered, but hard defense false (< hysteresis_sec)
    auto res1 = eavesdropper.process_frame(frame);
    assert(res1.trigger_soft_alert == true);
    assert(res1.trigger_hard_defense == false);

    // Sleep past hysteresis time threshold (600ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Second frame detects persistent gaze > 0.5s -> Hard defense triggered
    auto res2 = eavesdropper.process_frame(frame);
    assert(res2.trigger_soft_alert == true);
    assert(res2.trigger_hard_defense == true);

    std::cout << "[TEST PASSED] EavesdropperDetector Gaze Hysteresis test." << std::endl;
}

int main() {
    test_primary_user_calibration();
    test_eavesdropper_gaze_hysteresis();
    std::cout << "All EavesdropperDetector unit tests passed successfully!\n";
    return 0;
}
