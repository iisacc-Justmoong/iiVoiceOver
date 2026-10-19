#include <iiVoiceOver.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <type_traits>

static_assert(__cplusplus > 202002L);
static_assert(std::is_same_v<decltype(iiVoiceOver::helloWorld()), std::string>);
int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("Expected package prefix and output directory");
        auto model = iiVoiceOver::VoiceModel::bundled();
        if (!model) throw std::runtime_error(model.error().message);
        const auto prefix = std::filesystem::canonical(argv[1]);
        if (model->files().model != prefix / "share/iiVoiceOver/voices/en_US-ljspeech-medium/en_US-ljspeech-medium.onnx" ||
            model->files().phonemizer_data != prefix / "share/espeak-ng-data")
            throw std::runtime_error("Voice resources were resolved outside the installed package");
        iiVoiceOver::SpeechSynthesizer synth(std::move(*model));
        auto audio = synth.synthesize("Hello World");
        if (!audio) throw std::runtime_error(audio.error().message);
        if (audio->samples.empty() || audio->sample_rate != 22050 || audio->channels != 1)
            throw std::runtime_error("Installed native inference returned no speech");
        double energy = 0;
        for (float sample : audio->samples) energy += double(sample) * sample;
        if (std::sqrt(energy / audio->samples.size()) < 0.005)
            throw std::runtime_error("Installed native inference returned silence");
        const auto unique = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const auto output = std::filesystem::path(argv[2]) / ("installed-hello-world-" + unique + ".wav");
        auto saved = audio->saveWave(output);
        if (!saved) throw std::runtime_error(saved.error().message);
        if (std::filesystem::file_size(output) != 44 + audio->samples.size() * 2)
            throw std::runtime_error("Installed consumer WAV was truncated");
        std::cout << "installed native speech: " << audio->samples.size() << " samples, "
                  << audio->duration().count() << " seconds, " << output << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
