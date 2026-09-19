#!/usr/bin/env python3
"""Build the first offline, reviewable public-facing YOJ repository skeleton.

This script deliberately does not log in, submit code, mutate the raw archive,
or push to GitHub.  It materializes problem statements from the captured
``p_content`` element and writes a public-safe index whose records are still
marked ``RAW_CAPTURED`` until code cleaning and online re-verification finish.

Runtime dependency: lxml.  The generated Markdown is intentionally based on
the statement body rather than on the complete HTML page, and therefore does
not copy login state, navigation, or submission-detail HTML into the index.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable
from urllib.parse import quote, unquote, urljoin, urlparse
from urllib.request import Request, urlopen

try:
    from lxml import html
except ImportError as exc:  # pragma: no cover - exercised only on a bad host
    raise SystemExit("缺少 lxml；请先安装 lxml 后再运行 tools/build_initial.py") from exc


ROOT = Path(__file__).resolve().parents[1]
RAW_ROOT = ROOT / "代码库"
PUBLIC_ROOT = ROOT / "题解"
DATA_ROOT = ROOT / "data"
MANIFEST_PATH = RAW_ROOT / "AC抓取清单.json"
SITE_BASE = "http://yoj.ruc.edu.cn/"
SCHEMA_VERSION = 1
MAX_ASSET_BYTES = 64 * 1024 * 1024
ASSET_EXTENSIONS = {
    ".7z",
    ".bin",
    ".csv",
    ".dat",
    ".doc",
    ".docx",
    ".gz",
    ".in",
    ".json",
    ".pdf",
    ".rar",
    ".tar",
    ".txt",
    ".xls",
    ".xlsx",
    ".zip",
}


def json_load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.replace("\r\n", "\n").replace("\r", "\n"), encoding="utf-8")


def write_json(path: Path, value: Any) -> None:
    write_text(path, json.dumps(value, ensure_ascii=False, indent=2) + "\n")


def local_name(node: Any) -> str:
    tag = getattr(node, "tag", "")
    if not isinstance(tag, str):
        return ""
    return tag.rsplit("}", 1)[-1].lower()


def classes(node: Any) -> set[str]:
    return set((node.get("class") or "").split())


def text_content(node: Any) -> str:
    return "".join(node.itertext())


def normalize_inline_text(value: str) -> str:
    value = value.replace("\xa0", " ").replace("\u200b", "")
    value = value.replace("\r", "").replace("\n", " ")
    return re.sub(r"[ \t]+", " ", value)


MATH_OPERATORS = {
    "≤": r"\\le",
    "≥": r"\\ge",
    "≠": r"\\ne",
    "≈": r"\\approx",
    "±": r"\\pm",
    "×": r"\\times",
    "÷": r"\\div",
    "·": r"\\cdot",
    "∈": r"\\in",
    "∉": r"\\notin",
    "⊆": r"\\subseteq",
    "⊂": r"\\subset",
    "⊇": r"\\supseteq",
    "⊃": r"\\supset",
    "∪": r"\\cup",
    "∩": r"\\cap",
    "∧": r"\\land",
    "∨": r"\\lor",
    "¬": r"\\lnot",
    "→": r"\\to",
    "←": r"\\leftarrow",
    "↔": r"\\leftrightarrow",
    "∞": r"\\infty",
    "∑": r"\\sum",
    "∏": r"\\prod",
    "∫": r"\\int",
    "√": r"\\sqrt",
    "…": r"\\ldots",
    "⋯": r"\\cdots",
    "−": "-",
    "–": "-",
    "—": "-",
    "∣": "|",
    "⁄": "/",
}


def math_text(node: Any) -> str:
    """Convert the common MathML emitted by KaTeX to readable LaTeX.

    YOJ's captured pages usually contain rendered KaTeX MathML without the
    original source annotation.  This is intentionally a conservative
    converter: it preserves the mathematical structure for ordinary contest
    formulas and leaves an explicit review marker for unusual MathML.
    """

    tag = local_name(node)
    children = list(node)

    if tag in {"math", "semantics", "mstyle", "mpadded", "mphantom"}:
        if tag == "semantics" and children:
            # The first child is the presentation tree; annotations are not
            # needed when KaTeX has already emitted MathML.
            return math_text(children[0])
        return "".join(math_text(child) for child in children)
    if tag in {"mrow", "mstack", "mline"}:
        return "".join(math_text(child) for child in children)
    if tag in {"mi", "mn", "mtext", "ms"}:
        return normalize_inline_text(text_content(node)).strip()
    if tag == "mo":
        value = normalize_inline_text(text_content(node)).strip()
        return MATH_OPERATORS.get(value, value)
    if tag == "mspace":
        return r"\ "
    if tag == "msub" and len(children) >= 2:
        return f"{group_math(math_text(children[0]))}_{{{math_text(children[1])}}}"
    if tag == "msup" and len(children) >= 2:
        return f"{group_math(math_text(children[0]))}^{{{math_text(children[1])}}}"
    if tag == "msubsup" and len(children) >= 3:
        return (
            f"{group_math(math_text(children[0]))}_{{{math_text(children[1])}}}"
            f"^{{{math_text(children[2])}}}"
        )
    if tag == "mfrac" and len(children) >= 2:
        return f"\\frac{{{math_text(children[0])}}}{{{math_text(children[1])}}}"
    if tag == "msqrt" and children:
        return f"\\sqrt{{{''.join(math_text(child) for child in children)}}}"
    if tag == "mroot" and len(children) >= 2:
        return f"\\sqrt[{math_text(children[1])}]{{{math_text(children[0])}}}"
    if tag in {"mover", "munder", "munderover"} and children:
        base = math_text(children[0])
        if tag == "mover" and len(children) >= 2:
            accent = math_text(children[1])
            command = {"^": r"\\hat", "~": r"\\tilde", "¯": r"\\bar"}.get(accent, r"\\overset")
            return f"{command}{{{base}}}" if command != r"\\overset" else f"\\overset{{{accent}}}{{{base}}}"
        if tag == "munder" and len(children) >= 2:
            return f"\\underset{{{math_text(children[1])}}}{{{base}}}"
        if len(children) >= 3:
            return f"\\underset{{{math_text(children[1])}}}{{\\overset{{{math_text(children[2])}}}{{{base}}}}}"
    if tag == "mfenced":
        left = node.get("open", "(")
        right = node.get("close", ")")
        sep = node.get("separators", ",")[:1] or ","
        return f"{left}{sep.join(math_text(child) for child in children)}{right}"
    if tag in {"mtable", "mtr", "mtd"}:
        if tag == "mtd":
            return "".join(math_text(child) for child in children)
        if tag == "mtr":
            return " & ".join(math_text(child) for child in children)
        return r"\\ ".join(math_text(child) for child in children)
    if tag == "menclose" and children:
        return f"\\boxed{{{''.join(math_text(child) for child in children)}}}"
    if tag == "annotation":
        return normalize_inline_text(text_content(node)).strip()

    # A conservative fallback for MathML elements not listed above.
    fallback = normalize_inline_text(text_content(node)).strip()
    return fallback or "[公式待核对]"


def group_math(value: str) -> str:
    if not value:
        return "{}"
    if len(value) == 1 and value.isalnum():
        return value
    if value.startswith("\\") and "{" not in value and "}" not in value:
        return value
    return f"{{{value}}}"


class StatementConverter:
    """Render only a problem's statement body into Markdown."""

    block_tags = {"p", "div", "section", "article", "blockquote", "figure", "figcaption"}
    list_tags = {"ul", "ol"}

    def __init__(self, problem_url: str, asset_dir: Path | None = None, download_assets: bool = False) -> None:
        self.problem_url = problem_url or SITE_BASE
        self.images: list[str] = []
        self.formula_count = 0
        self.warnings: list[str] = []
        self.asset_dir = asset_dir
        self.download_assets = download_assets
        self.assets: list[dict[str, Any]] = []
        self._asset_by_url: dict[str, dict[str, Any]] = {}

    def absolute_url(self, value: str) -> str:
        return urljoin(self.problem_url, value)

    def is_download_candidate(self, source: str, label: str) -> bool:
        parsed = urlparse(source)
        if parsed.scheme.lower() in {"javascript", "data", "mailto"}:
            return False
        path = unquote(parsed.path).lower()
        if not path or path.endswith((".html", ".htm")):
            return False
        if Path(path).suffix in ASSET_EXTENSIONS:
            return True
        markers = ("/download/", "/upload/", "/uploads/", "/data/")
        return any(marker in path for marker in markers) or "下载" in label or "附件" in label

    def asset_filename(self, source: str, index: int) -> str:
        raw_name = Path(unquote(urlparse(source).path)).name or "resource"
        raw_name = re.sub(r"[^0-9A-Za-z一-龥._-]+", "_", raw_name).strip("._") or "resource"
        return f"{index:02d}_{raw_name}"

    def materialize_asset(self, source: str, label: str) -> dict[str, Any]:
        if source in self._asset_by_url:
            return self._asset_by_url[source]
        index = len(self.assets) + 1
        filename = self.asset_filename(source, index)
        local_path = self.asset_dir / filename if self.asset_dir is not None else None
        item: dict[str, Any] = {
            "url": source,
            "label": label or filename,
            "filename": filename,
            "relativePath": f"assets/{filename}",
            "status": "DETECTED_NOT_DOWNLOADED",
            "bytes": 0,
            "sha256": "",
            "error": "",
        }
        if self.download_assets and local_path is not None:
            try:
                local_path.parent.mkdir(parents=True, exist_ok=True)
                if local_path.is_file() and local_path.stat().st_size > 0:
                    content_hash = source_file_hash(local_path)
                    item["status"] = "CACHED"
                    item["bytes"] = local_path.stat().st_size
                    item["sha256"] = content_hash
                else:
                    request = Request(source, headers={"User-Agent": "RUC-YOJ-Archive/1.0"})
                    with urlopen(request, timeout=20) as response:  # nosec B310 - URL comes from captured problem HTML
                        content = response.read(MAX_ASSET_BYTES + 1)
                    if len(content) > MAX_ASSET_BYTES:
                        raise ValueError(f"资源超过 {MAX_ASSET_BYTES // (1024 * 1024)} MiB 限制")
                    local_path.write_bytes(content)
                    item["status"] = "DOWNLOADED"
                    item["bytes"] = len(content)
                    item["sha256"] = hashlib.sha256(content).hexdigest()
            except (OSError, ValueError, TimeoutError) as exc:
                item["status"] = "DOWNLOAD_FAILED"
                item["error"] = f"{exc.__class__.__name__}: {exc}"
                self.warnings.append(f"附件下载失败: {source} ({item['error']})")
        self.assets.append(item)
        self._asset_by_url[source] = item
        return item

    def inline(self, node: Any) -> str:
        tag = local_name(node)
        if tag in {"script", "style", "noscript", "svg", "canvas"}:
            return ""
        if tag == "br":
            return "\n"
        if tag == "img":
            src = (node.get("src") or "").strip()
            if not src:
                self.warnings.append("发现无 src 的图片节点")
                return "[图片待补充]"
            source = self.absolute_url(src)
            if source not in self.images:
                self.images.append(source)
            alt = normalize_inline_text(node.get("alt") or "题面图片").strip() or "题面图片"
            return f"![{alt}]({source})"
        if "katex" in classes(node):
            math_nodes = node.xpath('.//*[local-name()="math"]')
            self.formula_count += 1
            if not math_nodes:
                self.warnings.append("KaTeX 节点缺少 MathML")
                return r"\(公式待核对\)"
            latex = math_text(math_nodes[0]).strip()
            ancestor_classes = set()
            parent = node.getparent()
            if parent is not None:
                ancestor_classes = classes(parent)
            if "katex-display" in classes(node) or "katex-display" in ancestor_classes:
                return f"\n\\[\n{latex}\n\\]\n"
            return f"\\({latex}\\)"
        if tag == "code":
            value = normalize_inline_text(text_content(node)).strip()
            fence = "``" if "`" in value else "`"
            return f"{fence}{value}{fence}"
        if tag == "a":
            href = (node.get("href") or "").strip()
            label = self.inline_children(node).strip()
            if not href or href.lower().startswith(("javascript:", "data:")):
                return label
            source = self.absolute_url(href)
            if self.is_download_candidate(source, label):
                asset = self.materialize_asset(source, label)
                if asset["status"] in {"DOWNLOADED", "CACHED"}:
                    return f"[{label or asset['filename']}]({asset['relativePath']})"
                return f"[{label or asset['filename']}]({source})"
            return f"[{label or href}]({source})"
        if tag == "sup":
            return f"^{{{self.inline_children(node).strip()}}}"
        if tag == "sub":
            return f"_{{{self.inline_children(node).strip()}}}"
        if tag in {"strong", "b"}:
            return f"**{self.inline_children(node).strip()}**"
        if tag in {"em", "i"}:
            return f"*{self.inline_children(node).strip()}*"
        return self.inline_children(node, own_text=True)

    def inline_children(self, node: Any, own_text: bool = True) -> str:
        parts: list[str] = []
        if own_text and node.text:
            parts.append(normalize_inline_text(node.text))
        for child in node:
            parts.append(self.inline(child))
            if child.tail:
                parts.append(normalize_inline_text(child.tail))
        return "".join(parts)

    def render_list(self, node: Any, indent: str = "") -> str:
        ordered = local_name(node) == "ol"
        lines: list[str] = []
        index = 1
        for item in node.xpath("./li"):
            marker = f"{index}. " if ordered else "- "
            main_parts: list[str] = []
            if item.text:
                main_parts.append(normalize_inline_text(item.text))
            nested: list[Any] = []
            for child in item:
                if local_name(child) in self.list_tags:
                    nested.append(child)
                else:
                    main_parts.append(self.inline(child))
                if child.tail:
                    main_parts.append(normalize_inline_text(child.tail))
            main = "".join(main_parts).strip()
            if main:
                lines.append(f"{indent}{marker}{main}")
            for child in nested:
                nested_text = self.render_list(child, indent + "  ").rstrip()
                if nested_text:
                    lines.append(nested_text)
            index += 1
        return "\n".join(lines) + ("\n\n" if lines else "")

    def render_table(self, node: Any) -> str:
        rows: list[list[str]] = []
        for row in node.xpath(".//tr"):
            cells: list[str] = []
            for cell in row.xpath("./th|./td"):
                value = self.inline_children(cell).strip().replace("|", r"\|")
                value = re.sub(r"\s*\n\s*", " ", value)
                cells.append(value)
            if cells:
                rows.append(cells)
        if not rows:
            return ""
        width = max(len(row) for row in rows)
        rows = [row + [""] * (width - len(row)) for row in rows]
        lines = ["| " + " | ".join(rows[0]) + " |", "| " + " | ".join(["---"] * width) + " |"]
        lines.extend("| " + " | ".join(row) + " |" for row in rows[1:])
        return "\n".join(lines) + "\n\n"

    def render_block(self, node: Any) -> str:
        tag = local_name(node)
        if tag in {"script", "style", "noscript", "svg", "canvas"}:
            return ""
        if tag in {"h1", "h2", "h3", "h4", "h5", "h6"}:
            content = self.inline_children(node).strip()
            return f"{'#' * int(tag[1])} {content}\n\n" if content else ""
        if tag == "pre":
            content = text_content(node).replace("\xa0", " ").replace("\r\n", "\n").replace("\r", "\n").strip("\n")
            if not content.strip():
                return ""
            fence_len = max(3, max((len(run) for run in re.findall(r"`+", content)), default=0) + 1)
            fence = "`" * fence_len
            return f"{fence}\n{content}\n{fence}\n\n"
        if tag in self.list_tags:
            return self.render_list(node)
        if tag == "table":
            return self.render_table(node)
        if tag == "br":
            return "\n"
        if tag in self.block_tags:
            content = self.render_children(node).strip()
            return f"{content}\n\n" if content else ""
        return self.render_children(node)

    def render_children(self, node: Any) -> str:
        parts: list[str] = []
        if node.text:
            parts.append(normalize_inline_text(node.text))
        for child in node:
            if local_name(child) in self.block_tags | self.list_tags | {"pre", "table", "h1", "h2", "h3", "h4", "h5", "h6"}:
                parts.append(self.render_block(child))
            else:
                parts.append(self.inline(child))
            if child.tail:
                parts.append(normalize_inline_text(child.tail))
        return "".join(parts)

    def convert(self, document: Any) -> str:
        candidates = document.xpath("//*[@id='p_content']")
        if not candidates:
            candidates = document.xpath("//*[contains(concat(' ', normalize-space(@class), ' '), ' font-content ')]")
        if not candidates:
            self.warnings.append("未找到 p_content 或 font-content，题面需要人工补抓")
            return ""
        body = self.render_block(candidates[0])
        body = re.sub(r"[ \t]+\n", "\n", body)
        body = re.sub(r"\n{3,}", "\n\n", body).strip()
        return body


