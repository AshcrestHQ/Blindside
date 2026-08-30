#include "blindside/camera.hpp"
#include <iostream>
#include <cassert>

void test_camera_grab_failure() {
    blindside::Config config;
    // Set invalid camera index to force failure or fallback
    config.camera_index = 999;
    
    blindside::CameraCapture camera(config);
    
    // In synthetic mode, grab_frame returns true.
    // If we disable synthetic mode, grab_frame should fail.
    // However, CameraCapture falls back to synthetic mode automatically.
    // Let's test the explicit failure path by setting synthetic_mode to false after opening.
    camera.open();
    camera.set_synthetic_mode(false); // force real mode without a valid cap
    
    blindside::RawFrame frame;
    bool success = camera.grab_frame(frame);
    
    // It must return false for a broken/unopened camera.
    assert(success == false);
    
    // The frame buffer should be zeroed or remain empty.
    assert(frame.buffer.empty() || frame.buffer.size() == 0 || (frame.width == 0 && frame.height == 0));
    
    std::cout << "[PASS] Camera failure gracefully returns false without allocating a fake frame." << std::endl;
}

int main() {
    test_camera_grab_failure();
    return 0;
}
