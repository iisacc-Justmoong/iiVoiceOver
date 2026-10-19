#include "iiVoiceOver.h"
#include <iiSDKCore/Core/Text/Utf8.h>
#include <iiFileProviderNative.h>
#include <piper.h>
#include <json.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <system_error>
#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
# include <windows.h>
#else
# include <dlfcn.h>
#endif

namespace iiVoiceOver {
namespace {
using ii::core::ErrorCode;
using ii::core::fail;
using iiFileProvider::native::File;
using iiFileProvider::native::FileCode;
using iiFileProvider::native::FileError;
// Piper/eSpeak has process-global mutable phonemizer state. The complete native
// session lifetime is serialized, including construction and destruction.
std::timed_mutex native_mutex;

std::string utf8Path(const std::filesystem::path& path) {
    const auto bytes = path.u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}
bool validPath(const std::filesystem::path& path) {
    if (path.empty()) return false;
    const auto value = utf8Path(path);
    return value.find('\0') == std::string::npos && ii::core::valid_utf8(value);
}
ErrorCode fileErrorCode(FileCode code) {
    switch (code) {
    case FileCode::AlreadyExists: return ErrorCode::already_exists;
    case FileCode::InvalidPath: return ErrorCode::invalid_argument;
    case FileCode::LimitExceeded: return ErrorCode::out_of_range;
    case FileCode::Conflict: return ErrorCode::busy;
    default: return ErrorCode::failed;
    }
}
Result<std::filesystem::path> packagePrefix() {
#if defined(_WIN32)
    HMODULE module{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&helloWorld), &module))
        return fail(ErrorCode::failed, "Could not locate iiVoiceOver DLL");
    std::wstring buffer(32768, L'\0');
    const auto count = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!count || count >= buffer.size())
        return fail(ErrorCode::failed, "Could not locate iiVoiceOver package");
    buffer.resize(count);
    return std::filesystem::path(buffer).parent_path().parent_path();
#else
    Dl_info info{};
    if (!dladdr(reinterpret_cast<const void*>(&helloWorld), &info) || !info.dli_fname)
        return fail(ErrorCode::failed, "Could not locate iiVoiceOver shared library");
    std::error_code error;
    auto library = std::filesystem::canonical(info.dli_fname, error);
    if (error) return fail(ErrorCode::failed, "Could not resolve iiVoiceOver package path");
    return library.parent_path().parent_path();
#endif
}
}

std::string helloWorld() { return "Hello World"; }

