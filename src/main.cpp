#include "blindside/daemon.hpp"
#include "blindside/config.hpp"
#include "blindside/tray_icon.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>

namespace {
    std::atomic<bool> g_stop_requested{false};

    void signal_handler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            std::cout << "\n[Blindside CLI] Shutdown signal received (" << signal << "). Terminating..." << std::endl;
            g_stop_requested.store(true);
        }
    }
}

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  --daemon               Run continuously in desktop background mode\n"
              << "  --mode <mode>          Set operational mode: graceful (targeted blur), strict (full lock), pause (default: graceful)\n"
              << "  --calibrate            Calibrate primary user baseline face position on launch\n"
              << "  --synthetic            Run in synthetic headless test mode (no physical camera required)\n"
              << "  --trigger-mode <mode>  Set trigger action: soft, hard, both, log (default: both)\n"
              << "  --fps-active <fps>     Set active monitoring FPS (default: 30)\n"
              << "  --fps-idle <fps>       Set low-power idle FPS (default: 5)\n"
              << "  --hysteresis <sec>     Secondary gaze duration required for lock/blur (default: 1.0)\n"
              << "  --help                 Display this help message\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    blindside::Config config;
    bool run_daemon = false;
    bool run_calibration = false;
    bool synthetic_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--daemon") {
            run_daemon = true;
        } else if (arg == "--mode" && i + 1 < argc) {
            std::string mode_str = argv[++i];
            if (mode_str == "strict") config.daemon_state = blindside::DaemonState::StrictFullLock;
            else if (mode_str == "pause") config.daemon_state = blindside::DaemonState::PauseDetection;
            else config.daemon_state = blindside::DaemonState::GracefulTargetedBlur;
        } else if (arg == "--calibrate") {
            run_calibration = true;
        } else if (arg == "--synthetic") {
            synthetic_mode = true;
        } else if (arg == "--trigger-mode" && i + 1 < argc) {
            std::string mode_str = argv[++i];
            if (mode_str == "soft") config.trigger_mode = blindside::TriggerMode::SoftAlertOnly;
            else if (mode_str == "hard") config.trigger_mode = blindside::TriggerMode::HardDefenseOnly;
            else if (mode_str == "log") config.trigger_mode = blindside::TriggerMode::LogOnly;
            else config.trigger_mode = blindside::TriggerMode::Both;
        } else if (arg == "--fps-active" && i + 1 < argc) {
            config.active_fps = std::stod(argv[++i]);
        } else if (arg == "--fps-idle" && i + 1 < argc) {
            config.idle_fps = std::stod(argv[++i]);
        } else if (arg == "--hysteresis" && i + 1 < argc) {
            config.hysteresis_sec = std::stod(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "========================================================\n"
              << "  🛡️ BLINDSIDE: Physical Privacy & Visual Eavesdropper Daemon (Phase 2)\n"
              << "========================================================\n"
              << "Mode: " << blindside::SystemTrayController::get_mode_name(config.daemon_state)
              << " | Active FPS: " << config.active_fps 
              << " | Idle FPS: " << config.idle_fps 
              << " | Hysteresis: " << config.hysteresis_sec << "s\n";

    blindside::Daemon daemon(config);
    if (synthetic_mode) {
        daemon.set_synthetic_mode(true);
    }

    if (!daemon.initialize()) {
        std::cerr << "[Blindside CLI] Daemon initialization failed!" << std::endl;
        return 1;
    }

    if (run_calibration) {
        std::cout << "[Blindside CLI] Registering primary user face position..." << std::endl;
        if (daemon.calibrate_primary_user()) {
            std::cout << "[Blindside CLI] Primary user calibration successful!" << std::endl;
        }
    }

    daemon.start();

    if (run_daemon) {
        std::cout << "[Blindside CLI] Daemon active in background. Press Ctrl+C to stop." << std::endl;
        while (!g_stop_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        // Run diagnostic verification cycle
        std::cout << "[Blindside CLI] Running diagnostic verification cycle..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (synthetic_mode) {
            std::cout << "\n[Blindside CLI] Injecting synthetic live shoulder surfer event..." << std::endl;
            daemon.inject_synthetic_eavesdropper(true, 0.0);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }

    daemon.stop();
    std::cout << "[Blindside CLI] Process exited cleanly." << std::endl;
    return 0;
}
