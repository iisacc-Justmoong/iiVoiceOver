#include <iiVoiceOver.h>
#include <iostream>
int main() {
    const iiVoiceOver::SpeechAudio audio{{0.0F}, 22050, 1};
    if (!audio.waveBytes()) return 1;
    if (iiVoiceOver::SpeechSynthesizer::available()) return 1;
    auto result=iiVoiceOver::VoiceModel::bundled();
    if (result || result.error().code!=iiVoiceOver::ErrorCode::unsupported) return 1;
    std::cout << "Installed PCM API reports retired speech model backend\n";
}
