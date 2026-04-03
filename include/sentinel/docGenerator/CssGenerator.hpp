/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_

#include <stddef.h>
#include <string>
#include <vector>
#include "sentinel/docGenerator/DocGenerator.hpp"

namespace sentinel {

/**
 * @brief CssGenerator class
 */
class CssGenerator : public DocGenerator {
 public:
  /**
   * @brief make css string
   */
  inline std::string str() const override {
    return styleCssContent;
  }

 private:
  std::string styleCssContent =
      R"(/* ============================================================
   Sentinel Mutation Testing Report
   Palette: Deep Teal + Vivid accents
   Base tones: #036098 (accent), #002030 (text)
   ============================================================ */

:root {
  --bg:          #f2f4f7;
  --bg-white:    #ffffff;
  --bg-muted:    #e8ecf1;
  --border:      #d0d5de;
  --border-light:#dfe3ea;

  --text:        #002030;
  --text-sec:    #2c4050;
  --text-muted:  #607080;

  --green:       #0f8a5f;
  --green-bg:    #d0eddb;
  --red:         #d44030;
  --red-bg:      #f8d8d2;
  --skip:        #94a0b0;
  --skip-bg:     #eef0f4;

  --accent:      #036098;
  --accent-light:#0880c8;
  --accent-hover:#024a76;
  --accent-bg:   #e8f2fa;

  --font:        -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
  --font-mono:   SFMono-Regular, Consolas, 'Liberation Mono', Menlo, monospace;
  --radius:      8px;
  --radius-sm:   5px;
}

*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html { font-size: 15px; -webkit-font-smoothing: antialiased; }
body { font-family: var(--font); background: var(--bg); color: var(--text); line-height: 1.6; }
.wrap { max-width: 1140px; margin: 0 auto; padding: 28px 24px 48px; }

/* ================================================================
   HEADER
   ================================================================ */
.hdr {
  display: flex; align-items: center; justify-content: space-between;
  padding-bottom: 20px; border-bottom: 2px solid var(--border); margin-bottom: 24px;
}
.hdr__left { display: flex; align-items: center; gap: 10px; }
.hdr__icon { width: 30px; height: 30px; flex-shrink: 0; }
.hdr h1 { font-size: 1.12rem; font-weight: 700; color: var(--text); }
.hdr__right { display: flex; gap: 10px; }
.badge {
  font-size: .74rem; font-weight: 600; color: var(--text-sec);
  background: var(--bg-muted); border: 1px solid var(--border);
  border-radius: 16px; padding: 4px 12px;
}

/* ================================================================
   BREADCRUMB
   ================================================================ */
.crumb { font-size: .84rem; color: var(--text-muted); margin-bottom: 20px; }
.crumb a { color: var(--accent); text-decoration: none; font-weight: 600; }
.crumb a:hover { text-decoration: underline; color: var(--accent-hover); }
.crumb__sep { margin: 0 5px; opacity: .4; }

/* ================================================================
   CARDS
   ================================================================ */
.cards { display: grid; gap: 14px; margin-bottom: 24px; }
.cards--4 { grid-template-columns: repeat(4, 1fr); }
.cards--3 { grid-template-columns: repeat(3, 1fr); }

.card {
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 18px 20px;
  border-top: 3px solid transparent;
}
.card--score  { border-top-color: var(--accent); }
.card--killed { border-top-color: var(--green); }
.card--surv   { border-top-color: var(--red); }
.card--time   { border-top-color: var(--skip); }
.card--skip   { border-top-color: #6b7585; }
.card--skip   .card__val { color: #6b7585; }

.card__lbl {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 6px;
}
.card__val { font-size: 1.65rem; font-weight: 800; letter-spacing: -.02em; line-height: 1.15; }
.card--score  .card__val { color: var(--accent); }
.card--killed .card__val { color: var(--green); }
.card--surv   .card__val { color: var(--red); }
.card--time   .card__val { color: var(--text); }
.card__sub { font-size: .78rem; color: var(--text-sec); margin-top: 5px; }

.mini-bar { margin-top: 10px; height: 5px; background: var(--bg-muted); border-radius: 3px; overflow: hidden; }
.mini-bar__fill { height: 100%; border-radius: 3px; background: var(--accent); }

/* ================================================================
   PANELS
   ================================================================ */
.row2 { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; margin-bottom: 24px; }

.panel {
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 20px;
}
.panel__t {
  font-size: .74rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 16px;
}

/* donut */
.donut-wrap { display: flex; align-items: center; gap: 28px; }
.donut-svg { width: 140px; height: 140px; flex-shrink: 0; }
.legend { display: flex; flex-direction: column; gap: 10px; }
.legend-i { display: flex; align-items: center; gap: 8px; font-size: .86rem; color: var(--text); }
.legend-dot { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; }
.legend-i .n { font-weight: 700; margin-left: auto; min-width: 24px; text-align: right; }

/* bar chart — consistent style across all uses */
.bars { display: flex; flex-direction: column; gap: 12px; }
.bar-hdr { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 4px; }
.bar-name { font-size: .84rem; font-weight: 600; color: var(--text-muted); }
.bar-cnt { font-size: .74rem; color: var(--text-muted); }
.bar-track { height: 7px; background: var(--bg-muted); border-radius: 4px; overflow: hidden; display: flex; }
.bar-k { height: 100%; background: var(--green); }
.bar-s { height: 100%; background: var(--red); }
.bar-x { height: 100%; background: var(--skip); }
.bar-build { height: 100%; background: #036098; }
.bar-test  { height: 100%; background: #48b0e0; }

/* ================================================================
   CONFIG
   ================================================================ */
.cfg-btn {
  width: 100%; background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 13px 18px; color: var(--text-sec);
  font-family: var(--font); font-size: .78rem; font-weight: 700;
  text-transform: uppercase; letter-spacing: .06em;
  cursor: pointer; display: flex; align-items: center; justify-content: space-between;
}
.cfg-btn:hover { background: var(--bg-muted); }
.cfg-btn__arr { transition: transform .2s; font-size: .65rem; }
.cfg-btn[aria-expanded="true"] .cfg-btn__arr { transform: rotate(180deg); }
.cfg-btn[aria-expanded="true"] { border-radius: var(--radius) var(--radius) 0 0; }

.cfg-body {
  background: var(--bg-white); border: 1px solid var(--border); border-top: none;
  border-radius: 0 0 var(--radius) var(--radius);
  max-height: 0; overflow: hidden; transition: max-height .3s ease, padding .3s ease;
  padding: 0 18px;
}
.cfg-body.open { max-height: 500px; padding: 14px 18px 18px; }

.cfg-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 6px 28px; }
.cfg-item { display: flex; gap: 8px; font-size: .84rem; padding: 5px 0; border-bottom: 1px solid var(--border-light); }
.cfg-item:last-child { border-bottom: none; }
.cfg-k { color: var(--text-muted); font-weight: 600; min-width: 100px; font-family: var(--font-mono); font-size: .76rem; }
.cfg-v { color: var(--text); font-family: var(--font-mono); font-size: .8rem; word-break: break-all; }

/* ================================================================
   TABLE
   ================================================================ */
.tbl-sec { margin-top: 24px; }
.sec-t {
  font-size: .76rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 12px;
}

.dtbl {
  width: 100%; border-collapse: separate; border-spacing: 0;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); overflow: hidden;
}
.dtbl th {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-muted);
  padding: 10px 14px; text-align: left;
  background: var(--bg-muted); border-bottom: 1px solid var(--border);
}
.dtbl td {
  padding: 10px 14px; font-size: .86rem; color: var(--text);
  border-bottom: 1px solid var(--border-light); vertical-align: middle;
}
.dtbl tr:last-child td { border-bottom: none; }
.dtbl tr:hover td { background: var(--bg-muted); }
.dtbl a { color: var(--accent); text-decoration: none; font-weight: 600; }
.dtbl a:hover { text-decoration: underline; color: var(--accent-hover); }

.cov-cell { display: flex; align-items: center; gap: 8px; }
.cov-pct { font-family: var(--font-mono); font-weight: 600; font-size: .84rem; min-width: 38px; text-align: right; }
.cov-bar { flex: 1; height: 7px; background: var(--bg-muted); border-radius: 4px; overflow: hidden; max-width: 160px; }
.cov-bar__fill { height: 100%; border-radius: 4px; }
.cov-ratio { font-size: .76rem; color: var(--text-muted); font-family: var(--font-mono); min-width: 32px; }

.c-hi  { color: var(--green); }
.c-mid { color: #c07b10; }
.c-lo  { color: var(--red); }
.f-hi  { background: var(--green); }
.f-mid { background: #d99020; }
.f-lo  { background: var(--red); }

/* ================================================================
   SOURCE CODE
   ================================================================ */
.src-tbl {
  width: 100%; border-collapse: collapse;
  font-family: var(--font-mono); font-size: .84rem;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); overflow: hidden;
}
.src-tbl tr:hover { background: var(--bg-muted); }

.src-tbl .ln {
  width: 44px; text-align: right; padding: 2px 8px 2px 6px;
  color: var(--text-muted); background: var(--bg-muted);
  border-right: 1px solid var(--border); user-select: none; font-size: .8rem;
}
.src-tbl .mi {
  width: 32px; text-align: center; padding: 2px 4px;
  font-size: .74rem; font-weight: 700; cursor: pointer;
}
.src-tbl .cd { padding: 2px 10px; white-space: pre; color: var(--text); }
.src-tbl .cd pre { margin: 0; font-family: inherit; font-size: inherit; }

.lk .mi { background: var(--green-bg); color: var(--green); }
.lk .cd { background: var(--green-bg); }
.ls .mi { background: var(--red-bg); color: var(--red); }
.ls .cd { background: var(--red-bg); }

/* popup — fixed to viewport, positioned by JS */
.mpop {
  display: none; position: fixed; z-index: 1000;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 14px 18px;
  width: 400px; max-height: 70vh; overflow-y: auto;
  box-shadow: 0 8px 30px rgba(0,32,48,.14);
  font-family: var(--font); font-weight: 400; text-align: left; line-height: 1.6;
}
.mpop.visible { display: block; }
.mpop__t { font-weight: 700; font-size: .84rem; margin-bottom: 8px; display: flex; align-items: center; gap: 6px; }
.mpop__badge { font-size: .66rem; padding: 2px 8px; border-radius: 8px; font-weight: 700; text-transform: uppercase; }
.b-k { background: var(--green-bg); color: var(--green); }
.b-s { background: var(--red-bg); color: var(--red); }
.mpop__lbl { font-size: .72rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: .04em; margin-top: 6px; }
.mpop__code { background: var(--bg-muted); border-radius: var(--radius-sm); padding: 6px 10px; margin: 4px 0; font-family: var(--font-mono); font-size: .8rem; color: var(--text); }
.mpop__sep { border: none; border-top: 1px solid var(--border-light); margin: 10px 0; }

/* ================================================================
   MUTATIONS LIST
   ================================================================ */
.mlist { display: flex; flex-direction: column; gap: 6px; }
.ment {
  display: grid; grid-template-columns: minmax(28px, auto) minmax(220px, auto) minmax(0, 1fr) auto;
  align-items: center; column-gap: 10px;
  padding: 8px 12px; background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius-sm); font-size: .84rem; color: var(--text);
}
.ment:hover { background: var(--bg-muted); }
.ment__ln { font-family: var(--font-mono); font-weight: 600; color: var(--accent); min-width: 28px; }
.ment__op { font-family: var(--font-mono); font-weight: 500; }
.ment__kl {
  min-width: 0; font-size: .78rem; color: var(--text-muted);
  text-align: right; justify-self: end; overflow-wrap: anywhere;
}
.ment__st {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  padding: 2px 8px; border-radius: 8px; justify-self: end;
}

.tags { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 6px; }
.tag {
  font-size: .76rem; font-family: var(--font-mono); padding: 3px 10px;
  border-radius: 12px; background: var(--bg-muted); border: 1px solid var(--border); color: var(--text-sec);
}

/* ================================================================
   FOOTER
   ================================================================ */
.ftr {
  margin-top: 36px; padding-top: 16px; border-top: 2px solid var(--border);
  font-size: .76rem; color: var(--text-muted); text-align: center;
}
.ftr a { color: var(--accent); text-decoration: none; font-weight: 600; }
.ftr a:hover { text-decoration: underline; }

@media (max-width: 900px) {
  .cards--4 { grid-template-columns: repeat(2,1fr); }
  .cards--3 { grid-template-columns: repeat(2,1fr); }
  .row2 { grid-template-columns: 1fr; }
  .cfg-grid { grid-template-columns: 1fr; }
}
@media (max-width: 600px) {
  .cards--4, .cards--3 { grid-template-columns: 1fr; }
  .hdr { flex-direction: column; gap: 10px; align-items: flex-start; }
})";
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_
