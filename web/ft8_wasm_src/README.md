# FT8/FT4 WebAssembly module — build source

This folder contains the source used to build the FT8/FT4 decoder embedded
in `new_base.html` (inside the `#ft8WorkerSrc` script tag).

- `decode_wrap.c` — original C wrapper (this project, no license
  restrictions / effectively public domain — do whatever you like with it)
  that exposes `init_decode_ft8`, `init_decode_ft4`, `exec_decode_ft8`,
  `exec_decode_ft4` from unmodified [`kgoba/ft8_lib`](https://github.com/kgoba/ft8_lib)
  (MIT License) source files.

## Build

```sh
git clone https://github.com/kgoba/ft8_lib.git
emcc \
  decode_wrap.c \
  ft8_lib/ft8/decode.c \
  ft8_lib/ft8/ldpc.c \
  ft8_lib/ft8/crc.c \
  ft8_lib/ft8/message.c \
  ft8_lib/ft8/text.c \
  ft8_lib/ft8/constants.c \
  ft8_lib/common/monitor.c \
  ft8_lib/fft/kiss_fftr.c \
  ft8_lib/fft/kiss_fft.c \
  -I ft8_lib \
  -O2 \
  -D'LOG_PRINTF(...)=' \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=FT8Module \
  -s EXPORTED_FUNCTIONS=_init_decode_ft8,_init_decode_ft4,_exec_decode_ft8,_exec_decode_ft4,_malloc,_free \
  -s EXPORTED_RUNTIME_METHODS=cwrap,ccall,HEAPF32,HEAPU8,UTF8ToString \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT=web,worker,node \
  -s SINGLE_FILE=1 \
  -o ft8_wasm.js
```

The `-D'LOG_PRINTF(...)='` flag silences `ft8_lib`'s built-in debug logging
(harmless "empty body" compiler warnings are expected and can be ignored).
`ft8_wasm.js` is the output: a self-contained JS file (the WASM binary is
base64-embedded via `SINGLE_FILE=1`) that is then pasted into
`new_base.html`'s `#ft8WorkerSrc` script tag, followed by the message
handler / resampler glue code that lives in the same script tag.
