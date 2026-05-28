#pragma once

#include "fh6/audio_source.hpp"
#include "fh6/config.hpp"
#include "fh6/playback_dsp.hpp"
#include "fh6/ring_buffer.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fh6::sources {

struct JellyfinTrack {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::uint64_t duration_ms = 0;
};

// Resolves a Jellyfin playlist over HTTP, then streams each item through
// `ffmpeg -f s16le -ar 48000 -ac 2`. The PCM pipe is drained by pump() in
// non-blocking mode (PeekNamedPipe-gated) so a stalled HTTP stream can never
// freeze the AudioSourceManager pump loop.
class JellyfinSource final : public IAudioSource {
public:
    JellyfinSource(JellyfinConfig cfg, std::filesystem::path ffmpeg_path);
    ~JellyfinSource() override;

    std::string_view name() const noexcept override         { return "jellyfin"; }
    std::string_view display_name() const noexcept override { return "Jellyfin"; }

    bool initialize() override;
    void shutdown() noexcept override;

    void play() override;
    void pause() override;
    void stop() override;
    void next() override;
    void previous() override;
    void pump(RingBuffer& ring) override;

    // Settings drawer hot-update; re-fetches when auth/url/playlist fields
    // change, re-shuffles in place when only `shuffle` flips.
    void set_config(JellyfinConfig cfg);
    void set_ffmpeg_path(std::filesystem::path p);

    // POST /api/source/jellyfin/cast: swap to a specific playlist id.
    // Returns false if the fetch fails (queue left untouched).
    bool cast(std::string playlist_id);

    void set_playback_options(const PlaybackConfig& opts) override;

    TrackInfo current_track() const override;
    PlaybackState playback_state() const noexcept override {
        return state_.load(std::memory_order_acquire);
    }
    AuthState auth_state() const noexcept override;
    SourceCapabilities capabilities() const noexcept override { return {false, true, true}; }

private:
    struct Pipe;

    // mu_ held.
    bool refresh_queue_locked();   // releases mu_ across the HTTP fetch, re-acquires to swap
    void start_pipe_locked();
    void stop_pipe_locked();
    void advance_locked(std::ptrdiff_t step);

    JellyfinConfig cfg_;
    std::filesystem::path ffmpeg_path_;

    mutable std::mutex mu_;
    std::vector<JellyfinTrack> queue_;
    std::size_t current_idx_ = 0;
    std::unique_ptr<Pipe> pipe_;
    std::atomic<PlaybackState> state_{PlaybackState::stopped};

    EqualizerStage eq_;
    std::atomic<bool> volume_norm_{false};
};

} // namespace fh6::sources
