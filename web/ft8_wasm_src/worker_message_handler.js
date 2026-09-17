
/* ==== FT8/FT4 decode worker message handler (Si473x Radio integration) ====
 * This module (the WebAssembly build above) is compiled from kgoba/ft8_lib
 * (MIT License, Copyright (c) 2018 Karlis Goba -- https://github.com/kgoba/ft8_lib)
 * via a small original C wrapper (decode_wrap.c, MIT, written for this
 * project) using Emscripten. It replaces the previous @e04/ft8ts
 * (GPL-3.0) based implementation so this project is not required to be
 * distributed under GPL-3.0.
 *
 * The resampling code below (buildLowpassKernel/applyFir/resampleTo12k) is
 * original code written for this project (not derived from ft8_lib, ft8ts,
 * or any other third-party decoder) -- kgoba/ft8_lib's decoder is fixed to
 * a 12kHz input rate, so incoming audio (commonly 24kHz from this app's own
 * WebSocket audio path, or whatever rate the browser's AudioContext uses
 * for the HTTP stream path) is downsampled to 12kHz before decoding.
 */
(function () {
  var mod = null;
  var pendingMsgs = [];
  var DEPTH_PARAMS = {
    1: [16, 20, 100],
    2: [10, 25, 200],
    3: [8, 35, 300],
    4: [5, 50, 400],
  };

  function buildLowpassKernel(cutoffHz, sampleRate, numTaps) {
    var kernel = new Float32Array(numTaps);
    var mid = (numTaps - 1) / 2;
    var fc = cutoffHz / sampleRate; // normalized cutoff (0..0.5)
    var sum = 0;
    for (var i = 0; i < numTaps; i++) {
      var x = i - mid;
      var sinc = x === 0 ? 2 * fc : Math.sin(2 * Math.PI * fc * x) / (Math.PI * x);
      var w = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / (numTaps - 1)); // Hann window
      var v = sinc * w;
      kernel[i] = v;
      sum += v;
    }
    if (sum !== 0) {
      for (var j = 0; j < numTaps; j++) kernel[j] /= sum; // unity DC gain
    }
    return kernel;
  }

  function applyFir(samples, kernel) {
    var n = samples.length,
      k = kernel.length,
      half = (k - 1) >> 1;
    var out = new Float32Array(n);
    for (var i = 0; i < n; i++) {
      var acc = 0;
      for (var j = 0; j < k; j++) {
        var idx = i + j - half;
        if (idx >= 0 && idx < n) acc += samples[idx] * kernel[j];
      }
      out[i] = acc;
    }
    return out;
  }

  // Anti-alias low-pass (when downsampling) + linear-interpolation resample
  // to the fixed 12kHz rate the WASM decoder requires.
  function resampleTo12k(samples, srcRate) {
    var TARGET = 12000;
    if (srcRate === TARGET) return samples;
    if (srcRate > TARGET) {
      var cutoff = Math.min(5800, srcRate * 0.45);
      var kernel = buildLowpassKernel(cutoff, srcRate, 63);
      samples = applyFir(samples, kernel);
    }
    var ratio = srcRate / TARGET;
    var outLen = Math.floor(samples.length / ratio);
    var out = new Float32Array(outLen);
    for (var i = 0; i < outLen; i++) {
      var srcPos = i * ratio;
      var i0 = Math.floor(srcPos);
      var frac = srcPos - i0;
      var s0 = samples[i0] || 0;
      var s1 = samples[i0 + 1] !== undefined ? samples[i0 + 1] : s0;
      out[i] = s0 + (s1 - s0) * frac;
    }
    return out;
  }

  function handleDecode(d) {
    var id = d.id;
    if (!mod) {
      pendingMsgs.push(d);
      return;
    }
    try {
      var isFt4 = d.mode === "FT4";
      var sig12k = resampleTo12k(d.samples, d.sampleRate || 12000);
      var n = sig12k.length;

      var ptrSig = mod._malloc(n * 4);
      mod.HEAPF32.set(sig12k, ptrSig >> 2);
      var ptrRes = mod._malloc(4096);

      var p = DEPTH_PARAMS[d.depth] || DEPTH_PARAMS[2];
      var t0 = self.performance && performance.now ? performance.now() : Date.now();
      if (isFt4) {
        mod._init_decode_ft4();
        mod._exec_decode_ft4(ptrSig, n, p[0], p[1], p[2], ptrRes);
      } else {
        mod._init_decode_ft8();
        mod._exec_decode_ft8(ptrSig, n, p[0], p[1], p[2], ptrRes);
      }
      var ms = (self.performance && performance.now ? performance.now() : Date.now()) - t0;
      var resultsStr = mod.UTF8ToString(ptrRes);
      mod._free(ptrSig);
      mod._free(ptrRes);

      var results = [];
      var lines = resultsStr.split("\n");
      for (var i = 0; i < lines.length; i++) {
        var line = lines[i];
        if (!line) continue;
        var c1 = line.indexOf(",");
        var c2 = line.indexOf(",", c1 + 1);
        var c3 = line.indexOf(",", c2 + 1);
        if (c1 < 0 || c2 < 0 || c3 < 0) continue;
        results.push({
          snr: parseFloat(line.slice(0, c1)),
          dt: parseFloat(line.slice(c1 + 1, c2)),
          freq: parseFloat(line.slice(c2 + 1, c3)),
          msg: line.slice(c3 + 1),
        });
      }
      self.postMessage({ id: id, ok: true, results: results, ms: ms });
    } catch (e) {
      self.postMessage({ id: id, ok: false, error: (e && e.message) ? e.message : String(e) });
    }
  }

  self.onmessage = function (ev) {
    handleDecode(ev.data || {});
  };

  FT8Module()
    .then(function (m) {
      mod = m;
      self.postMessage({ ready: true });
      var q = pendingMsgs;
      pendingMsgs = [];
      for (var i = 0; i < q.length; i++) handleDecode(q[i]);
    })
    .catch(function (e) {
      self.postMessage({ ready: false, error: (e && e.message) ? e.message : String(e) });
    });
})();
