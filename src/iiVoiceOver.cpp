#include "iiVoiceOver.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace iiVoiceOver {
namespace {
std::unexpected<Error> fail(ErrorCode code, std::string message, std::exception_ptr cause = {}) {
    return std::unexpected(Error{code, std::move(message), std::move(cause)});
}
// Unicode scalar UTF-8: reject overlong sequences, surrogates and truncation.
bool validUtf8(std::string_view text) noexcept {
    std::size_t index = 0;
    while (index < text.size()) {
        const auto first = static_cast<unsigned char>(text[index++]);
        if (first < 0x80) continue;
        unsigned count = 0, value = 0, minimum = 0;
        if (first >= 0xc2 && first <= 0xdf) { count = 1; value = first & 0x1f; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { count = 2; value = first & 0x0f; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { count = 3; value = first & 7; minimum = 0x10000; }
        else return false;
        if (count > text.size() - index) return false;
        while (count--) {
            const auto next = static_cast<unsigned char>(text[index++]);
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return false;
    }
    return true;
}
}

bool isValidUtf8(std::string_view text) noexcept { return validUtf8(text); }

std::string helloWorld() { return "Hello World"; }

Result<VoiceModel> VoiceModel::load(ModelFiles files) {
    if (files.model.empty()) return fail(ErrorCode::invalid_argument, "A voice model is required");
    return fail(ErrorCode::unsupported, "The previous voice model backend has been retired");
}
Result<VoiceModel> VoiceModel::bundled() {
    return fail(ErrorCode::unsupported, "The bundled learned voice backend has been retired");
}
bool SpeechSynthesizer::available() noexcept { return false; }
Result<SpeechAudio> SpeechSynthesizer::synthesize(std::string_view text, SpeechOptions options,
                                               std::stop_token stop) const {
    if (text.empty() || text.find('\0') != std::string_view::npos || !isValidUtf8(text))
        return fail(ErrorCode::invalid_argument, "Speech requires nonempty UTF-8 text without NUL");
    if (stop.stop_requested()) return fail(ErrorCode::cancelled, "Speech cancelled");
    (void)options;
    return fail(ErrorCode::unsupported, "The learned voice backend has been retired");
}

std::chrono::duration<double> SpeechAudio::duration() const noexcept {
    if (!sample_rate || !channels) return {};
    return std::chrono::duration<double>(double(samples.size()) / sample_rate / channels);
}

Result<std::string> SpeechAudio::waveBytes() const {
    if (samples.empty() || sample_rate < 8000 || sample_rate > 192000 ||
        (channels != 1 && channels != 2) || samples.size() % channels)
        return fail(ErrorCode::invalid_argument, "Invalid interleaved PCM format");
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
        return wave;
    } catch (const std::exception& error) {
        return fail(ErrorCode::failed, std::string("Could not encode speech WAV: ") + error.what(), std::current_exception());
    }
}

} // namespace iiVoiceOver
