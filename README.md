# Qt 内网聊天原型

该仓库包含一个基于 Qt 5.15 的局域网聊天工具骨架，实现了设备发现、联系人目录、消息传输及基础桌面 UI 的核心类结构，可用于复刻「内网通」的核心能力。

## 目录结构
- `docs/design_outline.md`：详细的模块划分、协议与迭代计划。
- `src/core`：业务与网络核心，包括 `DiscoveryService`, `PeerDirectory`, `MessageRouter`, `ChatController`。
- `src/ui`：Qt Widgets 主窗口骨架，展示联系人列表和消息面板。
- `CMakeLists.txt`：使用 CMake 构建，启用 AUTOMOC/AUTOUIC/AUTORCC，适配 Qt 5.15。

核心逻辑与大纲的映射：
- **发现层**：`DiscoveryService` 负责 UDP 广播/心跳，与大纲中的模块 1 对应，支持自定义网段和探测接口。
- **目录层**：`PeerDirectory` 派生自 `QAbstractListModel` 并提供 UI 绑定，与模块 2 匹配。
- **消息层**：`MessageRouter` 使用 `QTcpServer/QTcpSocket` 建立点对点连接，映射模块 3。
- **控制层**：`ChatController` 聚合网络能力、配置和 UI 交互，是大纲中的中心编排模块 4，并留有扩展到存储/文件的挂钩。
- **界面层**：`MainWindow` 提供联系人选择、聊天和状态提示，承接模块 6 的基础界面。

## 构建与运行
```bash
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/msvc2019_64
cmake --build build
./build/nwt
```
根据本地 Qt 安装调整 `CMAKE_PREFIX_PATH`。Windows 上可使用 `-G "Ninja"` 或默认 Visual Studio 生成器。

## 运行与操作
- 首次启动会在可执行文件同级目录生成 `lan_chat_settings.json`，其中保存本机 ID、昵称、监听端口与自定义网段；后续启动直接复用，不依赖注册表。
- 默认状态下，`DiscoveryService` 会自动遍历当前启用的网卡广播地址，因此同一网段的客户端无需任何配置即可互相发现并出现在联系人列表中。
- 若要跨不同子网通信，点击主界面左侧的“添加网段”，输入 `CIDR`（示例 `10.10.20.0/24`）。应用会立即向该网段广播自身并写入配置文件，重启后仍会继续探测这些网段；如果只输入单个 IPv4 地址（如 `192.168.0.2`），软件会默认按 `/24` 处理，该行为同样会记录到配置中。
- 选中联系人后，在右侧输入框键入消息并按 Enter/点击“发送”即可走 TCP 点对点通道完成聊天，收到的消息实时展示在上方日志面板。

## 下一步建议
1. 在 `DiscoveryService` 中实现广播包签名、离线检测与错误上报，减少假阳性。
2. 为 `MessageRouter` 引入协议帧（长度前缀/CBOR）和文件传输命令，完善重试与 ack。
3. 新增 `StorageService`（SQLite）并在 `ChatController` 中持久化聊天记录、会话与网段配置。
4. 优化 UI：联系人在线状态、消息气泡、网段管理对话框及多会话窗口。
5. 编写自动化测试（Qt Test 或模拟器）覆盖发现、目录同步和消息收发逻辑。
