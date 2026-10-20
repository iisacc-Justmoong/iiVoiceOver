# Third-party notices

iiVoiceOver's own code is MIT. Native dependencies and voice assets
retain their separate licenses. No Python runtime or Qt library is required.

| Component | Pinned source | License / notice |
| --- | --- | --- |
| Native libpiper | OHF-Voice/piper1-gpl `619f513580c04dff5ef9a680f6f3f6979ed94d2e` | GPL-3.0, upstream `COPYING` |
| eSpeak NG | espeak-ng/espeak-ng `212928b394a96e8fd2096616bfd54e17845c48f6` | GPL-3.0-or-later, upstream `COPYING`; language data retain upstream terms |
| ucd-tools | Included in the pinned eSpeak NG source | Upstream `COPYING` and Unicode `COPYING.UCD` notices |
| Open JTalk frontend | tsukumijima/open_jtalk `4a1ef70897689d2ee6b8f9be35454a3a4b15b289` | Upstream BSD / MeCab notices, copied from the native dependency |
| ONNX Runtime | Microsoft 1.22.0 native CPU distribution | MIT, `LICENSE`, `ThirdPartyNotices.txt`, `Privacy.md` |
| nlohmann/json 3.11.2 | Header supplied by pinned libpiper | MIT; Niels Lohmann; installed `nlohmann-json-MIT.txt` and header notices |
| uni-algo | Header supplied by pinned libpiper | Public domain or MIT; full installed header notice |
| en_US-ljspeech-medium | rhasspy/piper-voices `c10ece1aade47bb51c153c893d14e5bf8e5b7117` | Repository metadata: MIT; voice MODEL_CARD: public-domain LJ Speech data, trained from scratch |

The voice is an English single-speaker model. Its upstream model card and
source/hashes are preserved in `models/` and installed with the package.
The dataset identifies its text, recordings and annotations as public domain:
<https://keithito.com/LJ-Speech-Dataset/>. The model card is preserved from
<https://huggingface.co/rhasspy/piper-voices/blob/c10ece1aade47bb51c153c893d14e5bf8e5b7117/en/en_US/ljspeech/medium/MODEL_CARD>.
Other Piper voices have independent terms and are not licensed by this notice.

The build downloads native dependency sources and SHA-256-verified inference
and model archives. `cmake/PatchPiper.cmake` makes these local libpiper changes:
exception-safe construction, disabling ONNX telemetry after environment
creation, UTF-8 conversion of ONNX paths on Windows, C++23 compilation, and
restricting native ONNX library lookup to the supplied distribution.
Rebuilding this repository applies the same patch. Fetched sources remain in
`build/`; upstream notices install under `share/licenses/iiVoiceOver/`.
Open JTalk is built by libpiper, but this initial API accepts only
eSpeak-phonemized voice configurations.
