# 数据格式与同步协议

终端与 PC 端通过 **TCP + JSON Lines** 通信：一行一条 JSON，以 `\n` 分隔，单条不超过 64 KB。
所有多字节整数小端；时间戳为 Unix 秒（UTC）；温度以 0.01 ℃ 存整数。

---

## 1. 分钟记录（16 字节）

| 字段 | 类型 | 偏移 | 说明 |
| --- | --- | --- | --- |
| `ts` | uint32 | 0 | Unix 秒 |
| `steps` | uint16 | 4 | 该分钟步数 |
| `hr` | uint8 | 6 | 心率 bpm，0 无效 |
| `spo2` | uint8 | 7 | 血氧 %，0 无效 |
| `tempX100` | int16 | 8 | 体温 ×100，如 3650 |
| `sitMin` | uint8 | 10 | 连续久坐分钟 |
| `flags` | uint8 | 11 | bit0 佩戴，bit1 预留，bit2 跌倒 |
| `reserved` | uint32 | 12 | 填 0 |

C++ 定义见 `components/hb_common/include/common/types.h`，有 `static_assert(sizeof == 16)` 与偏移断言。

---

## 2. 消息信封

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `v` | int | 协议版本，当前 1 |
| `type` | string | `AUTH` / `SYNC` / `ACK` / `PING` / `ALERT` |
| `seq` | uint32 | 客户端单调递增 |
| `deviceId` | string | 如 `HB-2026-G03-01` |
| `ts` | int64 | Unix 秒 |
| `payload` | object | 按 type 定义 |
| `code` | int | 0 成功 |

### AUTH

```json
{"v":1,"type":"AUTH","seq":1,"ts":1761091200,"deviceId":"HB-2026-G03-01",
 "payload":{"fw":"1.0.0","token":"<32 hex>"}}
```

### SYNC

```json
{"v":1,"type":"SYNC","seq":2,"ts":1761091260,"deviceId":"HB-2026-G03-01",
 "payload":{"fromCursor":10240,"records":[
   {"ts":1761091200,"steps":62,"hr":78,"spo2":97,"temp":3650,"sit":45,"flags":1},
   {"ts":1761091260,"steps":0,"hr":76,"spo2":97,"temp":3648,"sit":46,"flags":1}]}}
```

### ACK

```json
{"v":1,"type":"ACK","seq":3,"ts":1761091261,"deviceId":"HB-2026-G03-01","code":0,
 "payload":{"inserted":2,"duplicated":0,"nextCursor":10242}}
```

---

## 3. 幂等与游标

- 服务端以 `(deviceId, ts)` 唯一键，`INSERT OR IGNORE` 去重；
- 客户端**先写记录、后写游标**；
- ACK 丢失时客户端会重发旧游标，服务端必须正常接受并去重；
- 失败按 1、2、4、8 秒指数退避。

---

## 4. 错误码

| 码 | 含义 | 客户端处理 |
| --- | --- | --- |
| 0 | 成功 | 更新游标 |
| 1001 | 设备未注册 | 提示联系教师，停止重试 |
| 1002 | token 无效 | 清除本地 token，重新配置 |
| 1003 | 协议版本不支持 | 升级固件 |
| 2001 | 字段缺失/类型错 | 丢弃该批并记日志 |
| 2002 | 时间戳越界 | 校时后重发 |
| 3001 | 数据库忙 | 指数退避 |
| 3002 | 磁盘空间不足 | 告警并停止同步 |
| 4001 | 消息超过 64 KB | 拆批重发 |
