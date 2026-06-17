/*
 * Self-contained boot harness for the KiCad Gerber viewer (gerbview) WASM,
 * embedded in the blog post via an <iframe>. Adapted from the proven test
 * harness (tests/apps/kicad/gerbview.html) and the React port
 * (web/standalone/src/wasm/boot.ts) — same global `Module`, same preRun steps,
 * same wx.js -> wx-dom.js -> gerbview.js injection order.
 *
 * Classic script (NOT a module): the non-modularized WASM glue reads a GLOBAL
 * `var Module` and a GLOBAL `mainWindow`, and publishes `FS` onto the global
 * scope. Top-level `var`/`const` here share that global scope.
 *
 * Asset layout, split because the pthread worker script MUST be same-origin
 * (a Worker can't be created from a cross-origin URL). So the small glue stays
 * on the site origin and only the big binaries move to Cloudflare R2:
 *   GLUE_BASE   : wx.js, wx-dom.js, gerbview.js, mainScriptUrlOrBlob (same-origin)
 *   BINARY_BASE : gerbview.wasm, kicad-resources.bin (R2 in prod, local in dev)
 *
 * BINARY_BASE auto-switches by hostname: localhost → the local mirror (./wasm),
 * any other host → R2_BASE below. Override either base with ?glue=/?bin=.
 * To point at R2, set R2_BASE to your bucket's public URL — either a custom
 * domain (https://assets.pcbjam.com) or the managed https://<id>.r2.dev URL.
 */

