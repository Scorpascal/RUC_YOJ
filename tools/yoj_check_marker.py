"""Public README check date, independent of the archive content-change date."""
import re
from datetime import date

PREFIX = "最近完整检查日期（北京时间）："
LINE = re.compile(r"^最近完整检查日期（北京时间）：.*$", re.M)


def check_date_line(readme: str) -> str:
    match = re.search(r"^最近完整检查日期（北京时间）：`(\d{4}-\d{2}-\d{2})`$", readme, re.M)
    if match:
        try:
            return PREFIX + f"`{date.fromisoformat(match[1]).isoformat()}`"
        except ValueError:
            pass
    return PREFIX + "尚无完整成功记录"


def with_check_date(readme: str, completed: date) -> str:
    line = PREFIX + f"`{completed.isoformat()}`"
    if LINE.search(readme):
        return LINE.sub(lambda _: line, readme, count=1)
    title, separator, rest = readme.partition("\n")
    return title + "\n\n" + line + "\n" + separator + rest
