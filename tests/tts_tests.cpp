#include "iiVoiceOver.h"
#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

static_assert(__cplusplus > 202002L, "iiVoiceOver requires C++23");
namespace {
using namespace iiVoiceOver;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class T> T take(Result<T> result) {
    if (!result) throw std::runtime_error(result.error().message);
    return std::move(*result);
}
void take(Result<void> result) {
    if (!result) throw std::runtime_error(result.error().message);
}
template<class T> void error(const Result<T>& result, ErrorCode code) {
    require(!result && result.error().code == code, "Unexpected error result");
}
void checkAudio(const SpeechAudio& audio) {
    require(audio.sample_rate == 22050 && audio.channels == 1, "Wrong PCM format");
    require(audio.duration().count() > 0.25 && audio.duration().count() < 12, "Wrong duration");
    double energy = 0;
    for (float value : audio.samples) {
        require(std::isfinite(value) && std::abs(value) <= 1, "Invalid PCM sample");
        energy += double(value) * value;
    }
    require(std::sqrt(energy / audio.samples.size()) > 0.005, "Speech is silent");
}
void contracts() {
    require(helloWorld() == "Hello World", "Wrong greeting");
    error(VoiceModel::load({}), ErrorCode::invalid_argument);
    SpeechAudio pcm{{-1, -0.5F, 0, 0.5F, 1}, 22050, 1};
    const auto bytes = take(pcm.waveBytes());
    require(bytes.size() == 54 && bytes.substr(0, 4) == "RIFF" && bytes.substr(8, 4) == "WAVE",
            "Invalid WAV container");
    const auto u8 = [&](std::size_t n) { return static_cast<unsigned char>(bytes[n]); };
    require(u8(20) == 1 && u8(22) == 1 && u8(24) == 0x22 && u8(25) == 0x56 && u8(34) == 16,
            "Invalid WAV format header");
    require(u8(44) == 0 && u8(45) == 0x80 && u8(52) == 0xff && u8(53) == 0x7f,
            "PCM16 endpoints encoded incorrectly");
    pcm.samples[0] = std::numeric_limits<float>::quiet_NaN();
    error(pcm.waveBytes(), ErrorCode::invalid_argument);
    error(SpeechAudio{}.waveBytes(), ErrorCode::invalid_argument);
    pcm.samples = {0}; pcm.channels = 2;
    error(pcm.waveBytes(), ErrorCode::invalid_argument);

    require(isValidUtf8("Hello \xed\x95\x9c\xea\xb5\xad \xf0\x9f\x8c\x8d"), "Valid UTF-8 rejected");
    require(isValidUtf8(std::string_view("\0", 1)), "UTF-8 NUL rejected");
    require(!isValidUtf8("\xc0\xaf") && !isValidUtf8("\xed\xa0\x80") &&
            !isValidUtf8("\xf4\x90\x80\x80") && !isValidUtf8("\xe2\x82"), "Invalid UTF-8 accepted");

    error(VoiceModel::bundled(), ErrorCode::unsupported);
    error(VoiceModel::load({"retired-model", {}, {}}), ErrorCode::unsupported);
    require(!SpeechSynthesizer::available(), "Retired model inference advertised as available");
}
}
int main() {
    try { contracts(); std::cout << "PCM export and retired model boundary passed\n"; return 0; }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
