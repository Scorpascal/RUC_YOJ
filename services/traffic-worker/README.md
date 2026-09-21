# RUC YOJ 实时访问统计 Worker

GitHub Pages 只负责静态文件，不能直接保存共享计数。这个小型 Cloudflare Worker 使用 D1 保存两类数据：累计页面加载数，以及北京时间逐日页面加载数。页面首次加载时向 `POST /api/visit` 写入一次并立即读取同一事务中的七日快照；`GET /api/stats` 只读，用于写入响应丢失时的恢复读取。

## 首次部署

需要一个可用的 Cloudflare 账号，并在本地完成 `wrangler login`。命令应在本目录执行：

```bash
npx wrangler d1 create ruc-yoj-traffic
# 将返回的 database_id 写入 wrangler.toml
npx wrangler d1 execute ruc-yoj-traffic --remote --file=./schema.sql
npx wrangler deploy
```

部署后先检查：

```bash
curl https://<worker-subdomain>.workers.dev/health
```

然后把 Worker 根地址写入公开配置 `docs/data/traffic-config.json`，将 `mode` 从 `fallback` 改为 `realtime`：

```json
{
  "schemaVersion": 1,
  "mode": "realtime",
  "apiBaseUrl": "https://<worker-subdomain>.workers.dev",
  "fallbackUrl": "data/traffic.json"
}
```

`ALLOWED_ORIGIN` 默认只允许 `https://scorpascal.github.io`。如果绑定了自定义 Pages 域名，应在 `wrangler.toml` 中用逗号分隔追加来源。不要把 Cloudflare token、D1 密钥或个人凭据写入仓库。

## 数据契约与一致性

- `POST /api/visit`：仅允许 Pages 来源；一次页面加载只发送一次，不自动重试 POST，避免网络响应丢失时重复计数。
- `GET /api/stats`：只读当前累计数和连续七日数据。
- D1 的累计数和当日数在同一个 batch 中更新，并在同一个 batch 中读取，返回的 `todayVisits`、`totalVisits` 和趋势数组属于同一快照。
- 统计口径是 `page_loads`，不是去重访客人数；不保存 IP、Cookie 或用户账号。
- 前端实时请求失败时先回读 `GET /api/stats`，仍失败才显示仓库内 `docs/data/traffic.json` 静态快照。静态回退不会伪造新访问。

## 本地验证

```bash
node --test test/worker.test.mjs
```

仓库根目录的 Pages 门禁仍会验证静态回退 JSON；它不再通过 GitHub Actions 定时刷新访问数据。生产环境建议在 Cloudflare 控制台为 Worker 配置请求速率限制，防止异常脚本刷量。
