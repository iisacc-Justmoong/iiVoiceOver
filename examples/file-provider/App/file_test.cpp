#include "FileProviderAdapter.h"
#include <iiFileProviderPresentation.h>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace iiVoiceOver;
void check(bool value) { if (!value) throw std::runtime_error("voice file contract failed"); }
int main() {
    try {
        iiFileProvider::native::TemporaryDirectory directory;
        check(directory.isValid());
        const auto output = directory.path() / "speech.wav";
        SpeechAudio pcm{{-1, -0.5F, 0, 0.5F, 1}, 22050, 1};
        const auto encoded = pcm.waveBytes(); check(bool(encoded));
        check(bool(iiVoiceOverFileProvider::saveWave(pcm, output)));
        const auto bytes = iiFileProvider::native::File::read(output); check(bytes == *encoded);
        const auto duplicate = iiVoiceOverFileProvider::saveWave(pcm, output);
        check(!duplicate && duplicate.error().code == ErrorCode::already_exists);
        check(iiFileProvider::native::File::read(output) == bytes);
        pcm.samples[0] = std::numeric_limits<float>::quiet_NaN();
        const auto invalid = iiVoiceOverFileProvider::saveWave(pcm, directory.path() / "nan.wav");
        check(!invalid && invalid.error().code == ErrorCode::invalid_argument);
        check(!std::filesystem::exists(directory.path() / "nan.wav"));
        const auto noPath = iiVoiceOverFileProvider::saveWave(pcm, {});
        check(!noPath && noPath.error().code == ErrorCode::invalid_argument);
        iiFileProvider::native::Presentation::cancelPending(output);
        check(iiFileProvider::native::File::remove(output));
        std::cout << "voice file composition passed\n";
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
