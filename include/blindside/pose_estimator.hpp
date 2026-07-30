#ifndef BLINDSIDE_POSE_ESTIMATOR_HPP
#define BLINDSIDE_POSE_ESTIMATOR_HPP

#include "blindside/config.hpp"
#include "blindside/types.hpp"
#include "blindside/camera.hpp"

namespace blindside {

class PoseEstimator {
public:
    explicit PoseEstimator(const Config& config);
    ~PoseEstimator() = default;

    /**
     * @brief Estimates 3D head pose (pitch, yaw, roll) and screen gaze direction.
     * @param face Target face box and 2D facial landmarks.
     * @param frame Raw camera frame metadata for aspect ratio & focal length projection.
     * @return HeadPose calculated pose struct.
     */
    HeadPose estimate_pose(const FaceBox& face, const RawFrame& frame);

    /**
     * @brief Computes 3D gaze vector from pitch and yaw angles (in degrees).
     */
    static Point3f compute_gaze_vector(double pitch_deg, double yaw_deg);

    /**
     * @brief Checks if a head pose points towards the monitor/display surface.
     */
    bool is_gaze_directed_at_screen(const HeadPose& pose) const;

private:
    Config config_;
};

} // namespace blindside

#endif // BLINDSIDE_POSE_ESTIMATOR_HPP
