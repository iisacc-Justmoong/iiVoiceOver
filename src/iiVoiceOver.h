#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
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

enum class ErrorCode { invalid_argument, out_of_range, not_found, already_exists, full,
    cycle, unsupported, failed, cancelled, closed, busy, wrong_executor };
struct Error final { ErrorCode code; std::string message; std::exception_ptr cause{}; };
template<class T> using Result = std::expected<T, Error>;
// Validates Unicode scalar UTF-8, including embedded NUL as valid UTF-8.
[[nodiscard]] IIVOICEOVER_EXPORT bool isValidUtf8(std::string_view text) noexcept;

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

// Compatibility model descriptor. The previous learned voice backend is retired;
// loading a model returns an explicit unsupported result.
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
    std::size_t maximum_samples = 2'880'000; // Retained PCM output limit.
};
struct IIVOICEOVER_EXPORT SpeechAudio final {
    std::vector<float> samples; // Interleaved float32 PCM, [-1, 1].
    std::uint32_t sample_rate{};
    std::uint16_t channels = 1;
    [[nodiscard]] std::chrono::duration<double> duration() const noexcept;
    // Encode a PCM16 little-endian WAV without performing file I/O.
    [[nodiscard]] Result<std::string> waveBytes() const;
};

class IIVOICEOVER_EXPORT SpeechSynthesizer final {
public:
    explicit SpeechSynthesizer(VoiceModel model) : model_(std::move(model)) {}
    [[nodiscard]] const VoiceModel& model() const noexcept { return model_; }
    [[nodiscard]] static bool available() noexcept;
    // Retired model inference returns unsupported; PCM export is independent.
    [[nodiscard]] Result<SpeechAudio> synthesize(std::string_view text,
        SpeechOptions options = {}, std::stop_token stop = {}) const;
private:
    VoiceModel model_;
};

[[nodiscard]] IIVOICEOVER_EXPORT std::string helloWorld();

} // namespace iiVoiceOver