def markdown_path(path: str | Path) -> str:
    return quote(Path(path).as_posix(), safe="/-_.~")


def relative_path(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def source_file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_html_snapshot(path: Path) -> Any:
    """Decode captured HTML before handing it to lxml.

    Some YOJ responses contain a UTF-8 meta tag but lxml's byte parser can
    still select a single-byte encoding because of legacy page headers.  An
    explicit decode prevents Chinese text from becoming ``æ...`` mojibake.
    """

    raw = path.read_bytes()
    for encoding in ("utf-8", "gb18030", "big5"):
        try:
            return html.fromstring(raw.decode(encoding))
        except UnicodeDecodeError:
            continue
    return html.fromstring(raw.decode("utf-8", errors="replace"))


def safe_language(value: Any, fallback: str) -> str:
    value = str(value or fallback or "unknown")
    return value.replace("\n", " ").strip() or "unknown"


def get_problem_no(entry: dict[str, Any]) -> int:
    try:
        return int(str(entry.get("problemNo", "0")))
    except ValueError:
        return 0


def build_statement(
    entry: dict[str, Any], metadata: dict[str, Any], raw_dir: Path, download_assets: bool = False
) -> tuple[dict[str, Any], str]:
    problem = metadata.get("problem", {})
    source = metadata.get("source", {})
    code_meta = metadata.get("code", {})
    title = str(problem.get("title") or entry.get("title") or "未命名题目").strip()
    problem_no = str(problem.get("problemNo") or entry.get("problemNo") or "0")
    problem_url = str(problem.get("problemUrl") or "").strip()
    html_name = str((metadata.get("files") or {}).get("problemHtml") or Path(entry["files"]["problem"]).name)
    html_path = raw_dir / html_name
    folder = raw_dir.name
    asset_dir = PUBLIC_ROOT / folder / "assets"
    converter = StatementConverter(problem_url, asset_dir=asset_dir, download_assets=download_assets)
    status = "CONVERTED_FROM_HTML_SNAPSHOT"
    body = ""
    if not html_path.is_file():
        converter.warnings.append(f"题面快照不存在: {html_name}")
        status = "NEEDS_REVIEW"
    else:
        try:
            document = parse_html_snapshot(html_path)
            body = converter.convert(document)
            if not body:
                status = "NEEDS_REVIEW"
        except (OSError, ValueError, html.ParserError) as exc:
            converter.warnings.append(f"题面快照解析失败: {exc.__class__.__name__}")
            status = "NEEDS_REVIEW"

    memory = str(problem.get("memory") or "未记录").strip()
    time_limit = str(problem.get("time") or "未记录").strip()
    source_files = metadata.get("files") or {}
    complete_code = str(source_files.get("completeCode") or entry["files"].get("completeCode") or "")
    direct_code = str(source_files.get("directlySubmittableCode") or entry["files"].get("directlySubmittableCode") or "")
    complete_path = raw_dir / complete_code
    direct_path = raw_dir / direct_code
    observed_hash = source_file_hash(complete_path) if complete_path.is_file() else ""
    recorded_hash = str(code_meta.get("sha256") or entry.get("codeSha256") or "")
    hash_match = bool(observed_hash and recorded_hash and observed_hash == recorded_hash)
    if not complete_path.is_file():
        converter.warnings.append(f"完整代码不存在: {complete_code}")
    if not direct_path.is_file():
        converter.warnings.append(f"可提交代码不存在: {direct_code}")

    statement_rel = Path("题解") / folder / f"{problem_no.zfill(4)}_题目.md"
    statement_text = [
        f"# {problem_no}. {title}",
        "",
        "> 当前文件由 YOJ 原始题面快照的 `p_content` 离线转换生成。公式尽量转换为 LaTeX，图片保留为 Markdown 图片链接。",
        "> 当前仓库阶段：`RAW_CAPTURED`。代码尚未完成脱敏清理、版本规范化、提交形态核验和在线复验。",
        "",
        "## 题目信息",
        "",
        f"- 原题地址：[{problem_url or '未记录'}]({problem_url or SITE_BASE})",
        f"- 时间限制：{time_limit}",
        f"- 内存限制：{memory}",
        f"- 归档语言：`{safe_language(problem.get('language'), code_meta.get('language') or entry.get('language'))}`",
        "",
        "---",
        "",
    ]
    statement_text.append(body or "> 题面未能自动提取，请在后续同步中人工复核。")
    statement_text.extend(
        [
            "",
            "---",
            "",
            "## 归档状态",
            "",
            f"- 题面转换：`{status}`",
            "- 代码状态：`RAW_CAPTURED`（不可视为已清洗的公开题解）",
            "- 在线 AC 复验：`NOT_RUN`",
            "- 直接提交分块：`UNKNOWN_UNTIL_FORM_MAP`",
        ]
    )
    if converter.warnings:
        statement_text.extend(["", "## 自动转换提醒", ""])
        statement_text.extend(f"- {warning}" for warning in sorted(set(converter.warnings)))
    statement_text.append("")

    record = {
        "problemNo": problem_no,
        "title": title,
        "folder": folder,
        "problemUrl": problem_url,
        "limits": {"time": time_limit, "memory": memory},
        "language": safe_language(problem.get("language"), code_meta.get("language") or entry.get("language")),
        "archive": {
            "status": "RAW_CAPTURED",
            "submissionNo": str(problem.get("submissionNo") or entry.get("submissionNo") or ""),
            "capturedAt": str(source.get("capturedAt") or ""),
            "sourceSha256": observed_hash,
            "recordedSha256": recorded_hash,
            "sourceHashMatchesMetadata": hash_match,
            "completeCode": relative_path(complete_path) if complete_path.is_file() else None,
            "directlySubmittableCode": relative_path(direct_path) if direct_path.is_file() else None,
            "blockHandling": code_meta.get("blockHandling") or {"status": "unknown"},
        },
        "public": {
            "status": "RAW_CAPTURED",
            "publish": False,
            "statement": relative_path(ROOT / statement_rel),
            "cleanCode": None,
            "directlySubmittableCode": None,
            "onlineVerification": "NOT_RUN",
        },
        "statement": {
            "status": status,
            "formulaCount": converter.formula_count,
            "imageCount": len(converter.images),
            "imageUrls": converter.images,
            "assets": converter.assets,
            "warnings": sorted(set(converter.warnings)),
        },
    }
    return record, "\n".join(statement_text)


def make_readme(records: list[dict[str, Any]], manifest: dict[str, Any]) -> str:
    captured_at = str(manifest.get("capturedAt") or "未记录")
    status_counts = Counter(record["public"]["status"] for record in records)
    lines = [
        "# RUC YOJ 题解归档",
        "",
        "> 这是按 Method 指引生成的离线初步构建。当前只把原始题面快照转换成 Markdown：公式尽量保留为 LaTeX，图片保留为题面原始图片链接；原始 HTML 不进入公开索引。",
        "> `代码库/` 是抓取原始归档，代码仍处于待脱敏、待 C++17/Python 3.14 规范化、待提交形态核验和待在线复验状态，不能把本页的“原始代码”链接理解为最终公开题解。",
        "",
        "## 当前状态",
        "",
        f"- 原始归档时间：`{captured_at}`",
        f"- 已生成题面：`{len(records)}` 道",
        f"- 状态统计：`{dict(sorted(status_counts.items()))}`",
        "- 自动同步、提交测试、AC 复抓和 GitHub 更新：本轮未执行",
        "- 维护窗口：按 Method 约定，`23:55–00:10` 暂停网络操作；题面更新需要重新抓取并复核图片、公式和题面差异",
        "",
        "## 题目索引",
        "",
        "| 题号 | 题目 | 题面（LaTeX/图片） | 原始完整代码 | 原始可提交代码 | 语言 | 状态 |",
        "| ---: | --- | --- | --- | --- | --- | --- |",
    ]
    for record in records:
        statement = markdown_path(record["public"]["statement"])
        complete = record["archive"].get("completeCode")
        direct = record["archive"].get("directlySubmittableCode")
        complete_link = f"[待清理]({markdown_path(complete)})" if complete else "—"
        direct_link = f"[待核验]({markdown_path(direct)})" if direct else "—"
        title = record["title"].replace("|", r"\|").replace("\n", " ")
        lines.append(
            f"| {record['problemNo']} | {title} | [查看题面]({statement}) | {complete_link} | {direct_link} | `{record['language']}` | `{record['public']['status']}` |"
        )
    lines.extend(
        [
            "",
            "## 后续门禁",
            "",
            "1. 逐题清理注释、变量名和元数据，并保留必要的简洁注释。",
            "2. 依据题目实际提交框核验 C++17、Python 3.14 及特殊提交形态；完整代码与可直接粘贴的分块代码分别保留。",
            "3. 在非维护窗口使用独立测试环境提交验证；只有在线显示 Accepted 且题面/源码/哈希审计通过，才把状态提升为 `PUBLIC_READY`。",
            "4. 由构建脚本重新生成 README 和 `data/problems.json`，再进行敏感信息扫描、差异审查和 GitHub PR。",
            "",
            "后台操作方案见被 `.gitignore` 排除的 `method/`，原始抓取材料见 `代码库/`。",
            "",
        ]
    )
    return "\n".join(lines)


def build() -> int:
    parser = argparse.ArgumentParser(description="生成 RUC YOJ 离线初步仓库结构")
    parser.add_argument("--check", action="store_true", help="只检查输入与可转换性，不写入生成文件")
    parser.add_argument("--no-download-assets", action="store_true", help="不下载题面识别出的附件资源")
    args = parser.parse_args()

    if not MANIFEST_PATH.is_file():
        print(f"找不到抓取清单: {MANIFEST_PATH}", file=sys.stderr)
        return 2
    manifest = json_load(MANIFEST_PATH)
    entries = manifest.get("problems") or []
    records: list[dict[str, Any]] = []
    statement_texts: list[tuple[Path, str]] = []
    failures: list[str] = []
    hash_mismatches = 0

    for entry in sorted(entries, key=get_problem_no):
        folder = str(entry.get("folder") or "").strip()
        raw_dir = RAW_ROOT / folder
        metadata_ref = str((entry.get("files") or {}).get("metadata") or "").strip()
        # The manifest stores paths relative to 代码库/, while metadata files
        # inside each problem directory store only the basename.
        metadata_path = RAW_ROOT / metadata_ref if metadata_ref else raw_dir / f"{folder.split('_', 1)[0]}_元数据.json"
        try:
            metadata = json_load(metadata_path)
            record, statement = build_statement(entry, metadata, raw_dir, download_assets=not args.check and not args.no_download_assets)
        except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
            failures.append(f"{folder}: {exc.__class__.__name__}: {exc}")
            continue
        records.append(record)
        statement_rel = Path(record["public"]["statement"])
        statement_texts.append((ROOT / statement_rel, statement))
        if not record["archive"]["sourceHashMatchesMetadata"]:
            hash_mismatches += 1

    if args.check:
        report = {
            "manifestProblems": len(entries),
            "recordsBuilt": len(records),
            "failures": failures,
            "hashMismatches": hash_mismatches,
            "formulaCount": sum(r["statement"]["formulaCount"] for r in records),
            "imageCount": sum(r["statement"]["imageCount"] for r in records),
            "assetCount": sum(len(r["statement"]["assets"]) for r in records),
        }
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 0 if not failures else 1

    for path, content in statement_texts:
        write_text(path, content)
    DATA_ROOT.mkdir(parents=True, exist_ok=True)
    write_json(
        DATA_ROOT / "problems.json",
        {
            "schemaVersion": SCHEMA_VERSION,
            "purpose": "offline initial build; not yet public-ready",
            "generatedFrom": relative_path(MANIFEST_PATH),
            "capturedAt": manifest.get("capturedAt"),
            "generatedRecords": len(records),
            "records": records,
        },
    )
    write_json(
        DATA_ROOT / "build-report.json",
        {
            "schemaVersion": SCHEMA_VERSION,
            "manifestProblems": len(entries),
            "recordsBuilt": len(records),
            "failures": failures,
            "hashMismatches": hash_mismatches,
            "formulaCount": sum(r["statement"]["formulaCount"] for r in records),
            "imageCount": sum(r["statement"]["imageCount"] for r in records),
            "assetCount": sum(len(r["statement"]["assets"]) for r in records),
            "publicReady": 0,
            "notes": [
                "题面来自 p_content，不公开原始 HTML。",
                "公式转换为尽量可读的 LaTeX；含 warnings 的题目必须人工复核。",
                "图片暂保留为原站 Markdown 链接，后续可在非维护窗口按哈希缓存本地附件。",
                "题面附件在发现后下载到对应题目目录的 assets/；失败项保留原始链接并记录 warning。",
            ],
        },
    )
    write_text(ROOT / "README.md", make_readme(records, manifest))

    print(
        json.dumps(
            {
                "manifestProblems": len(entries),
                "recordsBuilt": len(records),
                "failures": failures,
                "hashMismatches": hash_mismatches,
                "formulaCount": sum(r["statement"]["formulaCount"] for r in records),
                "imageCount": sum(r["statement"]["imageCount"] for r in records),
                "assetCount": sum(len(r["statement"]["assets"]) for r in records),
                "publicReady": 0,
            },
            ensure_ascii=False,
            indent=2,
        )
    )
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(build())
