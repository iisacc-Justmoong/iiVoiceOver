#pragma once
#include <iiVoiceOver.h>
#include <iiFileProviderNative.h>
#include <filesystem>
#include <string>

namespace iiVoiceOverFileProvider {
inline iiVoiceOver::Result<void> saveWave(const iiVoiceOver::SpeechAudio &audio,
                                          const std::filesystem::path &destination) {
    using namespace iiVoiceOver;
    try {
        const auto path = destination.u8string();
        const std::string value(reinterpret_cast<const char *>(path.data()), path.size());
        if (value.empty() || value.find('\0') != std::string::npos || !isValidUtf8(value))
            return std::unexpected(Error{ErrorCode::invalid_argument, "Invalid WAV destination"});
        auto wave = audio.waveBytes();
        if (!wave) return std::unexpected(wave.error());
        iiFileProvider::native::File::create(destination, *wave);
        return {};
    } catch (const iiFileProvider::native::FileError &error) {
        ErrorCode code = ErrorCode::failed;
        switch (error.code()) {
        case iiFileProvider::native::FileCode::AlreadyExists: code = ErrorCode::already_exists; break;
        case iiFileProvider::native::FileCode::InvalidPath: code = ErrorCode::invalid_argument; break;
        case iiFileProvider::native::FileCode::LimitExceeded: code = ErrorCode::out_of_range; break;
        case iiFileProvider::native::FileCode::Conflict: code = ErrorCode::busy; break;
        default: break;
        }
        return std::unexpected(Error{code, error.what(), std::current_exception()});
    } catch (const std::exception &error) {
        return std::unexpected(Error{ErrorCode::failed,
            std::string("Could not publish speech WAV: ") + error.what(), std::current_exception()});
    }
}
} // namespace iiVoiceOverFileProvider
