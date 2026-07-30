#ifndef BLINDSIDE_FACE_DETECTOR_HPP
#define BLINDSIDE_FACE_DETECTOR_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include "blindside/camera.hpp"
#include <vector>
#include <string>
#include <memory>

namespace blindside {

class FaceDetector {
public:
    explicit FaceDetector(const Config& config);
    ~FaceDetector();

    bool initialize(const std::string& model_path = "");
    
    // Detect all face candidates in raw image frame
    std::vector<FaceBox> detect(const RawFrame& frame);

    bool is_initialized() const { return initialized_; }

private:
    Config config_;
    bool initialized_ = false;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace blindside

#endif // BLINDSIDE_FACE_DETECTOR_HPP