Result<VoiceModel> VoiceModel::load(ModelFiles files) {
    try {
        if (!validPath(files.model) || !validPath(files.phonemizer_data))
            return fail(ErrorCode::invalid_argument, "A model and eSpeak data directory are required");
        if (files.configuration.empty()) {
            files.configuration = files.model;
            files.configuration += ".json";
        }
        if (!validPath(files.configuration))
            return fail(ErrorCode::invalid_argument, "Invalid model configuration path");
        for (auto* path : {&files.model, &files.configuration, &files.phonemizer_data}) {
            std::error_code error;
            auto absolute = std::filesystem::canonical(*path, error);
            if (error) return fail(ErrorCode::not_found, "Voice resource not found: " + utf8Path(*path));
            *path = std::move(absolute);
        }
        if (!std::filesystem::is_regular_file(files.model) || !std::filesystem::is_regular_file(files.configuration))
            return fail(ErrorCode::invalid_argument, "Voice model and configuration must be regular files");
        for (const auto* filename : {"phontab", "phondata", "phonindex", "en_dict"}) {
            if (!std::filesystem::is_regular_file(files.phonemizer_data / filename))
                return fail(ErrorCode::not_found, "Incomplete eSpeak phonemizer data directory");
        }
        const auto config = nlohmann::json::parse(File::read(files.configuration, 1024 * 1024));
        if (config.value("phoneme_type", std::string("espeak")) != "espeak")
            return fail(ErrorCode::unsupported, "This API currently supports eSpeak-phonemized Piper models");
        const auto sample_rate = config.at("audio").at("sample_rate").get<std::int64_t>();
        const auto speakers = config.at("num_speakers").get<std::int64_t>();
        const auto voice = config.at("espeak").at("voice").get<std::string>();
        const auto& phonemes = config.at("phoneme_id_map");
        if (sample_rate < 8000 || sample_rate > 192000 || speakers < 1 || speakers > 65535 ||
            voice.empty() || voice.find('\0') != std::string::npos ||
            !phonemes.is_object() || phonemes.empty())
            return fail(ErrorCode::invalid_argument, "Invalid Piper voice configuration");
        for (const auto& ids : phonemes) {
            if (!ids.is_array() || ids.empty())
                return fail(ErrorCode::invalid_argument, "Invalid phoneme ID map");
            for (const auto& id : ids) {
                if (!id.is_number_integer() || id.get<std::int64_t>() < 0 ||
                    id.get<std::int64_t>() > std::numeric_limits<int>::max())
                    return fail(ErrorCode::invalid_argument, "Invalid phoneme ID");
            }
        }
        VoiceMetadata metadata{utf8Path(files.model.stem()), "",
            static_cast<std::uint32_t>(sample_rate), static_cast<std::uint32_t>(speakers)};
        if (config.contains("language")) metadata.language = config.at("language").value("code", std::string{});
        return VoiceModel(std::move(files), std::move(metadata));
    } catch (const FileError& error) {
        return fail(fileErrorCode(error.code()), error.what(), std::current_exception());
    } catch (const nlohmann::json::exception& error) {
        return fail(ErrorCode::invalid_argument, std::string("Invalid voice JSON: ") + error.what(), std::current_exception());
    } catch (const std::exception& error) {
        return fail(ErrorCode::failed, std::string("Could not load voice: ") + error.what(), std::current_exception());
    }
}

Result<VoiceModel> VoiceModel::bundled() {
    auto prefix = packagePrefix();
    if (!prefix) return std::unexpected(prefix.error());
    const auto model = *prefix / "share/iiVoiceOver/voices/en_US-ljspeech-medium/en_US-ljspeech-medium.onnx";
    return load({model, {}, *prefix / "share/espeak-ng-data"});
}

Result<SpeechAudio> SpeechSynthesizer::synthesize(std::string_view text, SpeechOptions options,
                                               std::stop_token stop) const {
    if (text.empty() || text.find('\0') != std::string_view::npos || !ii::core::valid_utf8(text))
        return fail(ErrorCode::invalid_argument, "Speech requires nonempty UTF-8 text without NUL");
    if (text.size() > 4096) return fail(ErrorCode::out_of_range, "Speech text exceeds 4096 UTF-8 bytes");
    if (!std::isfinite(options.speed) || options.speed < 0.25F || options.speed > 4 || !options.maximum_samples)
        return fail(ErrorCode::invalid_argument, "Invalid speech speed or sample limit");
    if (options.speaker >= model_.metadata().speakers)
        return fail(ErrorCode::out_of_range, "Speaker ID is outside this voice model");
    if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled");
    try {
        std::unique_lock lock(native_mutex, std::defer_lock);
        while (!lock.try_lock_for(std::chrono::milliseconds(10))) {
            if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled while waiting for the native backend");
        }
        if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled");
        const auto& files = model_.files();
        const auto model_path = utf8Path(files.model);
        const auto configuration = utf8Path(files.configuration);
        const auto data = utf8Path(files.phonemizer_data);
        std::unique_ptr<piper_synthesizer, decltype(&piper_free)> native(
            piper_create(model_path.c_str(), configuration.c_str(), data.c_str()), &piper_free);
        if (!native) return fail(ErrorCode::failed, "Native Piper voice initialization failed");
        if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled during voice initialization");
        auto settings = piper_default_synthesize_options(native.get());
        settings.length_scale /= options.speed;
        settings.speaker_id = static_cast<int>(options.speaker);
        if (!std::isfinite(settings.length_scale) || settings.length_scale <= 0 ||
            !std::isfinite(settings.noise_scale) || settings.noise_scale < 0 ||
            !std::isfinite(settings.noise_w_scale) || settings.noise_w_scale < 0)
            return fail(ErrorCode::invalid_argument, "Invalid native inference settings in voice configuration");
        const std::string owned_text(text);
        if (piper_synthesize_start(native.get(), owned_text.c_str(), &settings) != PIPER_OK)
            return fail(ErrorCode::failed, "Native phonemization failed");
        SpeechAudio audio{{}, model_.metadata().sample_rate, 1};
        for (;;) {
            if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled");
            piper_audio_chunk chunk{};
            const auto status = piper_synthesize_next(native.get(), &chunk);
            if (status != PIPER_OK && status != PIPER_DONE)
                return fail(ErrorCode::failed, "Native speech inference failed");
            if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled during native inference");
            // DONE may carry the final sentence, including the entire first and
            // only chunk. Consume its samples before deciding to stop.
            if (chunk.num_samples) {
                if (!chunk.samples || chunk.sample_rate != static_cast<int>(audio.sample_rate))
                    return fail(ErrorCode::failed, "Native inference returned an invalid PCM format");
                if (chunk.num_samples > options.maximum_samples - audio.samples.size())
                    return fail(ErrorCode::out_of_range, "Synthesized speech exceeds the retained sample limit");
                for (std::size_t index = 0; index < chunk.num_samples; ++index) {
                    const float value = chunk.samples[index];
                    if (!std::isfinite(value)) return fail(ErrorCode::failed, "Native inference returned a nonfinite sample");
                    audio.samples.push_back(std::clamp(value, -1.0F, 1.0F));
                }
            }
            if (status == PIPER_DONE) break;
        }
        if (audio.samples.empty()) return fail(ErrorCode::failed, "The supplied text produced no speech");
        return audio;
    } catch (const std::exception& error) {
        return fail(ErrorCode::failed, std::string("Native speech synthesis failed: ") + error.what(), std::current_exception());
    }
}

