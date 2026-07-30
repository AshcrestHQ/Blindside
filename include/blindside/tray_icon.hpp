#ifndef BLINDSIDE_TRAY_ICON_HPP
#define BLINDSIDE_TRAY_ICON_HPP

#include "blindside/config.hpp"
#include <atomic>
#include <iostream>
#include <string>

namespace blindside {

class SystemTrayController {
public:
    explicit SystemTrayController(std::atomic<DaemonState>& shared_state)
        : shared_state_(shared_state) {}

    ~SystemTrayController() = default;

    /**
     * @brief Lock-free atomic update of daemon operating mode.
     */
    void set_mode(DaemonState mode) {
        shared_state_.store(mode, std::memory_order_release);
        std::cout << "[SystemTray] Daemon state updated to: " << get_mode_name(mode) << std::endl;
    }

    DaemonState get_current_mode() const {
        return shared_state_.load(std::memory_order_acquire);
    }

    static const char* get_mode_name(DaemonState mode) {
        switch (mode) {
            case DaemonState::StrictFullLock:       return "Strict Mode (Full OS Lock)";
            case DaemonState::GracefulTargetedBlur: return "Graceful Mode (Targeted Window Blur)";
            case DaemonState::PauseDetection:        return "Pause Detection";
        }
        return "Unknown";
    }

private:
    std::atomic<DaemonState>& shared_state_;
};

} // namespace blindside

#endif // BLINDSIDE_TRAY_ICON_HPP
