#include "iiVoiceOver.h"
#include <iiFileProviderNative.h>
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
using ii::core::ErrorCode;
using iiFileProvider::native::File;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class T> T take(ii::core::Result<T> result) {
    if (!result) throw std::runtime_error(result.error().message);
    return std::move(*result);
}
void take(ii::core::Result<void> result) {
    if (!result) throw std::runtime_error(result.error().message);
}
template<class T> void error(const ii::core::Result<T>& result, ErrorCode code) {
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
    iiFileProvider::native::TemporaryDirectory directory;
    require(directory.isValid(), "Could not create provider test directory");
    const auto output = directory.path() / "speech.wav";
    SpeechAudio pcm{{-1, -0.5F, 0, 0.5F, 1}, 22050, 1};
    take(pcm.saveWave(output));
    const auto bytes = File::read(output);
    require(bytes.size() == 54 && bytes.substr(0, 4) == "RIFF" && bytes.substr(8, 4) == "WAVE",
            "Invalid WAV container");
    const auto u8 = [&](std::size_t n) { return static_cast<unsigned char>(bytes[n]); };
    require(u8(20) == 1 && u8(22) == 1 && u8(24) == 0x22 && u8(25) == 0x56 && u8(34) == 16,
            "Invalid WAV format header");
    require(u8(44) == 0 && u8(45) == 0x80 && u8(52) == 0xff && u8(53) == 0x7f,
            "PCM16 endpoints encoded incorrectly");
    error(pcm.saveWave(output), ErrorCode::already_exists);
    require(File::read(output) == bytes, "Existing audio was overwritten");
    pcm.samples[0] = std::numeric_limits<float>::quiet_NaN();
    error(pcm.saveWave(directory.path() / "nan.wav"), ErrorCode::invalid_argument);
    require(!std::filesystem::exists(directory.path() / "nan.wav"), "Invalid audio published");
    error(SpeechAudio{}.saveWave(directory.path() / "empty.wav"), ErrorCode::invalid_argument);
    pcm.samples = {0}; pcm.channels = 2;
    error(pcm.saveWave(directory.path() / "unaligned.wav"), ErrorCode::invalid_argument);

    auto model = take(VoiceModel::bundled());
    require(model.metadata().sample_rate == 22050 && model.metadata().speakers == 1,
            "Wrong bundled voice metadata");
    const SpeechSynthesizer synth(model);
    error(synth.synthesize(""), ErrorCode::invalid_argument);
    error(synth.synthesize(std::string("Hello\0World", 11)), ErrorCode::invalid_argument);
    error(synth.synthesize(std::string("\xc0\xaf", 2)), ErrorCode::invalid_argument);
    error(synth.synthesize(std::string(4097, 'a')), ErrorCode::out_of_range);
    SpeechOptions options;
    options.speed = 0;
    error(synth.synthesize("Hello World", options), ErrorCode::invalid_argument);
    options.speed = std::numeric_limits<float>::quiet_NaN();
    error(synth.synthesize("Hello World", options), ErrorCode::invalid_argument);
    options = {}; options.speaker = 1;
    error(synth.synthesize("Hello World", options), ErrorCode::out_of_range);
    options = {}; options.maximum_samples = 0;
    error(synth.synthesize("Hello World", options), ErrorCode::invalid_argument);
    std::stop_source cancelled;
    cancelled.request_stop();
    error(synth.synthesize("Hello World", {}, cancelled.get_token()), ErrorCode::cancelled);

    auto files = model.files();
    files.configuration = directory.path() / "bad.json";
    File::create(files.configuration, "{broken");
    error(VoiceModel::load(files), ErrorCode::invalid_argument);
    files = model.files(); files.model = directory.path() / "missing.onnx";
    error(VoiceModel::load(files), ErrorCode::not_found);
}
void inference() {
    const SpeechSynthesizer synth(take(VoiceModel::bundled()));
    auto audio = take(synth.synthesize("Hello World"));
    checkAudio(audio); // A single Piper chunk may return DONE together with all samples.
    iiFileProvider::native::TemporaryDirectory directory;
    const auto wave = directory.path() / "hello-world.wav";
    take(audio.saveWave(wave));
    require(File::read(wave).size() == 44 + audio.samples.size() * 2, "Truncated speech file");
    auto second = take(synth.synthesize("Hello World. Welcome to iiVoiceOver."));
    checkAudio(second);
    require(second.duration() > audio.duration(), "Multi-sentence speech was truncated");
    SpeechOptions slower;
    slower.speed = 0.5F;
    auto slow = take(synth.synthesize("Hello World", slower));
    checkAudio(slow);
    require(slow.duration() > audio.duration() * 1.3, "Speed control has no effect");
    SpeechOptions limited;
    limited.maximum_samples = 1;
    error(synth.synthesize("Hello World", limited), ErrorCode::out_of_range);
    std::cout << "native inference: " << audio.samples.size() << " samples, "
              << audio.sample_rate << " Hz, " << audio.duration().count() << " seconds\n";
}
void concurrent() {
    const SpeechSynthesizer synth(take(VoiceModel::bundled()));
    auto first = std::async(std::launch::async, [&] { return synth.synthesize("Hello World"); });
    auto second = std::async(std::launch::async, [&] {
        const SpeechSynthesizer other(take(VoiceModel::bundled()));
        return other.synthesize("Hello World");
    });
    checkAudio(take(first.get()));
    checkAudio(take(second.get()));
    checkAudio(take(synth.synthesize("Hello World")));
}
void corruptModel() {
    auto files = take(VoiceModel::bundled()).files();
    iiFileProvider::native::TemporaryDirectory directory;
    files.model = directory.path() / "corrupt.onnx";
    File::create(files.model, "invalid ONNX model");
    const SpeechSynthesizer synth(take(VoiceModel::load(files)));
    error(synth.synthesize("Hello World"), ErrorCode::failed);
    // Failed native construction must release resources and leave phonemization usable.
    checkAudio(take(SpeechSynthesizer(take(VoiceModel::bundled())).synthesize("Hello World")));
}
}
int main(int argc, char** argv) {
    try {
        const std::string test = argc > 1 ? argv[1] : "contracts";
        if (test == "contracts") contracts();
        else if (test == "inference") inference();
        else if (test == "concurrent") concurrent();
        else if (test == "corrupt") corruptModel();
        else throw std::runtime_error("Unknown test case");
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
