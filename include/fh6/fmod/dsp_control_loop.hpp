#pragma once

#include "fh6/fmod/dsp_bridge.hpp"
#include "fh6/fmod/metadata_injector.hpp"
#include "fh6/fmod/pe_image.hpp"

#include <atomic>
#include <stop_token>
#include <thread>

namespace fh6::fmod_bridge {

// 50 Hz tick: keeps the DSP installed on the current radio channel and
// drives the AudioSourceManager pump.
class ControlLoop {
public:
    ControlLoop(DSPBridge& bridge, const PEImage& img, float configured_gain);
    ~ControlLoop();

    ControlLoop(const ControlLoop&)            = delete;
    ControlLoop& operator=(const ControlLoop&) = delete;

    void set_configured_gain(float g) noexcept {
        configured_gain_.store(g, std::memory_order_release);
    }

private:
    void run(const std::stop_token& tok);
    void push_metadata() noexcept;

    DSPBridge& bridge_;
    const PEImage& img_;
    std::atomic<float> configured_gain_;
    MetadataInjector meta_;
    std::jthread thread_;
};

} // namespace fh6::fmod_bridge
