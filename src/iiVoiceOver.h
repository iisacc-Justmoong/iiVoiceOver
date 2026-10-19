#pragma once

#include <iiSDKCore/Core/Result/Result.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
# if defined(IIVOICEOVER_BUILDING_LIBRARY)
#  define IIVOICEOVER_EXPORT __declspec(dllexport)
# else
#  define IIVOICEOVER_EXPORT __declspec(dllimport)
# endif
#else
# define IIVOICEOVER_EXPORT __attribute__((visibility("default")))
#endif

namespace iiVoiceOver {

using ii::core::Result;

struct ModelFiles final {
    std::filesystem::path model;
    std::filesystem::path configuration; // Empty means model + ".json".
    std::filesystem::path phonemizer_data; // The espeak-ng-data directory.
};
struct VoiceMetadata final {
    std::string id;
    std::string language;
    std::uint32_t sample_rate{};
    std::uint32_t speakers{};
};

// Immutable, copyable description. load() validates paths and configuration;
// the native ONNX session is constructed when speech is requested.
class IIVOICEOVER_EXPORT VoiceModel final {
public:
    [[nodiscard]] static Result<VoiceModel> load(ModelFiles files);
    [[nodiscard]] static Result<VoiceModel> bundled();
    [[nodiscard]] const ModelFiles& files() const noexcept { return files_; }
    [[nodiscard]] const VoiceMetadata& metadata() const noexcept { return metadata_; }
private:
    VoiceModel(ModelFiles files, VoiceMetadata metadata)
        : files_(std::move(files)), metadata_(std::move(metadata)) {}
    ModelFiles files_;
    VoiceMetadata metadata_;
};

struct SpeechOptions final {
    float speed = 1.0F; // [0.25, 4]; 2 means twice as fast.
    std::uint32_t speaker = 0;
    std::size_t maximum_samples = 2'880'000; // Retained output limit, not an ONNX memory budget.
};
struct IIVOICEOVER_EXPORT SpeechAudio final {
    std::vector<float> samples; // Interleaved float32 PCM, [-1, 1].
    std::uint32_t sample_rate{};
    std::uint16_t channels = 1;
    [[nodiscard]] std::chrono::duration<double> duration() const noexcept;
    // PCM16 little-endian WAV. Atomic exclusive publication via iiFileProvider;
    // an existing destination is never replaced. Parent directories must exist.
    [[nodiscard]] Result<void> saveWave(const std::filesystem::path& destination) const;
};

class IIVOICEOVER_EXPORT SpeechSynthesizer final {
public:
    explicit SpeechSynthesizer(VoiceModel model) : model_(std::move(model)) {}
    [[nodiscard]] const VoiceModel& model() const noexcept { return model_; }
    // Blocking native CPU inference. UTF-8, 1..4096 bytes, no embedded NUL.
    // Safe from multiple callers: the process-global eSpeak backend serializes
    // native sessions. Run this on a worker for a GUI/runtime event loop.
    // Cancellation is checked while waiting and between native sentence chunks;
    // an ONNX Run already in progress is allowed to finish. No partial audio is returned.
    [[nodiscard]] Result<SpeechAudio> synthesize(std::string_view text,
        SpeechOptions options = {}, std::stop_token stop = {}) const;
private:
    VoiceModel model_;
};

[[nodiscard]] IIVOICEOVER_EXPORT std::string helloWorld();

} // namespace iiVoiceOver
