// Renders the results page from window.SITE_DATA (Site/data.js, made by Scripts/build-site.py).
(function () {
  "use strict";
  var D = window.SITE_DATA;
  if (!D) { return; }
  var BUDGET = D.meta.target_ms;
  var BOT = D.bottleneck_names;
  var METRICS = [
    { key: "frame_p50_ms", label: "프레임 p50", short: "프레임 p50", color: "var(--frame)" },
    { key: "frame_p95_ms", label: "프레임 p95", short: "프레임 p95", color: "var(--frame)" },
    { key: "game_p50_ms", label: "게임 스레드 시간", short: "게임 스레드", color: "var(--game)", bot: "game" },
    { key: "render_p50_ms", label: "렌더 스레드 시간", short: "렌더 스레드", color: "var(--render)", bot: "render" },
    { key: "gpu_p50_ms", label: "GPU 시간", short: "GPU", color: "var(--gpu)", bot: "gpu" }
  ];

  // ---------- formatting (same rule as Scripts/plot-readme.py fmt) ----------
  function fmt(x, signed) {
    var a = Math.abs(x) + 1e-9;  // round half up like Python's Decimal in plot-readme.py (toFixed alone gives 1.85 for 1.855)
    var s = a >= 100 ? a.toFixed(0) : a >= 10 ? a.toFixed(1) : a.toFixed(2);
    if (parseFloat(s) === 0) { return s; }
    if (signed) { return (x > 0 ? "+" : x < 0 ? "−" : "±") + s; }
    return (x < 0 ? "−" : "") + s;
  }
  function pct(p) { return (p > 0 ? "+" : p < 0 ? "−" : "±") + Math.abs(p).toFixed(1) + "%"; }
  function int(n) { return Math.round(n).toLocaleString("en-US"); }
  function kcount(n) {
    var man = Math.floor(n / 10000), r = n % 10000, cheon = Math.floor(r / 1000), h = r % 1000, out = [], t = "";
    if (man) { out.push(man + "만"); }
    if (cheon) { t += cheon + "천"; }
    if (h) { t += cheon ? (h / 100) + "백" : String(h); }
    if (t) { out.push(t); }
    return out.join(" ");
  }
  // Two significant figures: 781 -> 780, 4102 -> 4,100 (the README writes the max N this way).
  function approx(n) {
    var p = Math.pow(10, Math.floor(Math.log10(n)) - 1);
    return Math.round(n / p) * p;
  }
  function esc(t) {
    return String(t).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
  }
  function md(t) { return esc(t).replace(/`([^`]+)`/g, "<code>$1</code>"); }
  function $(id) { return document.getElementById(id); }
  function stepName(s) { return "단계 " + s.num + " " + s.name; }

  // ---------- state ----------
  var steps = D.steps;
  var state = { step: steps[0].num, n: D.meta.target_n };
  var userMotionOk = false;
  var reduceMotion = window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  function stepBy(num) { return steps[num - 1]; }

  function readHash() {
    var h = location.hash.replace(/^#/, "");
    var out = {};
    h.split("&").forEach(function (kv) {
      var p = kv.split("=");
      if (p.length === 2) { out[p[0]] = parseInt(p[1], 10); }
    });
    if (out.step >= 1 && out.step <= steps.length) { state.step = out.step; }
    if (out.n && D.counts.indexOf(out.n) >= 0) { state.n = out.n; }
    if (stepBy(state.step).counts.indexOf(state.n) < 0) { state.n = D.meta.target_n; }
  }
  function writeHash() {
    var h = "#step=" + state.step + (state.n !== D.meta.target_n ? "&n=" + state.n : "");
    if (location.hash !== h && history.replaceState) { history.replaceState(null, "", h); }
  }

  // ---------- header ----------
  function renderHeader() {
    var H = D.headline;
    $("lead").innerHTML = md("`ACharacter` 액터로 호드 1만을 돌리면 프레임 p50이 " + fmt(H.first_ms) + "ms이고, 60fps를 지키는 액터는 약 " +
      int(approx(H.first_max60)) + "개까지다. 같은 호드를 UE 5.8의 Mass Entity로 다시 만들고, 최적화 기법을 한 단계씩 더하며 " +
      "그때마다 같은 시나리오를 같은 방법으로 다시 쟀다.");
    $("spec-n").textContent = kcount(D.meta.target_n);
    $("spec-ms").textContent = "한 프레임 " + fmt(BUDGET) + "ms";
    $("res-first").textContent = fmt(H.first_ms) + "ms";
    $("res-final").textContent = fmt(H.final_ms) + "ms";
    $("res-note").textContent = "단계 1의 프레임 p50은 " + fmt(BUDGET) + "ms의 " + H.first_over_budget + "배다. 단계 8의 프레임 p95는 " +
      fmt(H.final_p95_ms) + "ms이고, " + fmt(BUDGET) + "ms까지의 여유는 " + fmt(H.final_margin_ms) + "ms다.";
    ["link-repo", "foot-repo"].forEach(function (id) { $(id).href = D.meta.repo; });
    ["link-release", "foot-release"].forEach(function (id) { $(id).href = D.meta.release; });
    $("foot-csv").href = D.meta.summary_csv;
  }

  // ---------- tabs ----------
  function renderTabs() {
    var html = steps.map(function (s) {
      return '<button type="button" role="tab" class="tab' + (s.group === "quality" ? " quality" : "") + '" id="tab-' + s.num +
        '" aria-controls="panel" aria-selected="false" tabindex="-1" data-step="' + s.num + '">' +
        '<span class="t-num">단계 ' + s.num + '</span><span class="t-name">' + esc(s.name) + '</span></button>';
    }).join("");
    var tabs = $("tabs");
    tabs.innerHTML = html;
    tabs.addEventListener("click", function (e) {
      var b = e.target.closest("[data-step]");
      if (b) { setStep(+b.getAttribute("data-step"), true); }
    });
    tabs.addEventListener("keydown", function (e) {
      var k = e.key, to = null;
      if (k === "ArrowRight" || k === "ArrowDown") { to = Math.min(state.step + 1, steps.length); }
      else if (k === "ArrowLeft" || k === "ArrowUp") { to = Math.max(state.step - 1, 1); }
      else if (k === "Home") { to = 1; }
      else if (k === "End") { to = steps.length; }
      if (to !== null) {
        e.preventDefault();
        setStep(to, true);
        $("tab-" + to).focus();
      }
    });
  }

  // ---------- panel ----------
  function setStep(num, user) {
    if (num < 1 || num > steps.length) { return; }
    state.step = num;
    if (stepBy(num).counts.indexOf(state.n) < 0) { state.n = D.meta.target_n; }
    renderPanel(user);
  }
  function setN(n) {
    state.n = n;
    renderPanel(true);
  }

  function loadClip(s) {
    var img = $("clip"), msg = $("clip-msg"), btn = $("clip-load");
    img.alt = s.alt;
    var size = " (" + s.clip_mb.toFixed(1) + "MB)";
    if (reduceMotion && !userMotionOk) {
      img.removeAttribute("src");
      img.classList.remove("loading");
      msg.textContent = "";
      btn.hidden = false;
      btn.textContent = "움직이는 화면 보기" + size;
      return;
    }
    btn.hidden = true;
    if (img.getAttribute("src") === s.clip) { return; }
    img.classList.add("loading");
    msg.textContent = "화면을 불러오는 중" + size;
    var token = s.clip;
    img.onload = function () {
      if (img.getAttribute("src") !== token) { return; }
      img.classList.remove("loading");
      msg.textContent = "";
    };
    img.onerror = function () {
      if (img.getAttribute("src") !== token) { return; }
      msg.textContent = "화면을 불러오지 못했다.";
    };
    img.setAttribute("src", s.clip);
  }

  function renderPanel(user) {
    var s = stepBy(state.step);
    var n = state.n;
    var row = s.by_n[String(n)];
    writeHash();

    steps.forEach(function (t) {
      var b = $("tab-" + t.num), on = t.num === s.num;
      b.setAttribute("aria-selected", on ? "true" : "false");
      b.tabIndex = on ? 0 : -1;
    });
    $("panel").setAttribute("aria-labelledby", "tab-" + s.num);
    $("prev").disabled = s.num === 1;
    $("next").disabled = s.num === steps.length;

    loadClip(s);
    $("clip-cap").textContent = "호드 총수 1만, 이동 장면, 오브 시점 카메라." + (n !== D.meta.target_n ? " 화면은 고른 호드 총수와 상관없이 1만이다." : "") +
      " 화면 왼쪽 위 HUD의 숫자는 측정값이 아니다.";

    $("step-title").textContent = stepName(s);
    var g = $("step-group");
    g.textContent = D.groups[s.group];
    g.className = "tag" + (s.group === "quality" ? " quality" : "");
    $("step-changed").innerHTML = md(s.changed);
    $("step-learned").innerHTML = md(s.learned);
    var a = $("step-post");
    a.href = s.post.url;
    a.textContent = "이 단계의 글 읽기: " + String(s.post.num).padStart(2, "0") + " " + s.post.title;

    // horde size selector
    $("nsel").innerHTML = D.counts.map(function (c) {
      var ok = s.counts.indexOf(c) >= 0;
      return '<button type="button" data-n="' + c + '" aria-pressed="' + (c === n) + '"' +
        (ok ? "" : ' disabled title="이 단계는 이 호드 총수를 재지 않았다"') + ">" + kcount(c) + "</button>";
    }).join("");

    // metric cards
    $("metrics").innerHTML = METRICS.map(function (m, i) {
      var v = row[m.key], isBot = m.bot && m.bot === row.bottleneck;
      return '<div class="metric' + (i === 0 ? " first" : "") + (isBot ? " is-bot" : "") + '">' +
        '<span class="m-label"><span class="sw" style="background:' + m.color + '"></span>' + m.label +
        (isBot ? '<span class="badge">병목</span>' : "") + "</span>" +
        '<span class="m-val num">' + fmt(v) + "<small>ms</small></span>" +
        '<span class="m-range num">세 번 잰 범위: ' + fmt(row[m.key + "_min"]) + "–" + fmt(row[m.key + "_max"]) + "</span></div>";
    }).join("");
    $("unmet").textContent = row.unmet ? "이 호드 총수에서는 이 단계가 다른 단계와 같은 일을 하지 못했다(기능 미달). 값은 참고이고, 최대 N 셈에서 뺀다." : "";

    renderBudget(row);
    renderDelta(s, n);
    renderMargin(s);

    highlightCharts();
    if (user) {
      $("announce").textContent = stepName(s) + ", 호드 총수 " + kcount(n) + ": 프레임 p50 " + fmt(row.frame_p50_ms) +
        "ms, 프레임 p95 " + fmt(row.frame_p95_ms) + "ms, 병목 " + BOT[row.bottleneck];
    }
  }

  // Steps 1 and 2 give the max N; steps 3 to 8 give the margin to 16.7 ms at horde 10000 (W14 decision 5).
  function renderMargin(s) {
    var r = s.by_n[String(D.meta.target_n)];
    var h = $("margin-h"), v = $("max60"), note = $("max60-note");
    if (s.max60) {
      h.textContent = "60fps를 지키는 최대 개체 수(최대 N)";
      v.textContent = "약 " + int(approx(s.max60.n)) + "개";
      note.textContent = "프레임 p95가 " + fmt(BUDGET) + "ms를 넘기 직전의 호드 총수다. 잰 호드 총수 사이를 로그 보간했다. 호드 총수 1만에서는 프레임 p95가 " +
        fmt(r.frame_p95_ms) + "ms라 " + fmt(BUDGET) + "ms를 넘는다.";
    } else {
      h.textContent = fmt(BUDGET) + "ms까지의 여유";
      v.textContent = s.margin_ms === null ? fmt(BUDGET) + "ms를 넘는다" : fmt(s.margin_ms) + "ms";
      note.textContent = "호드 총수 1만의 프레임 p95 " + fmt(r.frame_p95_ms) + "ms를 " + fmt(BUDGET) + "ms에서 뺀 값이다. 고른 호드 총수와 상관없이 1만 기준이다.";
    }
  }

  function renderBudget(row) {
    var MAX = 20;
    var x = function (v) { return Math.min(v, MAX) / MAX * 100; };
    var lineX = x(BUDGET);
    var html = METRICS.map(function (m) {
      var v = row[m.key], over = v > MAX, isBot = m.bot && m.bot === row.bottleneck;
      return '<div class="b-row"><span class="b-label">' + m.short + (isBot ? '<span class="badge">병목</span>' : "") + "</span>" +
        '<div class="b-track"><div class="b-bar' + (over ? " over" : "") + '" style="width:' + x(v).toFixed(2) + "%;background:" + m.color + '"></div>' +
        '<div class="b-line" style="left:' + lineX.toFixed(2) + '%"></div></div>' +
        '<span class="b-val num">' + fmt(v) + "ms</span></div>";
    }).join("");
    var anyOver = METRICS.some(function (m) { return row[m.key] > MAX; });
    html += '<div class="b-axis"><span></span><div>' + [0, 5, 10, 15, 20].map(function (t) {
      return '<span style="left:' + x(t) + '%">' + t + "</span>";
    }).join("") + "</div><span></span></div>";
    html = '<div class="b-row" style="margin:0"><span></span><div class="b-track" style="height:16px"><span class="b-line-label" style="left:' +
      lineX.toFixed(2) + '%">' + fmt(BUDGET) + "ms</span></div><span></span></div>" + html;
    if (anyOver) { html += '<p class="muted small">끝이 끊긴 막대는 20ms 축척 밖이다.</p>'; }
    $("budget").innerHTML = html;
  }

  function renderDelta(s, n) {
    var box = $("delta");
    if (s.num === 1) {
      $("delta-h").textContent = "앞 단계 대비";
      box.innerHTML = '<p class="muted">첫 단계라 견줄 앞 단계가 없다.</p>';
      return;
    }
    var prev = stepBy(s.num - 1);
    $("delta-h").textContent = "앞 단계(" + stepName(prev) + ") 대비";
    var d = s.delta[String(n)];
    if (!d) {
      box.innerHTML = '<p class="muted">앞 단계는 호드 총수 ' + kcount(n) + "를 재지 않았다.</p>";
      return;
    }
    var row = s.by_n[String(n)];
    var line = function (label, x) {
      var v = fmt(x.d, true) + "ms";
      var tail = x.real ? " (" + pct(x.pct) + ")" : ' <span class="same">· 구별되지 않음</span>';
      return "<li><span>" + label + '</span><span class="d-val num">' + v + tail + "</span></li>";
    };
    var bot = d.bottleneck_prev === row.bottleneck ? BOT[row.bottleneck] + ' <span class="same">· 그대로</span>'
      : BOT[d.bottleneck_prev] + " → " + BOT[row.bottleneck];
    box.innerHTML = '<ul class="delta">' + line("프레임 p50", d.frame) + line("프레임 p95", d.frame95) + line("게임 스레드 시간", d.game) + line("GPU 시간", d.gpu) +
      '<li><span>병목</span><span class="d-val">' + bot + "</span></li></ul>";
  }

  // ---------- ladder chart ----------
  function renderLadder() {
    var MAX = 18;
    var x = function (v) { return Math.min(v, MAX) / MAX * 100; };
    var html = "";
    steps.forEach(function (s, i) {
      var r = s.by_n[String(D.meta.target_n)];
      var cut = r.frame_p50_ms > MAX;
      var sp = s.group === "quality" ? " quality" : "";
      var bars = cut ? '<div class="c-bar cut' + sp + '" style="width:100%"></div>'
        : '<div class="c-bar p95' + sp + '" style="width:' + x(r.frame_p95_ms).toFixed(2) + '%"></div>' +
          '<div class="c-bar' + sp + '" style="width:' + x(r.frame_p50_ms).toFixed(2) + '%"></div>';
      var sub = (cut ? "축척 밖 · " + (r.frame_p50_ms / BUDGET).toFixed(1) + "배<br>" : "") + "프레임 p95 " + fmt(r.frame_p95_ms) + "<br>병목 " + BOT[r.bottleneck];
      html += '<button type="button" class="c-row" data-step="' + s.num + '" aria-label="' + esc(stepName(s)) + " 보기: 프레임 p50 " + fmt(r.frame_p50_ms) + "ms, 프레임 p95 " + fmt(r.frame_p95_ms) + 'ms">' +
        '<span class="c-label"><b>' + s.num + "</b>" + esc(s.name) + "</span>" +
        '<span class="c-track">' + bars + '<span class="c-line" style="left:' + x(BUDGET).toFixed(2) + '%"></span></span>' +
        '<span class="c-val num">' + fmt(r.frame_p50_ms) + "ms<small>" + sub + "</small></span></button>";
    });
    html += '<div class="c-foot"><span></span><div>' + [0, 5, 10, 15].map(function (t) {
      return '<span style="left:' + x(t) + '%">' + t + "</span>";
    }).join("") + '<span class="target" style="left:' + x(BUDGET) + '%;top:14px">' + fmt(BUDGET) + "ms</span></div><span></span></div>";
    html += '<p class="c-key"><span><i></i>프레임 p50</span><span><i class="k-p95"></i>프레임 p95</span>' +
      '<span><i class="k-line"></i>' + fmt(BUDGET) + "ms</span><span><i class=\"k-quality\"></i>빗금: " + D.groups.quality + "</span></p>";
    var el = $("ladder");
    el.innerHTML = html;
    el.addEventListener("click", function (e) {
      var b = e.target.closest("[data-step]");
      if (b) {
        setStep(+b.getAttribute("data-step"), true);
        $("steps").scrollIntoView({ behavior: reduceMotion ? "auto" : "smooth", block: "start" });
      }
    });
  }

  // Steps 3 to 8: bar = 16.7 ms minus the frame p95 at horde 10000. Steps 1 and 2 exceed 16.7 ms there, so they show the max N instead.
  function renderMarginChart() {
    var MAX = 3;
    var x = function (v) { return v / MAX * 100; };
    var html = "";
    steps.forEach(function (s) {
      var r = s.by_n[String(D.meta.target_n)], sp = s.group === "quality" ? " quality" : "", track, val;
      if (s.max60) {
        track = '<span class="c-none">' + fmt(BUDGET) + 'ms를 넘음</span>';
        val = "최대 N 약 " + int(approx(s.max60.n)) + "<small>프레임 p95 " + fmt(r.frame_p95_ms) + "ms</small>";
      } else {
        track = '<div class="c-bar' + sp + '" style="width:' + x(s.margin_ms).toFixed(2) + '%"></div>';
        val = fmt(s.margin_ms) + "ms<small>프레임 p95 " + fmt(r.frame_p95_ms) + "ms</small>";
      }
      html += '<div class="c-row" data-row="' + s.num + '"><span class="c-label"><b>' + s.num + "</b>" + esc(s.name) + "</span>" +
        '<span class="c-track">' + track + '</span><span class="c-val num">' + val + "</span></div>";
    });
    html += '<div class="c-foot"><span></span><div>' + [0, 1, 2, 3].map(function (t) {
      return '<span style="left:' + x(t) + '%">' + t + (t ? "ms" : "") + "</span>";
    }).join("") + '</div><span></span></div>';
    html += '<p class="c-key"><span><i class="k-quality"></i>빗금: ' + D.groups.quality + "</span></p>";
    $("margin").innerHTML = html;
  }

  function highlightCharts() {
    Array.prototype.forEach.call(document.querySelectorAll("#ladder .c-row, #margin .c-row"), function (r) {
      var on = +(r.getAttribute("data-step") || r.getAttribute("data-row")) === state.step;
      r.classList.toggle("sel", on);
      if (r.tagName === "BUTTON") { r.setAttribute("aria-current", on ? "true" : "false"); }
    });
  }

  // ---------- ablation ----------
  function renderAblation() {
    var A = D.ablation;
    $("abl-intro").textContent = "호드 총수 1만, 이동 장면, 오브 시점 카메라, 세 번 잰 중앙값. 기법별 기여는 단계 8 최종 구성에서 기법 하나만 끄고 같은 조건으로 잰 차이다. " +
      "끄면 프레임이 길어지는 기법은 시간을 줄이고, 짧아지는 기법은 시간을 쓴다. 0은 최종 구성(프레임 p95 " +
      fmt(A.base.frame_p95_ms) + "ms, 게임 스레드 시간 " + fmt(A.base.game_p50_ms) + "ms, GPU 시간 " + fmt(A.base.gpu_p50_ms) + "ms)이다. 최종 구성의 병목은 " +
      BOT[A.base_bottleneck] + "라, GPU 시간만 달라지는 기법은 프레임 p95에 드러나지 않는다. 구별되는 조건이 위, 구별되지 않는 조건이 가운데, 하위 실험이 아래다. 각 부분 안에서는 프레임 p95가 많이 길어지는 순이다.";
    var maxAbs = 0;
    A.rows.forEach(function (r) { ["frame", "game", "gpu"].forEach(function (k) { maxAbs = Math.max(maxAbs, Math.abs(r[k].d)); }); });
    var half = Math.ceil(maxAbs);
    var mets = [["frame", "프레임 p95", "var(--frame)"], ["game", "게임 스레드", "var(--game)"], ["gpu", "GPU", "var(--gpu)"]];
    var html = '<div class="a-head"><span></span><div><span></span><span class="dir"><span>← 빨라짐</span><span>느려짐 →</span></span><span></span></div></div>';
    A.rows.forEach(function (r) {
      var tag = r.bottleneck !== A.base_bottleneck ? '<span class="tagline">병목이 ' + BOT[r.bottleneck] + "로 바뀜</span>" : "";
      var notes = (r.notes || []).filter(function (t) { return t.indexOf("병목이") !== 0; });  // the bottleneck change has its own tag line
      var extra = (r.mixed ? '<small class="mixed">참고 값. ' + md(r.mixed) + "</small>" : "") +
        (notes.length ? "<small>" + esc(notes.join(", ")) + "</small>" : "");
      html += '<div class="a-row"><div class="a-name">' + esc(r.name) + (r.sub ? "<small>" + md(r.sub) + "</small>" : "") + extra + tag + '</div><div class="a-bars">';
      mets.forEach(function (m) {
        var v = r[m[0]], w = Math.abs(v.d) / half * 50;
        var left = v.d >= 0 ? 50 : 50 - w;
        html += '<div class="a-line"><span class="a-m">' + m[1] + '</span><span class="a-track"><span class="a-bar' + (v.real ? "" : " faint") +
          '" style="left:' + left.toFixed(2) + "%;width:" + Math.max(w, 0.4).toFixed(2) + "%;background:" + m[2] + '"></span></span>' +
          '<span class="a-v num' + (v.real ? " real" : "") + '">' + fmt(v.d, true) + (v.real ? "" : ' <span class="same">구별되지 않음</span>') + "</span></div>";
      });
      html += "</div></div>";
    });
    html += '<p class="c-key">단위 ms, 세 번 잰 중앙값의 차이. 흐린 막대와 "구별되지 않음": 차이가 두 조건의 범위 중 큰 쪽보다 작거나 0.5ms 아래다. 가로 끝은 ±' + half + "ms다.</p>";
    $("ablation").innerHTML = html;
    $("abl-notes").innerHTML = A.notes.map(function (t) { return "<li>" + md(t) + "</li>"; }).join("");
  }

  // ---------- entity pool ----------
  var POOL = [
    { key: "iskm_tracks_end", title: "ISKM 트랙 수", unit: "", tick: function (v) { return v ? (v / 10000) + "만" : "0"; }, end: function (v) { return int(v); }, zero: true },
    { key: "gpu_mem_mean", title: "GPU 메모리(MB)", unit: "MB", tick: function (v) { return int(v); }, end: function (v) { return int(v); } },
    { key: "frame_p95", title: "프레임 p95(ms)", unit: "ms", tick: function (v) { return String(v); }, end: function (v) { return fmt(v); } }
  ];
  function niceStep(span) {
    var raw = span / 3, p = Math.pow(10, Math.floor(Math.log10(raw)));
    var c = [1, 2, 2.5, 5, 10].map(function (m) { return m * p; }).filter(function (s) { return s >= raw; });
    return c[0];
  }
  function renderPool() {
    var P = D.pool;
    $("pool-notes").innerHTML = P.notes.map(function (t) { return "<li>" + md(t) + "</li>"; }).join("");
    var host = $("pool");
    var width = Math.max(260, Math.floor((host.clientWidth - 36) / (host.clientWidth > 900 ? 3 : 1)) - (host.clientWidth > 900 ? 12 : 0));
    var H = 170, L = 46, R = 64, T = 10, B = 26;
    var mins = P.minutes, m0 = mins[0], m1 = mins[mins.length - 1];
    var html = '<p class="c-key" style="margin:0 0 12px"><span><svg width="26" height="10" aria-hidden="true"><line x1="0" y1="5" x2="26" y2="5" style="stroke:var(--game);stroke-width:3"/></svg> 풀 켬(최종 구성)</span>' +
      '<span><svg width="26" height="10" aria-hidden="true"><line x1="0" y1="5" x2="26" y2="5" style="stroke:var(--gpu);stroke-width:3;stroke-dasharray:6 4"/></svg> 풀 끔</span>' +
      "<span>가로: 실행 시작부터 분, 1분 구간마다의 값. 호드 총수 1만, 이동 장면, 오브 시점 카메라, 조건마다 한 번</span></p><div class=\"pool-grid\">";
    POOL.forEach(function (c) {
      var vals = P.on[c.key].concat(P.off[c.key]);
      var lo = c.zero ? 0 : Math.min.apply(null, vals), hi = Math.max.apply(null, vals);
      var st = niceStep(hi - lo);
      lo = Math.floor(lo / st) * st; hi = Math.ceil(hi / st) * st;
      var sx = function (m) { return L + (m - m0) / (m1 - m0) * (width - L - R); };
      var sy = function (v) { return T + (hi - v) / (hi - lo) * (H - T - B); };
      var svg = '<svg viewBox="0 0 ' + width + " " + H + '" width="' + width + '" height="' + H + '" role="img" aria-label="' + esc(c.title) + ": 풀 켬 " +
        c.end(P.on[c.key][0]) + "에서 " + c.end(P.on[c.key][mins.length - 1]) + ", 풀 끔 " + c.end(P.off[c.key][0]) + "에서 " + c.end(P.off[c.key][mins.length - 1]) + '">';
      for (var t = lo; t <= hi + 1e-9; t += st) {
        var y = sy(t).toFixed(1);
        svg += '<line x1="' + L + '" x2="' + (width - R) + '" y1="' + y + '" y2="' + y + '" style="stroke:var(--line)"/>' +
          '<text x="' + (L - 6) + '" y="' + (+y + 4) + '" text-anchor="end" font-size="12" style="fill:var(--sub)">' + c.tick(Math.round(t * 100) / 100) + "</text>";
      }
      [m0, 4, 8, 12, m1].forEach(function (m) {
        svg += '<text x="' + sx(m).toFixed(1) + '" y="' + (H - 6) + '" text-anchor="middle" font-size="12" style="fill:var(--sub)">' + m + "분</text>";
      });
      [["on", "var(--game)", ""], ["off", "var(--gpu)", "stroke-dasharray:6 4;"]].forEach(function (k) {
        var arr = P[k[0]][c.key];
        var pts = arr.map(function (v, i) { return sx(mins[i]).toFixed(1) + "," + sy(v).toFixed(1); }).join(" ");
        svg += '<polyline points="' + pts + '" fill="none" style="stroke:' + k[1] + ";stroke-width:2.5;" + k[2] + '"/>';
        var ly = sy(arr[arr.length - 1]);
        svg += '<circle cx="' + sx(m1).toFixed(1) + '" cy="' + ly.toFixed(1) + '" r="3.5" style="fill:' + k[1] + '"/>' +
          '<text x="' + (sx(m1) + 7).toFixed(1) + '" y="' + (ly + 4).toFixed(1) + '" font-size="12.5" font-weight="700" style="fill:' + k[1] + '">' + c.end(arr[arr.length - 1]) + "</text>";
      });
      svg += "</svg>";
      html += "<div><h4>" + esc(c.title) + "</h4>" + svg + "</div>";
    });
    html += "</div>";
    host.innerHTML = html;
  }

  // ---------- method and posts ----------
  function renderMethod() {
    var M = D.meta;
    var runs = { 1: "한", 2: "두", 3: "세", 4: "네", 5: "다섯" }[M.runs_per_condition] || M.runs_per_condition;
    var items = [
      "Test 구성 패키지 하나(" + M.build + ")에서 여덟 단계와 기법별 기여 조건을 연달아 쟀다. 이 페이지의 값은 모두 이 측정에서 나왔다.",
      "화면은 " + M.res + " 창 모드, 스케일러빌리티 Epic이다. 수직 동기화, 프레임 상한, 동적 해상도는 껐다.",
      "시나리오는 반경 500m 평지다. 오브가 맵 중심 둘레의 반지름 57.3m 원을 2m/s로 돌고, 개체가 사방에서 3m/s로 오브를 쫓는다. 오브는 8m 안의 개체에 피해를 주고 2초마다 화면 안의 개체 5명을 저격한다. 죽은 개체는 맵 밖 무리의 기록 자리에서 다시 생겨 호드 총수가 그대로다.",
      "카메라는 오브 뒤 15m에서 오브를 따라가며 1분에 한 바퀴 도는 오브 시점이다. 고정 시드로 개체를 미리 놓고, 워밍업 60초 뒤 180초를 잰다.",
      "궤적이 비결정적이라 같은 조건을 " + runs + " 번 재고, 중앙값과 범위를 함께 적는다. 중앙값의 차이가 두 조건 범위 가운데 큰 쪽보다 작거나 " + fmt(M.distinct_floor_ms) + "ms 아래면 \"구별되지 않는다\"고 적는다.",
      "프레임 p50과 프레임 p95는 측정 창에서 프레임 시간의 50번째, 95번째 백분위수다. 프레임 시간은 게임 스레드, 렌더 스레드, GPU 가운데 가장 느린 쪽(병목)이 정한다.",
      "판정은 프레임 p95와 " + fmt(M.target_ms) + "ms로 한다. " + fmt(M.target_ms) + "ms까지의 여유는 " + fmt(M.target_ms) + "ms에서 프레임 p95를 뺀 값이다. 최대 N은 프레임 p95가 " + fmt(M.target_ms) + "ms를 넘기 직전의 호드 총수이고, 호드 1만에서 " + fmt(M.target_ms) + "ms를 넘는 단계 1과 2에만 쓴다.",
      "동등성 검사가 살아 있는 총수, 겹친 쌍, 캡슐 확인 트레이스 등으로 단계 사이가 같은 일을 하는지 조건마다 확인한다. 같은 일을 하지 못한 점은 기능 미달로 표시한다.",
      "측정 PC는 Ryzen 9 5950X, RTX 4090, 64GB, Windows 11, UE 5.8.4 소스 빌드다. 값은 이 PC 한 대와 평지 시나리오 하나에서 나왔다.",
      "이전 판은 태그 `v1`에 있고 겹침 방지와 캡슐이 없어 수치를 잇지 않는다."
    ];
    $("method").innerHTML = items.map(function (t) { return "<li>" + md(t) + "</li>"; }).join("") +
      '<li><a href="' + M.pc_specs + '">측정 PC 사양</a> · <a href="' + M.measurement_guide + '">측정 규약 전체</a> · <a href="' + M.summary_csv + '">최종 측정의 표(CSV)</a></li>';
  }
  function renderPosts() {
    $("posts").innerHTML = D.posts.map(function (p) {
      return '<li><span class="p-num">' + String(p.num).padStart(2, "0") + '</span><div><a href="' + p.url + '">' + esc(p.title) + "</a><p>" + md(p.sentence) + "</p></div></li>";
    }).join("");
  }

  // ---------- input ----------
  function bindInput() {
    $("prev").addEventListener("click", function () { setStep(state.step - 1, true); });
    $("next").addEventListener("click", function () { setStep(state.step + 1, true); });
    $("nsel").addEventListener("click", function (e) {
      var b = e.target.closest("[data-n]");
      if (b && !b.disabled) { setN(+b.getAttribute("data-n")); }
    });
    $("clip-load").addEventListener("click", function () { userMotionOk = true; loadClip(stepBy(state.step)); });
    document.addEventListener("keydown", function (e) {
      if (e.key !== "ArrowLeft" && e.key !== "ArrowRight") { return; }
      if (e.altKey || e.ctrlKey || e.metaKey || e.shiftKey) { return; }
      var t = e.target;
      if (t.closest && (t.closest("#tabs") || t.closest("input, textarea, select, [contenteditable]"))) { return; }
      var r = $("steps").getBoundingClientRect();
      if (r.bottom < 80 || r.top > window.innerHeight - 80) { return; }
      e.preventDefault();
      setStep(state.step + (e.key === "ArrowRight" ? 1 : -1), true);
    });
    // swipe on the clip
    var frame = document.querySelector(".frame"), sx = null, sy = null;
    frame.addEventListener("pointerdown", function (e) { sx = e.clientX; sy = e.clientY; });
    frame.addEventListener("pointerup", function (e) {
      if (sx === null) { return; }
      var dx = e.clientX - sx, dy = e.clientY - sy;
      sx = null;
      if (Math.abs(dx) > 40 && Math.abs(dx) > Math.abs(dy) * 1.5) { setStep(state.step + (dx < 0 ? 1 : -1), true); }
    });
    frame.addEventListener("pointercancel", function () { sx = null; });
    window.addEventListener("hashchange", function () { readHash(); renderPanel(false); });
    var lastW = window.innerWidth, timer = null;
    window.addEventListener("resize", function () {
      if (window.innerWidth === lastW) { return; }
      lastW = window.innerWidth;
      clearTimeout(timer);
      timer = setTimeout(renderPool, 150);
    });
  }

  readHash();
  renderHeader();
  renderTabs();
  renderLadder();
  renderMarginChart();
  renderAblation();
  renderPool();
  renderMethod();
  renderPosts();
  bindInput();
  renderPanel(false);
})();
