#include <iiVoiceOver.h>
#include <iiSDKCore/Process/Process.h>
#include "FileProviderAdapter.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--status") {
        std::cout << "{\"synthesisAvailable\":false,\"pcmExportAvailable\":true}\n";
        return 0;
    }
    std::filesystem::path output = "hello-world.wav";
    std::string text = "Hello World";
    bool play = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--play") play = true;
        else if (argument == "--output" && index + 1 < argc) output = argv[++index];
        else if (argument == "--text" && index + 1 < argc) text = argv[++index];
        else {
            std::cerr << "Usage: ii-voice-over [--text UTF8] [--output FILE.wav] [--play]\n";
            return 2;
        }
    }
    auto model = iiVoiceOver::VoiceModel::bundled();
    if (!model) { std::cerr << model.error().message << '\n'; return 1; }
    const auto id = model->metadata().id;
    iiVoiceOver::SpeechSynthesizer synthesizer(std::move(*model));
    auto audio = synthesizer.synthesize(text);
    if (!audio) { std::cerr << audio.error().message << '\n'; return 1; }
    auto saved = iiVoiceOverFileProvider::saveWave(*audio, output);
    if (!saved) { std::cerr << saved.error().message << '\n'; return 1; }
    std::cout << "voice=" << id << " text=\"" << text << "\" samples=" << audio->samples.size()
              << " sample_rate=" << audio->sample_rate << " channels=" << audio->channels
              << " duration_seconds=" << audio->duration().count()
              << " output=" << std::filesystem::absolute(output) << '\n';
    if (!play) return 0;
    ii::process::Launch launch;
#if defined(__APPLE__)
    launch.arguments = {"/usr/bin/afplay", std::filesystem::absolute(output).string()};
#elif defined(__linux__)
    launch.arguments = {"aplay", std::filesystem::absolute(output).string()};
#else
    std::cerr << "--play is provided on macOS (afplay) and Linux (aplay); the WAV was saved.\n";
    return 2;
#endif
    auto player = ii::process::Process::start(launch);
    if (!player) { std::cerr << player.error().message << '\n'; return 1; }
    ii::process::CommunicationOptions options;
    options.deadline = ii::core::Deadline::after(std::chrono::seconds(60));
    auto played = player->communicate({}, options);
    if (!played || played->exit.reason != ii::process::ExitReason::exited || played->exit.code != 0) {
        std::cerr << "Native audio playback failed\n";
        return 1;
    }
    std::cout << "native_playback=completed\n";
}
