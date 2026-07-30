#include "blindside/pose_estimator.hpp"
#include "blindside/config.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

void test_gaze_vector_computation() {
    // 0 deg pitch, 0 deg yaw -> Gaze vector pointing directly forward along +Z (0, 0, 1)
    auto gaze_center = blindside::PoseEstimator::compute_gaze_vector(0.0, 0.0);
    assert(std::abs(gaze_center.x - 0.0f) < 0.001f);
    assert(std::abs(gaze_center.y - 0.0f) < 0.001f);
    assert(std::abs(gaze_center.z - 1.0f) < 0.001f);
    (void)gaze_center;

    // 0 deg pitch, 90 deg yaw -> Gaze vector pointing right along +X (1, 0, 0)
    auto gaze_right = blindside::PoseEstimator::compute_gaze_vector(0.0, 90.0);
    assert(std::abs(gaze_right.x - 1.0f) < 0.001f);
    assert(std::abs(gaze_right.z - 0.0f) < 0.001f);
    (void)gaze_right;

    std::cout << "[TEST PASSED] 3D Gaze Vector calculation test." << std::endl;
}

void test_screen_gaze_bounds() {
    blindside::Config config;
    config.max_allowed_yaw_deg = 25.0;
    config.max_allowed_pitch_deg = 20.0;

    blindside::PoseEstimator estimator(config);

    blindside::HeadPose pose_facing_screen;
    pose_facing_screen.yaw_deg = 10.0;
    pose_facing_screen.pitch_deg = -5.0;
    assert(estimator.is_gaze_directed_at_screen(pose_facing_screen) == true);

    blindside::HeadPose pose_looking_away;
    pose_looking_away.yaw_deg = 45.0; // Exceeds 25 deg tolerance
    pose_looking_away.pitch_deg = 0.0;
    assert(estimator.is_gaze_directed_at_screen(pose_looking_away) == false);

    std::cout << "[TEST PASSED] Screen Gaze Bounds tolerance test." << std::endl;
}

int main() {
    test_gaze_vector_computation();
    test_screen_gaze_bounds();
    std::cout << "All Gaze Math unit tests passed successfully!\n";
    return 0;
}
