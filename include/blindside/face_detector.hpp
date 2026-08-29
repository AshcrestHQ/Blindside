#ifndef BLINDSIDE_FACE_DETECTOR_HPP
#define BLINDSIDE_FACE_DETECTOR_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include "blindside/camera.hpp"
#include <vector>
#include <string>

namespace cv {
    class FaceDetectorYN;
}

namespace blindside {

class IFaceDetector {
public:
    virtual ~IFaceDetector() = default;
    virtual bool initialize(const std::string& model_path = "") = 0;
    virtual std::vector<FaceBox> detect(const RawFrame& frame) = 0;
    virtual bool is_initialized() const = 0;
};

class FaceDetector : public IFaceDetector {
public:
    explicit FaceDetector(const Config& config);
    ~FaceDetector() override;

    bool initialize(const std::string& model_path = "models/face_detection_yunet_2023mar.onnx") override;
    
    std::vector<FaceBox> detect(const RawFrame& frame) override;

    bool is_initialized() const override { return initialized_; }

private:
    Config config_;
    bool initialized_ = false;

    void* detector_ptr_ = nullptr;
};

} // namespace blindside

#endif // BLINDSIDE_FACE_DETECTOR_HPP
