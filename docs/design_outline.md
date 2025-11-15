# 局域网聊天工具实现大纲

## 目标
- 在无中心服务器的局域网内完成设备发现、状态同步、文本消息和文件传输的基础能力。
- 使用 Qt 5.15 (Core/Network/Widgets) 构建跨平台桌面客户端。
- 模块化拆分网络、状态、持久化和界面，方便后续扩展音视频、远程控制等功能。

## 核心模块
1. **DiscoveryService (QUdpSocket)**
   - 在指定网段发送 UDP 广播/组播的在线宣告和心跳 (包含用户 ID、昵称、监听端口、功能列表)。
   - 监听同一端口接收来自其他客户端的宣告，解析后投递给目录模块。
   - 提供 `announceOnline()`, `probeSubnet(const QHostAddress &base, int maskBits)`, `handleDatagram()` 等接口。

2. **PeerDirectory (QObject + QAbstractListModel)**
   - 维护当前可达的客户端清单、状态、超时时间。
   - 向 UI 提供模型接口，向消息路由提供连接参数。
   - 负责冲突处理（多网卡、同 ID 多实例），暴露 `upsertPeer(PeerInfo)`、`peerTimeout()`。

3. **MessageRouter (QTcpServer/QTcpSocket)**
   - 对每个会话建立可靠的点对点 TCP 连接；可选地对小消息使用 UDP。
   - 提供 `sendChatMessage(peerId, payload)`、`startListener(quint16 port)` 等接口，并向控制层发出消息信号。
   - 规划统一的 JSON/CBOR 报文格式以扩展文件、确认、回执。

4. **ChatController (QObject)**
   - 将 UI 动作转成业务调用：创建会话、发送文字/文件、更新网段配置。
   - 订阅 Discovery/Router 信号；聚合消息并写入存储。
   - 负责配置与 `QSettings` 交互（用户资料、默认端口、允许的网段列表）。

5. **StorageService (QtSql/SQLite)**
   - 记录会话、历史消息、文件元数据；提供分页查询。
   - 后续可扩展漫游或备份。

6. **UI 层 (Qt Widgets / QML)**
   - 主窗口展示联系人列表（绑定 PeerDirectory 模型）、当前聊天、网络状态提示。
   - 提供“添加网段”对话框：验证输入的 CIDR，回传给控制器更新发现范围。

## 网络协议
- 默认 UDP 广播端口：`45454`，消息采用 JSON 序列（type、senderId、displayName、listenPort、capabilities、timestamp）。
- 发现流程：
  1. 启动时读取网段列表，对每个广播地址发送 `hello`。
  2. 设备收到后立即 `hello_ack`，并加入目录；心跳每 15s 发送一次。
  3. 超过 45s 未收到心跳则判定离线。
- 聊天流程：
  1. 用户选中对方，控制器向 MessageRouter 请求会话；必要时发起 TCP 连接。
  2. 双方以 JSON 消息传递 `chat`, `ack`, `file_offer` 等类型。
  3. 文件传输可复用 TCP 连接或单独开启 `QTcpSocket`/`QUdpSocket` 通道。

## 数据结构
- `PeerInfo`：`id`, `displayName`, `address`, `listenPort`, `lastSeen`, `capabilities`。
- `ChatMessage`：`id`, `peerId`, `direction`, `payload`, `timestamp`, `deliveryState`。
- `Settings`：`localId`, `nickname`, `listenPort`, `subnets (QVector<CidrBlock>)`。

## 配置与安全
- 启动生成/读取唯一 ID，存储在与可执行文件同目录的 JSON 配置（`lan_chat_settings.json`），方便免安装分发。
- UI 支持用户直接输入 CIDR 或单个 IP，若缺失掩码则为 IPv4 默认 `/24`、IPv6 默认 `/64`，并在保存前规范化网段。
- 可选对消息进行对称加密（预留字段 `encryption`）。
- 文件传输前进行 SHA-256 校验以防损坏。

## 开发阶段建议
1. **阶段 1：网络骨架**
   - 搭建 DiscoveryService 和 PeerDirectory，能看到实时在线列表。
2. **阶段 2：聊天能力**
   - MessageRouter + 简易 UI，支持文本对话，测试多客户端。
3. **阶段 3：可靠性与存储**
   - 引入 StorageService、消息重发、断线恢复。
4. **阶段 4：文件传输与高级功能**
   - 添加文件流控、日志、管理员工具等。

## 测试要点
- 同网段、多网段（子网掩码不同）的发现互通性。
- 网络异常（断网、IP 冲突、重复 ID）。
- 大量在线客户端下的广播风暴和 UI 性能。
- 多平台（Windows/Linux/macOS）构建一致性。