std::chrono::duration<double> SpeechAudio::duration() const noexcept {
    if (!sample_rate || !channels) return {};
    return std::chrono::duration<double>(double(samples.size()) / sample_rate / channels);
}

Result<void> SpeechAudio::saveWave(const std::filesystem::path& destination) const {
    if (!validPath(destination) || samples.empty() || sample_rate < 8000 || sample_rate > 192000 ||
        (channels != 1 && channels != 2) || samples.size() % channels)
        return fail(ErrorCode::invalid_argument, "Invalid WAV destination or interleaved PCM format");
    if (samples.size() > (std::numeric_limits<std::uint32_t>::max() - 36ULL) / 2)
        return fail(ErrorCode::out_of_range, "Audio exceeds the RIFF/WAV size limit");
    if (std::any_of(samples.begin(), samples.end(), [](float value) {
            return !std::isfinite(value) || value < -1 || value > 1;
        })) return fail(ErrorCode::invalid_argument, "PCM samples must be finite and within [-1, 1]");
    try {
        const auto size = static_cast<std::uint32_t>(samples.size() * 2);
        std::string wave(44 + static_cast<std::size_t>(size), '\0');
        wave.replace(0, 4, "RIFF"); wave.replace(8, 8, "WAVEfmt "); wave.replace(36, 4, "data");
        const auto little = [&](std::size_t offset, std::uint32_t value, unsigned count) {
            for (unsigned index = 0; index < count; ++index)
                wave[offset + index] = static_cast<char>((value >> (index * 8)) & 0xff);
        };
        little(4, 36 + size, 4); little(16, 16, 4); little(20, 1, 2); little(22, channels, 2);
        little(24, sample_rate, 4); little(28, sample_rate * channels * 2, 4);
        little(32, channels * 2, 2); little(34, 16, 2); little(40, size, 4);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const auto value = static_cast<std::int16_t>(samples[index] <= -1 ? -32768 : std::lround(samples[index] * 32767));
            little(44 + index * 2, static_cast<std::uint16_t>(value), 2);
        }
        File::create(destination, wave);
        return {};
    } catch (const FileError& error) {
        return fail(fileErrorCode(error.code()), error.what(), std::current_exception());
    } catch (const std::exception& error) {
        return fail(ErrorCode::failed, std::string("Could not publish speech WAV: ") + error.what(), std::current_exception());
    }
}

} // namespace iiVoiceOver
