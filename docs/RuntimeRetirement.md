# 폐기한 모델 런타임

2026-10-27부터 ONNX·ONNX Runtime을 빌드·모델 실행·설치·배포 경로에서 제거한다.
선택적 설치나 자동 다운로드 경로도 제공하지 않는다. 과거 검증 기록은 현재 지원
선언이 아니며, 해당 모델을 지원하는 것으로 간주하지 않는다.

Piper 합성과 번들 음성 모델을 제거한다. `SpeechSynthesizer::available()`은
`false`이며, 음성 모델 로딩과 합성은 지원 불가 결과를 반환한다. PCM 재생 데이터,
길이 계산과 메모리 WAV 인코딩은 유지한다.
`SpeechAudio::waveBytes()`는 파일을 만들지 않고 같은 유효성 검사와 PCM16 WAV
인코딩 결과를 반환한다. 기존 `saveWave(path)` 멤버는 제거했으며, 별도 소비자
`examples/file-provider`가 iiFileProvider의 독점 생성 경로로 게시한다. 호출자는
어댑터 또는 `waveBytes()` 사용으로 이전하고 다시 빌드해야 한다.
WAV 저장 계약 검사의 임시 디렉터리는 `resources/` 아래에서 제공자가 만들고
검사가 끝나면 정리한다.

별도 `examples/native-cli`의 `ii-voice-over --status`는 합성 불가와 PCM 내보내기
가용성을 JSON으로 노출한다. 본체 `iiVoiceOver.contracts`는 인코딩을 검사하고,
별도 `iiVoiceOver.FileProvider` 검사는 파일 저장 및 덮어쓰기 거부를 검사한다.

2026-10-28 macOS arm64에서 두 SDK 검색을 비활성화한 CMake 빌드와 본체
경계·PCM·UTF-8 계약 검사 2/2가 통과했다. 별도 직접 컴파일한 동일 PCM
검사도 통과했다. 격리 설치본은 두 SDK 검색을 차단한 소비자에서 컴파일했고,
파일 제공자 조합 검사는 WAV 내용·독점 생성·잘못된 입력 거부를 통과했다.
CLI는 별도 소비자로 컴파일만 했다. 실제 모델 추론, 음성 재생 및 GUI 앱은
실행하지 않았다.

iiDecision·iiLocalDiffusion·iiVoiceOver의 설치 갱신은 해당 SDK 접두사에 남은 구형
ONNX 공유 라이브러리를 제거한다. 제품 공통 배포 정책은 ONNX 런타임, 기존 Piper
바이너리와 OpenCV DNN의 재포함을 거부한다. 폐기한 모델 기능이 필요하면 지원되는
네이티브 API와 교체 가능한 iiSDK 인터페이스에 별도 구현을 연결해야 한다.
