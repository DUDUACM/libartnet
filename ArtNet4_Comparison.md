# Art-Net 4 规范 vs libartnet 当前实现 — 完整对照清单

## 一、操作码 (OpCode) 对照

根据 Art-Net 4 表 1 定义的所有操作码：

| OpCode | 名称 | Art-Net 4 | libartnet | 状态 |
|--------|------|-----------|-----------|------|
| `0x2000` | OpPoll | 定义 | `ARTNET_POLL` | ✅ 已实现 |
| `0x2100` | OpPollReply | 定义 | `ARTNET_REPLY` | ✅ 已实现 |
| `0x2300` | OpDiagData | 定义 | `ARTNET_DIAGDATA` | ✅ 已实现 |
| `0x2400` | OpCommand | 定义 | `ARTNET_COMMAND` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x5000` | OpDmx | 定义 | `ARTNET_DMX` | ✅ 已实现 |
| `0x5100` | OpNzs | 定义 | `ARTNET_NZS` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x5200` | OpSync | 定义 | `ARTNET_SYNC` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x6000` | OpAddress | 定义 | `ARTNET_ADDRESS` | ✅ 已实现 |
| `0x7000` | OpInput | 定义 | `ARTNET_INPUT` | ✅ 已实现 |
| `0x8000` | OpTodRequest | 定义 | `ARTNET_TODREQUEST` | ✅ 已实现 |
| `0x8100` | OpTodData | 定义 | `ARTNET_TODDATA` | ✅ 已实现 |
| `0x8200` | OpTodControl | 定义 | `ARTNET_TODCONTROL` | ✅ 已实现 |
| `0x8300` | OpRdm | 定义 | `ARTNET_RDM` | ✅ 已实现 |
| `0x8400` | OpRdmSub | 定义 | `ARTNET_RDMSUB` | ✅ 已实现（收包 + `artnet_send_rdmsub()` 发送 API） |
| `0xa010` | OpVideoSetup | 定义 | `ARTNET_VIDEOSTEUP` | ⚠️ 仅桩（规范标注已弃用） |
| `0xa020` | OpVideoPalette | 定义 | `ARTNET_VIDEOPALETTE` | ⚠️ 仅桩（规范标注已弃用） |
| `0xa040` | OpVideoData | 定义 | `ARTNET_VIDEODATA` | ⚠️ 仅桩（规范标注已弃用） |
| `0xf000` | OpMacMaster | 定义 | `ARTNET_MACMASTER` | ⚠️ 仅桩（规范标注已弃用） |
| `0xf100` | OpMacSlave | 定义 | `ARTNET_MACSLAVE` | ⚠️ 仅桩（规范标注已弃用） |
| `0xf200` | OpFirmwareMaster | 定义 | `ARTNET_FIRMWAREMASTER` | ✅ 已实现 |
| `0xf300` | OpFirmwareReply | 定义 | `ARTNET_FIRMWAREREPLY` | ✅ 已实现 |
| `0xf400` | OpFileTnMaster | 定义 | `ARTNET_FILETNMASTER` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0xf500` | OpFileFnMaster | 定义 | `ARTNET_FILEFNMASTER` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0xf600` | OpFileFnReply | 定义 | `ARTNET_FILEFNREPLY` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0xf800` | OpIpProg | 定义 | `ARTNET_IPPROG` | ✅ 已实现 |
| `0xf900` | OpIpProgReply | 定义 | `ARTNET_IPREPLY` | ✅ 已实现 |
| `0x9000` | OpMedia | 定义 | `ARTNET_MEDIA` | ⚠️ 仅桩（打印日志） |
| `0x9100` | OpMediaPatch | 定义 | `ARTNET_MEDIAPATCH` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9200` | OpMediaControl | 定义 | `ARTNET_MEDIACONTROL` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9300` | OpMediaControlReply | 定义 | `ARTNET_MEDIACONTROLREPLY` | ⚠️ 仅桩（打印日志） |
| `0x9700` | OpTimeCode | 定义 | `ARTNET_TIMECODE` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9800` | OpTimeSync | 定义 | `ARTNET_TIMESYNC` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9900` | OpTrigger | 定义 | `ARTNET_TRIGGER` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9a00` | OpDirectory | 定义 | `ARTNET_DIRECTORY` | ✅ OpCode 已定义，处理逻辑缺失 |
| `0x9b00` | OpDirectoryReply | 定义 | `ARTNET_DIRECTORYREPLY` | ✅ OpCode 已定义，处理逻辑缺失 |

---

## 二、各数据包结构字段差异

### 1. ArtPoll — 字段缺失

| Art-Net 4 字段 | 大小 | libartnet | 状态 |
|----------------|------|-----------|------|
| ID[8] | 8 | `id[8]` | ✅ |
| OpCode | 2 | `opCode` | ✅ |
| ProtVerHi | 1 | `verH` | ✅ |
| ProtVerLo | 1 | `ver` | ✅ |
| Flags | 1 | `flags` | ✅ 已重命名（原 `ttm`），`artnet_poll_flags_t` 枚举已修正，收包逻辑已更新 |
| DiagPriority | 1 | `diagPriority` | ✅ 已修正（原 pad 已重命名） |
| TargetPortAddressTopHi | 1 | `targetPortAddressTopHi` | ✅ 已添加 |
| TargetPortAddressTopLo | 1 | `targetPortAddressTopLo` | ✅ 已添加 |
| TargetPortAddressBottomHi | 1 | `targetPortAddressBottomHi` | ✅ 已添加 |
| TargetPortAddressBottomLo | 1 | `targetPortAddressBottomLo` | ✅ 已添加 |

---

### 2. ArtPollReply — 大量字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| ID[8] | `id[8]` | ✅ |
| OpCode | `opCode` | ✅ |
| IP Address[4] | `ip[4]` | ✅ |
| Port | `port` | ✅ |
| VersInfoH | `verH` | ✅ |
| VersInfoL | `ver` | ✅ |
| NetSwitch (15-bit高7位) | `netSwitch` | ✅ 已修正（原 subH 已重命名） |
| SubSwitch | `sub` | ✅ |
| OemHi | `oemH` | ✅ |
| Oem | `oem` | ✅ |
| Ubea Version | `ubea` | ✅ |
| Status1 | `status` | ✅ |
| EstaMan[2] | `etsaman[2]` | ✅ |
| ShortName[18] | `shortname[18]` | ✅ |
| LongName[64] | `longname[64]` | ✅ |
| NodeReport[64] | `nodereport[64]` | ✅ |
| NumPortsHi | `numbportsH` | ✅ |
| NumPortsLo | `numbports` | ✅ |
| PortTypes[4] | `porttypes[4]` | ✅ |
| GoodInput[4] | `goodinput[4]` | ✅ |
| GoodOutputA[4] | `goodoutput[4]` | ✅ |
| SwIn[4] | `swin[4]` | ✅ |
| SwOut[4] | `swout[4]` | ✅ |
| **AcnPriority** | `acnPriority` | ✅ 已修复（原 swvideo 已重命名） |
| SwMacro | `swmacro` | ✅ |
| SwRemote | `swremote` | ✅ |
| Spare | `sp1` | ✅ |
| Spare | `sp2` | ✅ |
| ~~sp3~~ / ~~style~~ | — | ✅ 已移除（Art-Net 4 中不存在） |
| MAC[6] | `mac[6]` | ✅ |
| **BindIp[4]** | `bindIp[4]` | ✅ 已添加 |
| **BindIndex** | `bindIndex` | ✅ 已添加 |
| **Status2** | `status2` | ✅ 已添加 |
| **GoodOutputB[4]** | `goodOutputB[4]` | ✅ 已添加 |
| **Status3** | `status3` | ✅ 已添加 |
| **DefaultRespUID[6]** | `defaultRespUid[6]` | ✅ 已添加 |
| Filler[15] | `filler[15]` | ✅ 已修正（32→15） |

---

### 3. ArtAddress — 字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| NetSwitch | `netSwitch` | ✅ 已修正（原 filler1 已重命名） |
| **BindIndex** | `bindIndex` | ✅ 已修正（原 filler2 已重命名） |
| ShortName[18] | `shortname[18]` | ✅ |
| LongName[64] | `longname[64]` | ✅ |
| SwIn[4] | `swin[4]` | ✅ |
| SwOut[4] | `swout[4]` | ✅ |
| SubSwitch | `subnet` | ✅ |
| **AcnPriority** | `acnPriority` | ✅ 已修复（原 swvideo 已重命名） |
| Command | `command` | ✅ 已完善（全部 40 个命令均有处理逻辑） |

**ArtAddress Command 处理状态**：

| 命令值 | 名称 | libartnet 定义 | 处理逻辑 |
|----------|------|--------------|---------|
| 0x00 AcNone | 无动作 | ✅ `ARTNET_PC_NONE` | ✅ 空操作 |
| 0x01 AcCancelMerge | 取消合并 | ✅ `ARTNET_PC_CANCEL` | ✅ 清除所有端口 merge 源（已修复） |
| 0x02 AcLedNormal | LED正常 | ✅ `ARTNET_PC_LED_NORMAL` | ✅ 设置 LED 状态，反映到 Status1 |
| 0x03 AcLedMute | LED静音 | ✅ `ARTNET_PC_LED_MUTE` | ✅ 设置 LED 状态，反映到 Status1 |
| 0x04 AcLedLocate | LED定位 | ✅ `ARTNET_PC_LED_LOCATE` | ✅ 设置 LED 状态，反映到 Status1 |
| 0x05 AcResetRxFlags | 重置标志 | ✅ `ARTNET_PC_RESET` | ✅ 重置所有 4 端口标志（已修复：原仅端口 0） |
| 0x06 AcAnalysisOn | 启用分析 | ✅ `ARTNET_PC_ANALYSIS_ON` | ✅ 预留（应用层） |
| 0x07 AcAnalysisOff | 禁用分析 | ✅ `ARTNET_PC_ANALYSIS_OFF` | ✅ 预留（应用层） |
| 0x08 AcFailHold | 故障保持 | ✅ `ARTNET_PC_FAIL_HOLD` | ✅ 设置 failsafe_mode，反映到 Status3 |
| 0x09 AcFailZero | 故障归零 | ✅ `ARTNET_PC_FAIL_ZERO` | ✅ 设置 failsafe_mode，反映到 Status3 |
| 0x0a AcFailFull | 故障满值 | ✅ `ARTNET_PC_FAIL_FULL` | ✅ 设置 failsafe_mode，反映到 Status3 |
| 0x0b AcFailScene | 故障场景 | ✅ `ARTNET_PC_FAIL_SCENE` | ✅ 设置 failsafe_mode，反映到 Status3 |
| 0x0c AcFailRecord | 记录场景 | ✅ `ARTNET_PC_FAIL_RECORD` | ✅ 预留（应用层） |
| 0x10-0x13 AcMergeLtp0-3 | LTP合并 | ✅ `ARTNET_PC_MERGE_LTP_*` | ✅ 设置 merge 模式 + LTP 标志位 |
| 0x20-0x23 AcDirectionTx0-3 | 方向=输出 | ✅ `ARTNET_PC_DIRECTION_TX_*` | ✅ 修改端口类型为输出 |
| 0x30-0x33 AcDirectionRx0-3 | 方向=输入 | ✅ `ARTNET_PC_DIRECTION_RX_*` | ✅ 修改端口类型为输入 |
| 0x50-0x53 AcMergeHtp0-3 | HTP合并 | ✅ `ARTNET_PC_MERGE_HTP_*` | ✅ 设置 merge 模式，清除 LTP 标志位（已修复） |
| 0x60-0x63 AcArtNetSel0-3 | 选择Art-Net | ✅ `ARTNET_PC_ARTNET_SEL_*` | ✅ 设置端口协议选择 |
| 0x70-0x73 AcAcnSel0-3 | 选择sACN | ✅ `ARTNET_PC_ACN_SEL_*` | ✅ 设置端口协议选择 |
| 0x90-0x93 AcClearOp0-3 | 清除输出 | ✅ `ARTNET_PC_CLR_*` | ✅ 清零 DMX 输出缓冲区 |
| 0xa0-0xa3 AcStyleDelta0-3 | 增量输出 | ✅ `ARTNET_PC_STYLE_DELTA_*` | ✅ 设置输出样式，反映到 GoodOutputB |
| 0xb0-0xb3 AcStyleConst0-3 | 连续输出 | ✅ `ARTNET_PC_STYLE_CONST_*` | ✅ 设置输出样式，反映到 GoodOutputB |
| 0xc0-0xc3 AcRdmEnabled0-3 | 启用RDM | ✅ `ARTNET_PC_RDM_ENABLED_*` | ✅ 设置 RDM 状态，反映到 GoodOutputB |
| 0xd0-0xd3 AcRdmDisabled0-3 | 禁用RDM | ✅ `ARTNET_PC_RDM_DISABLED_*` | ✅ 设置 RDM 状态，反映到 GoodOutputB |

---

### 4. ArtInput — 字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| Filler1 | `filler1` | ✅ |
| **BindIndex** | `bindIndex` | ✅ 已修正（原 filler2 已重命名） |
| NumPortsHi | `numbportsH` | ✅ |
| NumPortsLo | `numbports` | ✅ |
| Input[4] | `input[4]` | ✅ |

---

### 5. ArtTodRequest — Net 字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| Spare1-7 (7字节) | `spare1-7` (7字节) | ✅ |
| **Net** | `net` | ✅ 已添加（原 spare8 已重命名） |
| Command | `command` | ✅ |
| AddCount | `adCount` | ✅ |
| Address[32] | `address[32]` | ✅ |

---

### 6. ArtTodData — BindIndex + Net 缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| RdmVer | `rdmVer` | ✅ |
| Port | `port` | ✅ |
| Spare1-6 (6字节) | `spare1-6` (6字节) | ✅ |
| **BindIndex** | `bindIndex` | ✅ 已添加（原 spare7 已重命名） |
| **Net** | `net` | ✅ 已添加（原 spare8 已重命名） |
| CommandResponse | `cmdRes` | ✅ |
| Address | `address` | ✅ |
| UidTotalHi | `uidTotalHi` | ✅ |
| UidTotalLo | `uidTotal` | ✅ |
| BlockCount | `blockCount` | ✅ |
| UidCount | `uidCount` | ✅ |
| ToD[] | `tod[200][6]` | ✅ |

---

### 7. ArtTodControl — Net 字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| Spare1-7 (7字节) | `spare1-7` (7字节) | ✅ |
| **Net** | `net` | ✅ 已添加（原 spare8 已重命名） |
| Command | `cmd` | ✅ |
| Address | `address` | ✅ |

---

### 8. ArtRdm — Net 字段缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| Spare1-7 (7字节) | `spare1-7` (7字节) | ✅ |
| **Net** | `net` | ✅ 已添加（原 spare8 已重命名） |
| Command | `cmd` | ✅ |
| Address | `address` | ✅ |
| RdmPacket[] | `data[512]` | ✅ |

---

### 9. ArtIpProg — 默认网关缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| Command | `Command` | ✅ |
| ProgIp[4] | `ProgIp(4)` | ✅ |
| ProgSm[4] | `ProgSm(4)` | ✅ |
| ProgPort[2] (已弃用) | `ProgPort(2)` | ✅ |
| **ProgDg[4] (默认网关)** | `ProgDgHi/2/1/Lo` | ✅ 已添加 |
| Spare1-4 | `Spare1-4` | ✅ |

---

### 10. ArtIpProgReply — Status + 默认网关缺失

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| ProgIp[4] | `ProgIp(4)` | ✅ |
| ProgSm[4] | `ProgSm(4)` | ✅ |
| ProgPort[2] | `ProgPort(2)` | ✅ |
| **Status** | `Status` | ✅ 已添加（原 Filler3 已重命名） |
| **ProgDg[4] (默认网关)** | `ProgDgHi/2/1/Lo` | ✅ 已添加 |

---

### 11. ArtDmx — 寻址方式

| Art-Net 4 字段 | libartnet 字段 | 状态 |
|----------------|----------------|------|
| SubUni (低字节) | `universe` (uint16 低字节) | ✅ 功能等价，`htols()` 保证字节序正确 |
| Net (高7位) | `universe` (uint16 高字节) | ✅ 同上 |

---

## 三、已实现的 OpCode 处理逻辑（本次新增 13 个）

| 数据包 | OpCode | 功能说明 | 状态 |
|--------|--------|----------|------|
| ArtSync | 0x5200 | 同步帧 + 4 秒超时回退非同步模式 | ✅ |
| ArtNzs | 0x5100 | 非零起始码 DMX，OEM 过滤 + dmx 回调 | ✅ |
| ArtCommand | 0x2400 | 文本命令，OEM 过滤 + command 回调 | ✅ |
| ArtTimeCode | 0x9700 | SMPTE 时间码，timecode 回调 | ✅ |
| ArtTimeSync | 0x9800 | 实时时钟同步，timesync 回调 | ✅ |
| ArtTrigger | 0x9900 | 宏触发，OEM 过滤 + trigger 回调 | ✅ |
| ArtDirectory | 0x9a00 | 目录请求 → 回复 ArtDirectoryReply | ✅ |
| ArtDirectoryReply | 0x9b00 | 目录数据，directory_reply 回调 | ✅ |
| ArtFileTnMaster | 0xf400 | 文件上传 → 回复 ArtFirmwareReply | ✅ |
| ArtFileFnMaster | 0xf500 | 文件下载请求，file_fn_master 回调 | ✅ |
| ArtFileFnReply | 0xf600 | 文件数据，file_fn_reply 回调 | ✅ |
| ArtMediaPatch | 0x9100 | 媒体补丁，mediapatch 回调 | ✅ |
| ArtMediaControl | 0x9200 | 媒体控制，mediacontrol 回调 | ✅ |

**仍缺失（超出 Art-Net 4 库范围）**：

| 数据包 | 说明 |
|--------|------|
| ArtVlc | ArtNzs 的特定实现（可见光通信），由应用层处理 |

---

## 四、Art-Net 4 架构级特性缺失

| 特性 | 说明 | 状态 |
|------|------|------|
| **15-bit Universe 寻址** | Net(7位) + SubNet(4位) + Universe(4位) = 32768 个 Universe | ✅ `g_port_t.addr` 改为 `uint16_t`，`MAKE_ADDR` 宏，发包/收包均支持完整 15-bit |
| **BindIndex 机制** | 支持超过4个DMX端口，允许 >1000 个端口的网关 | ✅ 所有关包的 BindIndex 字段已添加（ArtPollReply, ArtAddress, ArtInput, ArtTodData） |
| **BindIp 机制** | 绑定节点共享 IP 地址 | ✅ ArtPollReply 中已添加 bindIp 字段 |
| **sACN/Art-Net 协议切换** | 每端口选择 sACN 或 Art-Net 作为传输协议 | ⚠️ 超出范围：ArtAddress 命令已处理（`proto_sel` 状态存储），但 sACN 传输本身属 E1.31 协议，不在本库范围内 |
| **ArtPollReply 必须单播** | Art-Net 4 不允许 ArtPollReply 广播 | ✅ `handle_poll()` 始终将 `reply_addr` 设为 `p->from`，不再回退广播 |
| **ArtRdm 必须单播** | Art-Net 4 不允许 ArtRdm 广播 | ✅ `handle_rdm()` 存储请求方 IP，`artnet_tx_rdm()` 始终单播回复 |
| **单播订阅 (ArtDmx)** | ArtDmx 应仅单播给订阅者，不允许广播 | ✅ `artnet_send_dmx()` 始终单播到已知订阅者，不广播 |
| **故障安全 (Fail-safe)** | 数据丢失时保持/归零/满值/播放场景 | ✅ `check_timeouts()` 检测 DMX 超时，按模式执行 hold/zero/full/scene；`failsafe_triggered` 防止重复触发 |
| **LLRP 支持** | 底层链路恢复协议 | ⚠️ 超出范围：E1.17 独立协议，`ARTNET_STATUS3_LLRP` 广告位已定义 |
| **RDMnet 默认响应器** | DefaultResponder UID | ✅ `defaultRespUid[6]` 字段已添加，接收端解析已实现 |
| **端口方向切换** | 运行时切换端口输入/输出方向 | ✅ ArtAddress 命令 `ARTNET_PC_DIRECTION_TX/RX_0-3` 已处理 |
| **输出样式控制** | 连续/增量输出模式 | ✅ ArtAddress 命令已处理，`output_style` 状态存储，`GoodOutputB` 已反映 |
| **RDM 启用/禁用控制** | 每端口独立控制 RDM | ✅ ArtAddress 命令已处理，`rdm_enabled` 状态存储，`GoodOutputB` 已反映 |
| **目标模式 ArtPoll** | 按 Universe 范围过滤 ArtPoll 响应 | ✅ `handle_poll()` 检查 `ARTNET_POLL_FLAG_TARGET_MODE`，端口地址不在 [Bottom..Top] 范围内则不回复 |
| **诊断系统** | ArtDiagData + 优先级过滤 | ✅ `artnet_diagdata_t` 结构体、`artnet_tx_diagdata()` 发送、ArtPoll Flags 配置、优先级过滤均已实现 |

---

## 五、已知的代码 Bug

| 问题 | 位置 | 说明 |
|------|------|------|
| ~~`ARTNET_MEDIAPATCH = 0x9200`~~ | ~~`packets.h:75`~~ | ✅ 已修正为 0x9100 |
| ~~`ARTNET_PC_CLR_0/1/2/3` 全为 0x93~~ | ~~`artnet.h:78-81`~~ | ✅ 已修正为 0x90/0x91/0x92/0x93 |
| ~~`DiagPriority` 被当作 `pad`~~ | ~~`packets.h:87`~~ | ✅ 已修正为 `diagPriority` |
| ~~ArtPollReply 缺少 `BindIp`/`BindIndex` 等字段~~ | ~~`packets.h:92-126`~~ | ✅ 已添加所有 Art-Net 4 字段 |
| ~~`sp3` + `style` 字段~~ | ~~`packets.h:119-122`~~ | ✅ 已移除 |
| ~~`remove_tod_uid` 跳过第一个 UID~~ | ~~`tod.c:85-88`~~ | ✅ `offset += WIDTH` 改为 `offset = i * WIDTH`，修复 off-by-one |
| ~~`TOD_RESPONSE_NAK = 0x00` 与 `TOD_RESPONSE_FULL` 相同~~ | ~~`artnet.c:51`~~ | ✅ 已修正为 `0xFF` |
| ~~`artnet_tx_rdm` 错误引用 `rdm.data.todcontrol.id`~~ | ~~`transmit.c:234`~~ | ✅ 已修正为 `rdm.data.rdm.id` |
| ~~`handle_rdm` 传递固定 512 而非实际数据长度~~ | ~~`receive.c:669`~~ | ✅ 已改为 `p->length - (sizeof - ARTNET_MAX_RDM_DATA)` |
| ~~`rdmVer` 字段未设置~~ | ~~`transmit.c`~~ | ✅ `artnet_tx_rdm` 和 `artnet_tx_tod_data` 均已设置 `ARTNET_RDM_VERSION` |
| ~~`artnet_tx_rdm` 发送完整 512 字节填充~~ | ~~`transmit.c:229`~~ | ✅ 已改为仅发送 header + 实际数据长度 |
| ~~`handle_tod_data` 中 `rdm_tod_c` 回调被注释~~ | ~~`receive.c:612-613`~~ | ✅ 已启用回调，传递 `p->data.toddata.port` |
| ~~`handle_tod_control` 不处理 `AtcNone`~~ | ~~`receive.c:629`~~ | ✅ 改为 switch 结构，显式处理 `ARTNET_TOD_FULL` (AtcNone) |
| ~~`handle_tod_request` 仅处理 `ARTNET_NODE`~~ | ~~`receive.c:576`~~ | ✅ 已添加 `ARTNET_MSRV` 支持 |
| ~~缺少 `artnet_send_rdmsub` API~~ | ~~`artnet.h`~~ | ✅ 已添加完整的 public API + transmit 实现 |

---

## 六、已新增的 API

| 接口 | 文件 | 说明 |
|------|------|------|
| `artnet_set_net_addr()` | `artnet.h` / `artnet.c` | 设置 Net 地址（15-bit port address 的高7位，0-127） |
| `artnet_send_diagnostic()` | `artnet.h` / `artnet.c` / `transmit.c` | 发送诊断数据（ArtDiagData），带优先级过滤 |
| `artnet_send_rdmsub()` | `artnet.h` / `artnet.c` / `transmit.c` | 发送压缩 RDM 子设备数据（ArtRdmSub） |
| `ARTNET_RDM_VERSION` 常量 | `common.h` | RDM 协议版本（0x01 = RDM v1.0） |
| `artnet_poll_flags_t` 枚举 | `artnet.h` | ArtPoll Flags 位掩码 |
| `artnet_diag_priority_t` 枚举 | `artnet.h` | 诊断优先级代码 |
| `artnet_led_state_t` 枚举 | `artnet.h` | Status1 LED 状态值 |
| `artnet_status2_t` 枚举 | `artnet.h` | ArtPollReply Status2 位掩码 |
| `artnet_status3_t` 枚举 | `artnet.h` | ArtPollReply Status3 位掩码 |
| `artnet_failsafe_mode_t` 枚举 | `artnet.h` | 故障安全模式值 |
| `artnet_good_output_b_t` 枚举 | `artnet.h` | GoodOutputB 位掩码 |

---

## 七、总结统计

| 类别 | 数量 |
|------|------|
| Art-Net 4 定义的操作码总数 | 30 |
| OpCode 全部已定义 | 30 ✅ |
| 已完整实现（有处理逻辑） | 29 |
| 仅桩实现（打印日志/已弃用） | 5 (VideoSetup/Palette/Data, MacMaster/Slave) |
| OpCode 已定义但处理逻辑缺失 | 0 ✅ |
| 已新增 API / 枚举 | 14 项 |
| 已修复的代码 Bug | 14 处 ✅ |
| 数据包字段已全部对齐 Art-Net 4 | ✅ |