(function () {
  "use strict";

  // R2 bucket public URL (no trailing slash). Custom domain for the
  // pcbjam-assets bucket (the managed URL https://pub-cecc0239e6f74d99ba7d06630bd87c64.r2.dev
  // also still works).
  var R2_BASE = "https://assets.pcbjam.com";

  var params = new URLSearchParams(location.search);
  var isLocal = /^(localhost|127\.0\.0\.1|\[?::1\]?)$/.test(location.hostname);
  var GLUE_BASE = (params.get("glue") || "./wasm").replace(/\/+$/, "");
  var BINARY_BASE = (params.get("bin") || (isLocal ? "./wasm" : R2_BASE)).replace(
    /\/+$/,
    ""
  );

  // KiCad paths baked into the WASM build (see web/standalone/src/wasm/constants.ts).
  var KICAD_VERSION_DIR = "9.99";
  var KICAD_CONFIG_DIR =
    "/home/kicad/.config/kicad/kicad/" + KICAD_VERSION_DIR;
  var RESOURCE_PATH =
    "/workspace/build-wasm/sysroot/share/kicad/resources";

  // The board to auto-open: all tiny_tapeout layers (bottom→top for a sensible
  // draw order). The .gbrjob is preloaded too but we open the individual layer
  // files (gerbview opens gerber/drill files from argv deterministically).
  var BOARD_DIR = "/home/kicad/demo";
  var BOARD_FILES = [
    "tinytapeout-demo-Edge_Cuts.gbr",
    "tinytapeout-demo-B_Cu.gbr",
    "tinytapeout-demo-In2_Cu.gbr",
    "tinytapeout-demo-In1_Cu.gbr",
    "tinytapeout-demo-F_Cu.gbr",
    "tinytapeout-demo-B_Mask.gbr",
    "tinytapeout-demo-F_Mask.gbr",
    "tinytapeout-demo-B_Paste.gbr",
    "tinytapeout-demo-F_Paste.gbr",
    "tinytapeout-demo-B_Silkscreen.gbr",
    "tinytapeout-demo-F_Silkscreen.gbr",
    "tinytapeout-demo-User_2.gbr",
    "tinytapeout-demo-PTH.drl",
    "tinytapeout-demo-NPTH.drl",
    "tinytapeout-demo-job.gbrjob",
  ];
  var OPEN_FILES = BOARD_FILES.filter(function (f) {
    return /\.(gbr|drl)$/.test(f);
  });
  var OPEN_ARGS = OPEN_FILES.map(function (f) {
    return BOARD_DIR + "/" + f;
  });

  // ── Status UI ──────────────────────────────────────────────────────────────
  var statusText = document.getElementById("status-text");
  var progressBar = document.getElementById("progress-bar");
  function setStatusUI(text) {
    if (statusText) statusText.textContent = text;
    var m = /(\d+)\/(\d+)/.exec(text || "");
    if (m && progressBar) {
      progressBar.style.width =
        (parseInt(m[1], 10) / parseInt(m[2], 10)) * 100 + "%";
    }
  }
  function hideStatus() {
    var s = document.getElementById("status");
    if (s) s.style.display = "none";
  }
  function showError(msg) {
    console.error("[KICAD_ERROR] " + msg);
    if (statusText) {
      statusText.textContent = "Error: " + msg;
      statusText.style.color = "#ff6b6b";
    }
  }

  // The WASM reads the top frame geometry from a GLOBAL `mainWindow` element.
  var mainWindow = document.getElementById("main-window");
  window.mainWindow = mainWindow;

  // ── Prefetch the heavy, non-wasm assets in parallel with the wasm download ──
  // images.tar.gz (compiled-in KiCad resources) and the board layers. Both are
  // ~10x smaller than gerbview.wasm, so they land before preRun runs; a run
  // dependency guards the rare case where the wasm wins the race.
  var resourceData = null;
  // Served without a .gz extension on purpose (see sync-demo-wasm.sh) so no
  // server adds Content-Encoding: gzip. Written into MEMFS as images.tar.gz.
  fetch(BINARY_BASE + "/kicad-resources.bin")
    .then(function (r) {
      if (!r.ok) throw new Error("HTTP " + r.status);
      return r.arrayBuffer();
    })
    .then(function (buf) {
      resourceData = new Uint8Array(buf);
      console.log("[KICAD] prefetched images.tar.gz (" + resourceData.length + " bytes)");
    })
    .catch(function (err) {
      console.warn("[KICAD] images.tar.gz prefetch failed:", err.message);
    });

  var boardData = null; // { name: Uint8Array }
  var boardPromise = Promise.all(
    BOARD_FILES.map(function (name) {
      return fetch("./board/" + name)
        .then(function (r) {
          if (!r.ok) throw new Error("HTTP " + r.status + " for " + name);
          return r.arrayBuffer();
        })
        .then(function (buf) {
          return [name, new Uint8Array(buf)];
        });
    })
  )
    .then(function (pairs) {
      boardData = {};
      pairs.forEach(function (p) {
        boardData[p[0]] = p[1];
      });
      console.log("[KICAD] prefetched " + pairs.length + " board files");
    })
    .catch(function (err) {
      console.error("[KICAD] board prefetch failed:", err.message);
    });

  // ── preRun steps ────────────────────────────────────────────────────────────
  function createCanvas() {
    var canvas = document.createElement("canvas");
    canvas.id = "canvas";
    canvas.style.display = "none";
    canvas.style.width = window.innerWidth + "px";
    canvas.style.height = window.innerHeight + "px";
    canvas.oncontextmenu = function (e) {
      e.preventDefault();
    };
    canvas.addEventListener(
      "webglcontextlost",
      function (e) {
        showError("WebGL context lost — reload to retry.");
        e.preventDefault();
      },
      false
    );
    mainWindow.appendChild(canvas);
    Module.canvas = canvas;
    console.log("[KICAD] canvas " + window.innerWidth + "x" + window.innerHeight);
  }

  function writeResources() {
    FS.mkdirTree(RESOURCE_PATH);
    if (resourceData) {
      FS.writeFile(RESOURCE_PATH + "/images.tar.gz", resourceData);
      console.log("[KICAD] wrote images.tar.gz");
    } else {
      console.warn("[KICAD] images.tar.gz not ready at preRun");
    }
  }

  // Suppress the first-run setup wizard (its modal loop crashes Asyncify on our
  // ephemeral MEMFS): make every settings provider report NeedsUserInput()==false.
  function seedKicadConfig() {
    FS.mkdirTree(KICAD_CONFIG_DIR);
    var seed = function (path, contents) {
      if (FS.analyzePath(path).exists) return;
      FS.writeFile(path, contents);
    };
    seed(
      KICAD_CONFIG_DIR + "/kicad_common.json",
      JSON.stringify(
        {
          do_not_show_again: {
            update_check_prompt: true,
            data_collection_prompt: true,
          },
        },
        null,
        2
      )
    );
    seed(KICAD_CONFIG_DIR + "/sym-lib-table", "(sym_lib_table\n  (version 7)\n)\n");
    seed(KICAD_CONFIG_DIR + "/fp-lib-table", "(fp_lib_table\n  (version 7)\n)\n");
    seed(
      KICAD_CONFIG_DIR + "/design-block-lib-table",
      "(design_block_lib_table\n  (version 7)\n)\n"
    );
    console.log("[KICAD] seeded config (wizard suppressed)");
  }

  // Write the board into MEMFS and point argv at it so gerbview auto-opens it.
  // argv must be set before main(); a run dependency keeps main() waiting if the
  // board fetch hasn't landed yet.
  function preloadBoard() {
    Module.arguments = OPEN_ARGS;
    var writeBoard = function () {
      if (!boardData) return;
      FS.mkdirTree(BOARD_DIR);
      Object.keys(boardData).forEach(function (name) {
        FS.writeFile(BOARD_DIR + "/" + name, boardData[name]);
      });
      console.log("[KICAD] wrote board into " + BOARD_DIR + "; argv=" + OPEN_ARGS.length + " files");
    };
    if (boardData) {
      writeBoard();
      return;
    }
    var add = window.addRunDependency;
    var rm = window.removeRunDependency;
    if (typeof add === "function" && typeof rm === "function") {
      add("board-files");
      boardPromise.then(function () {
        writeBoard();
        rm("board-files");
      });
    } else {
      // Best-effort fallback (board << wasm, so this path is unlikely to lose).
      boardPromise.then(writeBoard);
    }
  }

  // ── Module config (global) ───────────────────────────────────────────────────
  var Module = {
    thisProgram: "/usr/bin/gerbview", // argv[0] for KiCad's DEBUG check
    arguments: OPEN_ARGS,
    preRun: [createCanvas, writeResources, seedKicadConfig, preloadBoard],
    postRun: [],
    print: function () {
      console.log("[KICAD_OUT] " + Array.prototype.join.call(arguments, " "));
    },
    printErr: function () {
      console.error("[KICAD_ERR] " + Array.prototype.join.call(arguments, " "));
    },
    setStatus: function (text) {
      if (text) console.log("[KICAD_STATUS] " + text);
      setStatusUI(text);
    },
    totalDependencies: 0,
    monitorRunDependencies: function (left) {
      this.totalDependencies = Math.max(this.totalDependencies, left);
      Module.setStatus(
        left
          ? "Preparing… (" + (this.totalDependencies - left) + "/" + this.totalDependencies + ")"
          : "All downloads complete."
      );
    },
    onRuntimeInitialized: function () {
      console.log("[KICAD] runtime initialized");
      if (Module.canvas) Module.canvas.style.display = "block";
      hideStatus();
    },
    onAbort: function (what) {
      showError("aborted: " + (what === undefined ? "" : String(what)));
    },
    // Route the .wasm to BINARY_BASE; everything else (the pthread worker) to GLUE_BASE.
    locateFile: function (path) {
      return (/\.wasm$/.test(path) ? BINARY_BASE : GLUE_BASE) + "/" + path;
    },
    // Pin the pthread worker to the same-origin glue (workers cannot be cross-origin).
    mainScriptUrlOrBlob: GLUE_BASE + "/gerbview.js",
  };
  window.Module = Module;

  Module.setStatus("Downloading…");
  window.onerror = function (msg, url, line) {
    showError(msg + " @ " + url + ":" + line);
    return false;
  };

  // ── Inject glue scripts in the required order ────────────────────────────────
  function loadScript(src) {
    return new Promise(function (resolve, reject) {
      var s = document.createElement("script");
      s.src = src;
      s.onload = resolve;
      s.onerror = function () {
        reject(new Error("failed to load " + src));
      };
      document.body.appendChild(s);
    });
  }

  loadScript(GLUE_BASE + "/wx.js")
    .then(function () {
      return loadScript(GLUE_BASE + "/wx-dom.js");
    })
    .then(function () {
      return loadScript(GLUE_BASE + "/gerbview.js");
    })
    .then(function () {
      console.log("[KICAD] injected wx.js + wx-dom.js + gerbview.js (glue=" + GLUE_BASE + ", bin=" + BINARY_BASE + ")");
    })
    .catch(function (err) {
      showError(err.message);
    });
})();
