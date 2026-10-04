"""
Blindside Python Reference Test Suite
Unit and simulation tests validating the Python reference implementation.
NOTE: These tests validate tools/reference/run_blindside.py only.
They do NOT replace or test the C++20 production test suite.
"""

import math
import os
import sys
import time
import unittest
from collections import deque
import numpy as np

# Ensure reference module path is accessible
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

from run_blindside import (
    Config,
    FaceBox,
    HeadPose,
    Point2f,
    Point3f,
    PoseEstimator,
    EavesdropperDetector,
    FaceDetector,
    FrameResult,
    CameraCapture
)

class TestGazeMath(unittest.TestCase):
    def test_gaze_vector_computation(self):
        # 0 deg pitch, 0 deg yaw -> Gaze vector pointing directly forward (0, 0, 1)
        gaze_center = PoseEstimator.compute_gaze_vector(0.0, 0.0)
        self.assertAlmostEqual(gaze_center.x, 0.0, places=3)
        self.assertAlmostEqual(gaze_center.y, 0.0, places=3)
        self.assertAlmostEqual(gaze_center.z, 1.0, places=3)

        # 0 deg pitch, 90 deg yaw -> Gaze vector pointing right (1, 0, 0)
        gaze_right = PoseEstimator.compute_gaze_vector(0.0, 90.0)
        self.assertAlmostEqual(gaze_right.x, 1.0, places=3)
        self.assertAlmostEqual(gaze_right.z, 0.0, places=3)

    def test_screen_gaze_bounds(self):
        config = Config(max_allowed_yaw_deg=25.0, max_allowed_pitch_deg=20.0)
        estimator = PoseEstimator(config)

        pose_facing_screen = HeadPose(yaw_deg=10.0, pitch_deg=-5.0)
        self.assertTrue(estimator.is_gaze_directed_at_screen(pose_facing_screen))

        pose_looking_away = HeadPose(yaw_deg=45.0, pitch_deg=0.0)
        self.assertFalse(estimator.is_gaze_directed_at_screen(pose_looking_away))

class MockFaceDetector(FaceDetector):
    def __init__(self, config: Config):
        super().__init__(config)
        self.initialized = True
        self.trigger_secondary = False

    def initialize(self, model_path: str) -> bool:
        return True

    def detect(self, frame: np.ndarray):
        faces = [
            FaceBox(
                x=300, y=200, width=100, height=100, confidence=0.95,
                landmarks=[
                    Point2f(330, 230), Point2f(370, 230), Point2f(350, 250),
                    Point2f(330, 270), Point2f(370, 270)
                ]
            )
        ]
        if self.trigger_secondary:
            faces.append(
                FaceBox(
                    x=50, y=50, width=100, height=100, confidence=0.90,
                    landmarks=[
                        Point2f(80, 80), Point2f(120, 80), Point2f(100, 100),
                        Point2f(80, 120), Point2f(120, 120)
                    ]
                )
            )
        return faces

class TestEavesdropperDetector(unittest.TestCase):
    def test_primary_user_calibration(self):
        config = Config()
        mock_detector = MockFaceDetector(config)
        estimator = PoseEstimator(config)
        eavesdropper = EavesdropperDetector(config, mock_detector, estimator)

        dummy_frame = np.zeros((480, 640, 3), dtype=np.uint8)
        self.assertTrue(eavesdropper.calibrate(dummy_frame))
        self.assertTrue(eavesdropper.calibrated)

    def test_eavesdropper_gaze_hysteresis(self):
        config = Config(hysteresis_sec=0.2, daemon_state="strict")
        mock_detector = MockFaceDetector(config)
        estimator = PoseEstimator(config)
        eavesdropper = EavesdropperDetector(config, mock_detector, estimator)

        dummy_frame = np.zeros((480, 640, 3), dtype=np.uint8)
        eavesdropper.calibrate(dummy_frame)

        # Trigger secondary face
        mock_detector.trigger_secondary = True

        # First frame: detects secondary face, soft alert triggers, hard defense False
        res1 = eavesdropper.process_frame(dummy_frame)
        self.assertTrue(res1.trigger_soft_alert)
        self.assertFalse(res1.trigger_hard_defense)

        # Wait beyond hysteresis (300ms > 200ms)
        time.sleep(0.3)

        # Second frame: persistent gaze triggers hard defense
        res2 = eavesdropper.process_frame(dummy_frame)
        self.assertTrue(res2.trigger_soft_alert)
        self.assertTrue(res2.trigger_hard_defense)

class TestLiveness(unittest.TestCase):
    def test_spoof_detection(self):
        config = Config(liveness_window_sec=3.0, ear_blink_threshold=0.20)
        mock_detector = MockFaceDetector(config)
        estimator = PoseEstimator(config)
        eavesdropper = EavesdropperDetector(config, mock_detector, estimator)

        # Feed static pitch/yaw (variance = 0) with no blinks (ear = 0.35 > threshold)
        for _ in range(5):
            live, spoof = eavesdropper.evaluate_liveness(pitch=5.0, yaw=10.0, ear=0.35)

        self.assertFalse(live)
        self.assertTrue(spoof)

    def test_live_user_micro_movements(self):
        config = Config(liveness_window_sec=3.0, ear_blink_threshold=0.20)
        mock_detector = MockFaceDetector(config)
        estimator = PoseEstimator(config)
        eavesdropper = EavesdropperDetector(config, mock_detector, estimator)

        # Feed varying pitch/yaw (micro movements) and a blink (ear = 0.12)
        pitches = [5.0, 5.8, 6.2, 5.1, 4.9]
        yaws = [10.0, 11.2, 9.8, 10.5, 11.0]
        ears = [0.30, 0.31, 0.12, 0.29, 0.32] # 0.12 is a blink

        for p, y, e in zip(pitches, yaws, ears):
            live, spoof = eavesdropper.evaluate_liveness(pitch=p, yaw=y, ear=e)

        self.assertTrue(live)
        self.assertFalse(spoof)

class TestCameraFailure(unittest.TestCase):
    def test_synthetic_camera_fallback(self):
        config = Config(camera_index=999) # Invalid camera index
        camera = CameraCapture(config)
        self.assertTrue(camera.open())
        self.assertTrue(camera.synthetic_mode)
        frame = camera.grab_frame()
        self.assertIsNotNone(frame)
        self.assertEqual(frame.shape, (480, 640, 3))
        camera.close()

if __name__ == "__main__":
    unittest.main(verbosity=2)
