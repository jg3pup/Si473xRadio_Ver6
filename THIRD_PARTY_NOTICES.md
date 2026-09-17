# Third-Party Notices

This project embeds one piece of third-party open source code. This file
documents what is embedded, under what license, and what was changed.

## kgoba/ft8_lib (FT8/FT4 decoder, compiled to WebAssembly)

- **What**: The DSP/decode routines used by the "FT8/FT4自動判読機能"
  feature (in `new_base.html`, compiled to a WebAssembly module and
  embedded inline inside a `<script type="text/plain" id="ft8WorkerSrc">`
  tag, run in a Web Worker created from it at runtime).
- **Upstream project**: https://github.com/kgoba/ft8_lib
- **License**: **MIT License**, Copyright (c) 2018 Kārlis Goba. The full
  upstream license text is reproduced verbatim below. Because it is MIT
  (not GPL/copyleft), embedding it does **not** require this repository or
  any other part of this project to be released under any particular
  license — MIT only requires keeping the copyright/permission notice
  (satisfied by this file).
- **How it's embedded**: Not used as-is. A small original C wrapper file
  (`decode_wrap.c`, written for this project, also MIT-style/no
  restrictions) calls into unmodified upstream `ft8_lib` source files
  (`ft8/decode.c`, `ft8/ldpc.c`, `ft8/crc.c`, `ft8/message.c`,
  `ft8/text.c`, `ft8/constants.c`, `common/monitor.c`, `fft/kiss_fft.c`,
  `fft/kiss_fftr.c` — none of these upstream files were modified) and
  exposes four functions (`init_decode_ft8`, `init_decode_ft4`,
  `exec_decode_ft8`, `exec_decode_ft4`) via Emscripten, producing a single
  WebAssembly module usable from JavaScript for both FT8 and FT4 decoding
  (the upstream project's own demo CLI only builds one mode at a time).
- **Resampling**: `ft8_lib`'s decoder requires exactly 12kHz input. Because
  this app's audio can arrive at other rates (e.g. 24kHz from the ESP32
  WebSocket audio path), an original anti-alias-filter + resample-to-12kHz
  routine (not derived from `ft8_lib`, `ft8ts`, or any other third-party
  decoder) is included directly in the Worker's JavaScript.
- **Signal strength column**: The "強度(目安)" value shown in the results
  table is `candidate_score * 0.5`, the same rough approximation used by
  `ft8_lib`'s own demo/reference decoder (its source code literally labels
  this a `// TODO: compute better approximation of SNR`). It is a relative
  score for comparing signals within one decode, not a calibrated dB SNR
  value — this is an inherent limitation of the upstream library, not
  something introduced by this project.

### Upstream MIT license text (verbatim)

```
MIT License

Copyright (c) 2018 Kārlis Goba

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## How to obtain the unmodified upstream source

`git clone https://github.com/kgoba/ft8_lib.git` (used at commit reachable
via the default branch as of 2026-08). The unmodified upstream `.c`/`.h`
files listed above, together with this project's own `decode_wrap.c`
wrapper, are what the embedded WebAssembly module in `new_base.html` was
built from.

## Note on this project's own license

This repository's own `LICENSE` (the existing "Si473X Radio Ver6
ソフトウェア使用許諾書") does **not** need to change because of this
feature. It previously could not have embedded GPL-3.0 code without
conflicting with its own terms; `ft8_lib` being MIT avoids that problem
entirely, since MIT-licensed code may be embedded in a project under any
license, including a proprietary all-rights-reserved one.
