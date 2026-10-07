# Ubuntu 26.04 兼容验证记录

验证日期：2026-10-08（Asia/Shanghai；Actions 时间为 2026-10-07 UTC）。

结论：本轮在真实 GitHub-hosted `ubuntu-24.04` 和 `ubuntu-26.04` 上，使用镜像自带 `/usr/bin/python3`，证明 nightly-health 的 regression、历史回执 check 路径及 Pages 的校验、目录生成、打包上传兼容。没有发现真实 OS 不兼容；只新增验证设施和本报告，不需要固定生产作业到 24.04。没有执行生产 deploy-pages，不能据此声称完整部署已验证。

## 基准、提交与真实运行

- 重新 fetch 后 `origin/main`：`8f3025a073a11289e0c4976a6ee7e2fdbc7c1fa2`，仍与历史审计基准一致；没有已有同任务分支或 PR，也没有适用 AGENTS.md / CONTRIBUTING 文件。
- 原工作树干净，在独立 worktree 建立 `codex/ubuntu-2604-compat`，保留原 `work` 分支。没有修改题目、题解、发布清单、站点数据或历史回执。
- 实际验证的代码提交：[ceb0cbafd86ccd6dde9584cb8f686dac5d37e9b3](https://github.com/Scorpascal/RUC_YOJ/commit/ceb0cbafd86ccd6dde9584cb8f686dac5d37e9b3)。后续提交仅增加本报告，改变 HEAD，但不改变被验证的代码或工作流。
- [真实 Actions run 37665213637](https://github.com/Scorpascal/RUC_YOJ/actions/runs/37665213637)：同一候选 SHA，6 个作业全部 success，没有重跑、setup-python 或额外 Python 依赖安装。
- [PR #1](https://github.com/Scorpascal/RUC_YOJ/pull/1)：文档提交后的 HEAD 也由测试分支 push 自动运行同一双镜像验证；其最新运行结果和 SHA 以 PR 中的验证链接为准。

## 真实 runner 结果

| 项目 | ubuntu-24.04 | ubuntu-26.04 |
| --- | --- | --- |
| OS | Ubuntu 24.04.5 LTS | Ubuntu 26.04.1 LTS |
| ImageOS / ImageVersion | ubuntu24 / 20260927.320.1 | ubuntu26 / 20260927.149.1 |
| Python 路径 / 版本 | /usr/bin/python3 / 3.12.3 | /usr/bin/python3 / 3.14.4 |
| Python OpenSSL | 3.0.13（30 Jan 2024） | 3.5.5（27 Jan 2026） |
| Git / curl / GNU tar | 2.55.0 / 8.5.0 / 1.35 | 2.55.0 / 8.18.0 / 1.35 |
| tzdata | 2026c-0ubuntu0.24.04.1 | 2026c-0ubuntu0.26.04.1 |
| Asia/Shanghai / 文件系统编码 | 正确加载 +08:00 / utf-8 | 正确加载 +08:00 / utf-8 |
| unittest discover | 124 项，0 跳过，exit 0 | 124 项，0 跳过，exit 0 |
| catalog --check | 509 项，492 verified，exit 0 | 509 项，492 verified，exit 0 |
| traffic --check | 通过，exit 0 | 通过，exit 0 |
| audit_consistency | failures=[]，exit 0 | failures=[]，exit 0 |
| 先检查、后生成 catalog | git diff 无漂移，exit 0 | git diff 无漂移，exit 0 |
| configure-pages@v6 | enablement=false，只读查询成功 | enablement=false，只读查询成功 |
| upload-pages-artifact@v5 | 打包、上传、下载通过 | 打包、上传、下载通过 |
| 下载后的站点文件核验 | 6 个文件，全部 SHA256 一致 | 6 个文件，全部 SHA256 一致 |
| 原始在线回执 CLI | exit 0，正常回执 | exit 0，正常回执 |
| 同一固定回执快照 | 状态、摘要、exit 0 一致 | 状态、摘要、exit 0 一致 |
| Python 原生 HTTPS | 验证证书的匿名 API 请求通过 | 验证证书的匿名 API 请求通过 |
| 现有真实 curl 回退 | 1 次真实 curl 请求通过 | 1 次真实 curl 请求通过 |

Python 默认 CA 路径均为 `/usr/lib/ssl/cert.pem` 与 `/usr/lib/ssl/certs`。各 runner 的完整必要诊断、各检查日志、独立步骤 outcome、站点哈希保存在 `ubuntu-compat-evidence-*` artifact。日志中的模拟登录/提交输出来自现有测试替身，没有真实登录 YOJ 或提交题目；缺少个人词表的 warning 两边相同，测试未跳过。

Pages artifact 名称分别为 `ubuntu-compat-pages-ubuntu-24.04`（首轮 ID 11503475050）和 `ubuntu-compat-pages-ubuntu-26.04`（首轮 ID 11502223462）。已通过 API 确认存在且未过期，再由每个 runner 下载自己的 artifact。验证 GNU tar 可读取，并逐文件比较整个可发布文件清单和 SHA256，包含 `index.html`、`yoj-quick-submit.html`、catalog、quick-submit、traffic、traffic-config。两边文件哈希完全一致，catalog SHA256 为 `fa1a4f8ce6cd2f63b4ff5fa4017be949298d0f0238171f90a5121e8d2bc3c494`。Pages 包保留 1 天，其他诊断保留 7 天；过期后以本报告和 Actions 日志为长期摘要证据。

## 回执与联网证据

自动选择已完成、格式有效且能覆盖正常路径的 `cycle_date=2026-10-07`。固定下载快照来自回执分支 `codex/yoj-health` 的 SHA `c9ba2b8bf20cd6b0247eedfc07380406457b6a98`，文件 blob SHA `feff821e0a22374acd48db60a774426197c3d4a0`；冻结观察时间为 `2026-10-08T02:13:58+08:00`。

两个 runner 都执行原始 `yoj_health.py check --cycle-date 2026-10-07 --summary ...`，且输出写入 GITHUB_STEP_SUMMARY。两边运行前后在线回执分支 SHA 均稳定且相同，因此在线结果可以直接比较：`scheduleState=ON_TIME`、`runState=RETURNED_OK`、`sourceState=AVAILABLE`、`syncState=SUCCEEDED`、`reason=NONE`、最后成功同步 `2026-10-07T23:34:17+08:00`，摘要完全相同，退出码均 0。

固定快照验证在隔离入口仅替换 public_json 和观察时间，执行原始 CLI main，严格比较期望结果、摘要和退出码。这提供确定性的 OS 语义对照，并不代表联网成功。联网成功另外由原始 CLI、无回退的原生 urllib 请求、强制 urllib 首次失败后进入现有 public_json 的真实 curl 分支证明；curl 本身没有替身。请求匿名、只读、有超时与大小限制，保留原有 HTTPS 协议、证书校验与最多三次尝试边界。任一诊断失败都会保留结果并导致观察/汇总作业非零；没有 `continue-on-error` 或 `|| true`。

## 实际依赖范围

- `nightly-health.yml` regression 真正执行 unittest；check-receipt 只读调用 `yoj_health.py check`。ZoneInfo 在 health/scheduler/guard 等模块导入时加载，来自系统 tzdata。
- 现有测试真实执行 fcntl 文件锁、fsync/os.replace 原子写入、symlink/FIFO 隔离拒绝，以及临时 bare 仓库、本地 main 的 Git publish。Git 正常路径真实使用 Popen 独立进程组和 communicate 超时参数；超时后的 killpg 分支只由 mock 覆盖，本轮没有进行真实杀进程组实验。
- Pages 真实执行 UTF-8 JSON、中文题解路径读取、文件 SHA256 与发布清单链一致性检查。目录生成只写 catalog 并同步 quick-submit，不运行 build_initial 或题库重建。
- `build_traffic.py --check` 只验证本地数据，不访问统计服务；certifi 可选。C/C++ 编译与样例执行属于其他业务路径，在当前测试中为替身，不是这两个工作流的真实依赖；没有新增编译器或 lxml 安装。
- checkout@v5、configure-pages@v6 使用 runner 内嵌 Node24，成功执行已获真实证明；不能用系统 node 版本替代该证据。configure-pages 只有该作业增加 `pages: read`，显式禁止 enablement；其他权限为 contents:read，所有 checkout 关闭 persist-credentials。
- upload-pages-artifact@v5 在 Linux 执行 GNU tar 的 dereference/hard-dereference 打包，再通过内置固定 SHA 的 upload-artifact v7 上传。path=docs、默认隐藏文件排除及 1 天保留期与生产一致，只使用独立 artifact 名称。

## 官方资料与问题归因

已核对 [官方迁移公告 #14748](https://github.com/actions/runner-images/issues/14748)：26.04 已 GA，可显式使用；ubuntu-latest 计划于 2026-10-19 开始迁移，2026-11-19 完成。镜像文档快照基于 runner-images `e7c7cb8f4227797c6404a4e98c2ad463c2f70f91`：[24.04 软件清单](https://github.com/actions/runner-images/blob/e7c7cb8f4227797c6404a4e98c2ad463c2f70f91/images/ubuntu/Ubuntu2404-Readme.md)、[26.04 软件清单](https://github.com/actions/runner-images/blob/e7c7cb8f4227797c6404a4e98c2ad463c2f70f91/images/ubuntu/Ubuntu2604-Readme.md)。文档系统 Python 与实测一致；缓存 Python 的列表不等于 PATH 默认版本。

实测使用的生产 Actions 版本解析为 checkout `fbc6f3992d24b796d5a048ff273f7fcc4a7b6c09`、configure-pages `45bfe0192ca1faeb007ade9deae92b16b8254a0d`、upload-pages-artifact `fc324d3547104276b827a68afc52ff2a11cc49c9`；上传内部固定 upload-artifact `bbbca2ddaa5d8feaa63e36b76fdaad77386f024f`（v7.0.0）。

没有生产兼容缺陷，未添加兼容性修复或跳过测试。实现阶段为验证工作流明确配置 `shell: bash`，确保保存日志的 tee 管道启用 pipefail，Python 失败不会被掩盖；选择回执时跳过尚未截止的周期。actionlint 1.7.12 的内置标签表尚未包含 ubuntu-26.04，静态检查仅豁免这条已由官方公告及真实运行证明的标签告警，其余检查通过。

本地重新运行 Python 3.12.14 的 124 项 unittest 和三项 Pages 校验通过，重生成无漂移；本地真实 curl 回退探针失败并严格返回非零。该开发环境网络要求继承代理，而生产 clean_environment 不向 curl 子进程传递代理变量；真实 hosted runners 的相同原始 curl 路径两边都通过，因此没有为开发环境修改生产网络策略。首次推送被 GH007 私有邮箱保护拒绝，尚未发布的提交改用 GitHub noreply 后普通推送成功，没有强推。

此前 main 的 [夜间失败 run 37527661840](https://github.com/Scorpascal/RUC_YOJ/actions/runs/37527661840) 发生于 24.04：2026-10-05 `NO_RECEIPT` 导致退出 1；2026-10-06 虽 `ON_TIME`，业务执行非零且 `YOJ_CAMPUS_ACCESS_REQUIRED`。这是缺回执/外部访问状态，不是迁移造成的 OS 不兼容，未修改历史证据或原始健康检查门禁。

## 未覆盖内容与撤销

没有执行 deploy-pages@v5（Node24、OIDC、Pages 创建部署/轮询的独立路径），也没有验证新部署后的在线站点。没有运行 initialize/flush/replay、推送真实回执、抓取题库、在线提交、全题库编译或统计服务抓取。历史 SUCCEEDED 是已有回执证据，不证明本次新同步或新部署。未来镜像版本、网络故障或业务状态变化仍需重新运行，不能永久外推本轮结果。

生产两个工作流原样保留 ubuntu-latest，不建议固定到 24.04。验证工作流使用独立 concurrency、不取消生产 Pages/夜间作业，仅本测试分支 push 或手动触发，无生产部署权限。

未合并时关闭 PR、删除测试分支即可撤销远端验证设施，main 从未变更；合并后 revert 本任务验证设施及报告提交（或删除新增工作流、两个 helper 与 maintenance 报告）。无需回滚生产 runner、题库数据或回执。没有自动合并 PR。
