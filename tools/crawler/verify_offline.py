#!/usr/bin/env python3
"""
离线核验 代码库/ 现有归档（第 2 步：不联网，只检查已落盘文件）。

检查内容：
1. 每题目录必需文件是否齐全（题目原文.html / 提交详情.html / 完整代码 / 可提交代码 / 元数据.json）。
2. 代码文件 sha256 是否与元数据一致（发现被后续脚本或编辑器意外改动的文件）。
3. 元数据 JSON 是否可解析、字段是否完整。
4. 题目原文.html 是否含"附件下载表单"（downloadAttachHandler），标记为附件候选，
   需要在校园网 + 登录会话下在线 POST 确认是否真的有附件文件。
5. 汇总与 AC抓取清单.json 的题目数、题号集合是否一致。

不做的事（本脚本明确不处理，需在线复核）：
- 不判断多提交框（真正的提交表单页未归档，静态 题目原文.html/提交详情.html 中只看到
  单一 id="editor"，不能代表提交页只有一个代码框）。
- 不下载任何附件文件（本脚本只做本地文件检查，不发起网络请求）。

用法：
    python3 tools/crawler/verify_offline.py [--repo-root PATH] [--report PATH]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".", help="仓库根目录（含 代码库/）")
    ap.add_argument("--report", default=None, help="报告输出路径，默认写到 logs/verify_offline_report.md")
    args = ap.parse_args()

    root = Path(args.repo_root).resolve()
    code_dir = root / "代码库"
    if not code_dir.is_dir():
        print(f"未找到 代码库/ 目录：{code_dir}", file=sys.stderr)
        return 2

    manifest_path = code_dir / "AC抓取清单.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else None

    problem_dirs = sorted(p for p in code_dir.iterdir() if p.is_dir())

    missing_files: list[str] = []
    sha_mismatch: list[str] = []
    bad_json: list[str] = []
    attachment_candidates: list[str] = []
    ok_count = 0

    for pdir in problem_dirs:
        pid = pdir.name.split("_", 1)[0]
        meta_path = pdir / f"{pid}_元数据.json"
        if not meta_path.exists():
            missing_files.append(f"{pdir.name}: 缺少 {pid}_元数据.json")
            continue
        try:
            meta = json.loads(meta_path.read_text(encoding="utf-8"))
        except Exception as e:  # noqa: BLE001
            bad_json.append(f"{pdir.name}: 元数据 JSON 解析失败 ({e})")
            continue

        files = meta.get("files", {})
        required = ["problemHtml", "submissionHtml", "completeCode", "directlySubmittableCode"]
        missing_here = []
        for key in required:
            rel = files.get(key)
            if not rel or not (pdir / rel).exists():
                missing_here.append(key)
        if missing_here:
            missing_files.append(f"{pdir.name}: 缺少 {', '.join(missing_here)}")
            continue

        # sha256 校验（针对 completeCode）
        expected_sha = (meta.get("code") or {}).get("sha256")
        complete_code_path = pdir / files["completeCode"]
        if expected_sha:
            actual_sha = sha256_of(complete_code_path)
            if actual_sha != expected_sha:
                sha_mismatch.append(
                    f"{pdir.name}: sha256 不一致（元数据={expected_sha[:12]}… 实际={actual_sha[:12]}…）"
                )
                continue

        # 附件候选检测（仅本地静态检查，不发起网络请求）
        problem_html_path = pdir / files["problemHtml"]
        try:
            html_text = problem_html_path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            html_text = ""
        if "downloadAttachHandler" in html_text:
            attachment_candidates.append(pdir.name)

        ok_count += 1

    # 与全局清单交叉核对
    manifest_mismatch: list[str] = []
    if manifest:
        manifest_ids = {p["problemNo"] for p in manifest.get("problems", [])}
        dir_ids = {p.name.split("_", 1)[0].lstrip("0") or "0" for p in problem_dirs}
        only_in_manifest = manifest_ids - dir_ids
        only_in_dirs = dir_ids - manifest_ids
        if only_in_manifest:
            manifest_mismatch.append(f"清单中有但目录缺失: {sorted(only_in_manifest)}")
        if only_in_dirs:
            manifest_mismatch.append(f"目录中有但清单缺失: {sorted(only_in_dirs)}")

    lines: list[str] = []
    lines.append("# 离线核验报告（P2 第 2 步）")
    lines.append("")
    lines.append(f"- 题目目录总数：{len(problem_dirs)}")
    lines.append(f"- 文件齐全且 sha256 一致：{ok_count}")
    lines.append(f"- 缺文件：{len(missing_files)}")
    lines.append(f"- 元数据 JSON 解析失败：{len(bad_json)}")
    lines.append(f"- sha256 不一致：{len(sha_mismatch)}")
    lines.append(f"- 与 AC抓取清单.json 题号不一致：{len(manifest_mismatch)}")
    lines.append(f"- 附件候选（题面含下载附件表单，需在线复核是否真有文件）：{len(attachment_candidates)}")
    lines.append("")

    def _section(title: str, items: list[str]) -> None:
        lines.append(f"## {title}（{len(items)}）")
        if not items:
            lines.append("（无）")
        else:
            for it in items:
                lines.append(f"- {it}")
        lines.append("")

    _section("缺文件明细", missing_files)
    _section("元数据 JSON 解析失败明细", bad_json)
    _section("sha256 不一致明细", sha_mismatch)
    _section("与全局清单不一致明细", manifest_mismatch)
    _section("附件候选题目（需在线 POST downloadAttachHandler 确认）", attachment_candidates)

    lines.append("## 明确未覆盖的检查项（需在线，本脚本不做）")
    lines.append("")
    lines.append("- 多提交框识别：需要登录后打开真实“提交/编辑代码”页面，统计代码编辑器/textarea 数量与字段名；")
    lines.append("  当前归档的题面页与提交详情页均只体现单一编辑器，不能作为“该题只有一个提交框”的证据。")
    lines.append(
        "- 附件文件本体：需要在线 POST `/index.php/index/problem/downloadAttachHandler.html`"
        "（参数 `pno`）下载并核对是否返回非空文件。"
    )
    lines.append("")

    report = "\n".join(lines)
    out_path = Path(args.report) if args.report else (root / "logs" / "verify_offline_report.md")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(report, encoding="utf-8")
    print(report)
    print(f"\n报告已写入：{out_path}")

    return 1 if (missing_files or bad_json or sha_mismatch or manifest_mismatch) else 0


if __name__ == "__main__":
    raise SystemExit(main())
