# Art-Net 4 (根据官网文档翻译)

## Art-Net 4 以太网通信协议规范

<www.Art-Net.info>

---

## Artistic Licence

---

## PLASA 2016 创新奖获得者

© 版权所有 Artistic Licence 1998-2022

本文档按“现状”提供，不提供任何形式的保证，包括但不限于针对特定用途的适销性和适用性的暗示保证。

Art-Net™ 是 Artistic Licence 的商标。Art-Net 协议及相关文档的版权归 Artistic Licence 所有。欢迎任何第三方无偿使用此通信协议。有关版权信息的详细信息，请参阅版权声明部分。

Artistic Licence 要求任何实施此协议的制造商通过以下链接申请 OemCode：www.art-net.org.uk/?page_id=203

## 目录

- [Art-Net 4 (根据官网文档翻译)](#art-net-4-根据官网文档翻译)
  - [Art-Net 4 以太网通信协议规范](#art-net-4-以太网通信协议规范)
  - [Artistic Licence](#artistic-licence)
  - [PLASA 2016 创新奖获得者](#plasa-2016-创新奖获得者)
  - [目录](#目录)
  - [文档历史](#文档历史)
    - [修订 AJ-BD 的说明](#修订-aj-bd-的说明)
    - [修订 BE 的说明](#修订-be-的说明)
    - [修订 BF 的说明](#修订-bf-的说明)
    - [修订 BG 的说明](#修订-bg-的说明)
    - [修订 BH 的说明](#修订-bh-的说明)
    - [修订 BI 的说明](#修订-bi-的说明)
    - [修订 BJ 的说明](#修订-bj-的说明)
    - [修订 BK 的说明](#修订-bk-的说明)
    - [修订 DA 的说明](#修订-da-的说明)
    - [修订 DB 的说明](#修订-db-的说明)
    - [修订 DC 的说明](#修订-dc-的说明)
    - [修订 DD 的说明](#修订-dd-的说明)
    - [修订 DE 的说明](#修订-de-的说明)
    - [修订 DF 的说明](#修订-df-的说明)
  - [Art-Net 概述](#art-net-概述)
  - [Art-Net 4](#art-net-4)
  - [Universe 寻址](#universe-寻址)
  - [版权声明](#版权声明)
  - [术语](#术语)
  - [以太网实现](#以太网实现)
    - [一般说明](#一般说明)
    - [协议操作](#协议操作)
    - [IP 地址配置](#ip-地址配置)
    - [IP 地址配置 - DHCP](#ip-地址配置---dhcp)
    - [IP 地址配置 – 静态寻址](#ip-地址配置--静态寻址)
      - [IP 地址示例](#ip-地址示例)
  - [控制器默认轮询](#控制器默认轮询)
  - [Art-Net 数据包定义](#art-net-数据包定义)
    - [ArtPoll](#artpoll)
      - [多个控制器](#多个控制器)
      - [目标模式](#目标模式)
    - [ArtPoll 数据包定义](#artpoll-数据包定义)
    - [表 1 - OpCode](#表-1---opcode)
    - [表 2 - OemCode](#表-2---oemcode)
    - [表 3 – NodeReport 代码](#表-3--nodereport-代码)
    - [表 4 – Style 代码](#表-4--style-代码)
    - [ArtPollReply](#artpollreply)
      - [ArtPollReply 数据包定义](#artpollreply-数据包定义)
    - [ArtlpProg](#artlpprog)
      - [ArtlpProg 数据包定义](#artlpprog-数据包定义)
    - [ArtlpProgReply](#artlpprogreply)
      - [ArtlpProgReply 数据包定义](#artlpprogreply-数据包定义)
    - [ArtAddress](#artaddress)
      - [ArtAddress 数据包定义](#artaddress-数据包定义)
    - [ArtDiagData](#artdiagdata)
      - [ArtDiagData 数据包定义](#artdiagdata-数据包定义)
        - [ArtDiagData](#artdiagdata-1)
        - [表 5 – 优先级代码](#表-5--优先级代码)
    - [ArtTimeCode](#arttimecode)
      - [ArtTimeCode 数据包定义](#arttimecode-数据包定义)
    - [ArtCommand](#artcommand)
      - [ArtCommand 数据包定义](#artcommand-数据包定义)
        - [表 6 – ArtCommand 命令](#表-6--artcommand-命令)
    - [ArtTrigger](#arttrigger)
      - [ArtTrigger 数据包定义](#arttrigger-数据包定义)
    - [ArtDmx](#artdmx)
      - [单播订阅](#单播订阅)
      - [ArtDmx 数据包定义](#artdmx-数据包定义)
    - [ArtSync](#artsync)
      - [管理同步和非同步模式](#管理同步和非同步模式)
      - [ArtSync 数据包定义](#artsync-数据包定义)
    - [ArtNzs](#artnzs)
      - [ArtNzs 数据包定义](#artnzs-数据包定义)
    - [ArtVlc](#artvlc)
      - [ArtVlc 数据包定义](#artvlc-数据包定义)
    - [ArtInput](#artinput)
      - [ArtInput 数据包定义](#artinput-数据包定义)
    - [固件和 UBEA 升级](#固件和-ubea-升级)
    - [ArtFirmwareMaster](#artfirmwaremaster)
      - [ArtFirmwareMaster 数据包定义](#artfirmwaremaster-数据包定义)
    - [ArtFirmwareReply](#artfirmwarereply)
      - [ArtFirmwareReply 数据包定义](#artfirmwarereply-数据包定义)
  - [RDM 支持](#rdm-支持)
    - [RDM 发现](#rdm-发现)
      - [输出网关操作](#输出网关操作)
      - [输入网关操作](#输入网关操作)
      - [控制器操作](#控制器操作)
    - [ArtTodRequest](#arttodrequest)
      - [ArtTodRequest 数据包定义](#arttodrequest-数据包定义)
    - [ArtTodData](#arttoddata)
      - [ArtTodData 数据包定义](#arttoddata-数据包定义)
    - [ArtTodControl](#arttodcontrol)
      - [ArtTodControl 数据包定义](#arttodcontrol-数据包定义)
    - [ArtRdm](#artrdm)
      - [ArtRdm 数据包定义](#artrdm-数据包定义)
    - [ArtRdmSub](#artrdmsub)
      - [ArtRdmSub 数据包定义](#artrdmsub-数据包定义)
  - [状态显示](#状态显示)
  - [数据完整性](#数据完整性)

## 文档历史

### 修订 AJ-BD 的说明

- ArtPollReply 中添加了绑定地址概念。
- 更正了 ArtPollReply 中的填充计数错误。
- ArtPollReply 中添加了 DHCP 标志。
- 添加了 ArtDiagData 数据包。
- 添加了 ArtCommand 数据包。
- 更正了 ArtDmx 单播的细节。
- 添加了关于 ArtDmx 长度的说明。
- 添加了 ArtTimeCode 数据包。
- Art-Net 3 发布。
- 添加了 15 位 Universe 地址。
- 更正了“Net”描述中的错误。
- 解决了有限广播与定向广播之间的混淆。

### 修订 BE 的说明

- 更正了 ArtFirmwareMaster 中的拼写错误 - FirmwareLength 错误地定义为 Int32。
- 添加了 ArtNzs 数据包。
- ArtPollReply 中的 EstaCode 重新定义为两个字节。
- 改进了端口地址编程权限的解释。
- 更正了默认 IP 地址示例中的错误。

### 修订 BF 的说明

- 明确了 ArtRdmSub 中的字节序。
- 改进了 Universe 订阅的描述。

### 修订 BG 的说明

- 明确了 ArtTodControl 必须使用 ArtTodData 回复。

### 修订 BH 的说明

- 说明澄清。
- 提高了可读性。
- 添加了 ArtCommand 的定义。
- 添加了 ArtNzs 的定义。

### 修订 BI 的说明

- 排版修正。
- 添加了 ArtSync。

### 修订 BJ 的说明

- 指出 OpMac OpCode 已弃用。
- 更正了 ArtTodControl 定义中的类型转换错误。

### 修订 BK 的说明

- 添加了 ArtVlc
- ArtPoll 中添加了 Vic 管理
- 建议的 ArtDmx 保活时间改为 800 毫秒到 1000 毫秒，以提高与 sACN 的兼容性。
- 明确了 ArtTodRequest 中的数组长度。

### 修订 DA 的说明

- Art-Net 4 首次发布。
- 取消了绑定节点必须拥有与根节点不同 IP 地址的要求，以避免具有超过四个端口的设备需要进行多宿主配置。
- ArtTodData 中添加了 BindIndex
- ArtPollReply 和 ArtAddress 中添加了 sACN / Art-Net 协议选择功能

### 修订 DB 的说明

- 定义了 ArtPollReply->Status2.Bit4。

### 修订 DC 的说明

- 将所有“Port Address”、“PortAddress”的拼写统一为“Port-Address”。
- ArtIpProg 中的 TCP/IP 端口号编程现在显示为已弃用。

### 修订 DD 的说明

- 更正了 ArtTrigger 中缺失的填充字段。

### 修订 DE 的说明

- 添加了 ArtPollReply->GoodOutputB 字段。
- 更新了 ArtPollReply->Status2。
- 添加了 ArtPollReply->Status3。
- 为故障安全支持添加了额外的 ArtAddress 命令。
- ArtPollReply 中添加了 DefaultResponder。
- 添加了 LLRP 支持标志。
- ArtlpProg 中添加了 DefaultGateway。
- 现在强制要求 ArtRdm 使用单播，以缓解在大型系统中的可扩展性问题。
- 现在强制要求 ArtPollReply 使用单播，以缓解在大型系统中的可扩展性问题。
- 为 ArtPoll 添加了目标模式，以减少发现过程中的网络流量。
- ArtAddress 中添加了 sACN 优先级字段，以便可以设置由 DMX 输入节点生成的 sACN 数据包的优先级。
- 明确了 ArtDmx->Physical 字段与合并的关系。

### 修订 DF 的说明

- ArtAddress 中缺少端口方向的枚举定义。已更正。

## Art-Net 概述

Art-Net 是一种基于 TCP/IP 协议套件的以太网协议。其目的是允许使用标准网络技术在广域网上传输大量 DMX512 数据。

## Art-Net 4

协议的最新修订版实现了许多新功能，同时也简化了数据传输机制。这些变化均基于使用该协议的制造商的反馈。

Art-Net 4 采用了一种新方案来处理支持多个 DMX 端口的网关。以前，支持超过四个 DMX 端口的网关需要多个 IP 地址（多宿主）。这对用户和开发者来说都很麻烦，而且在某些硬件平台上确实无法实现。新方案允许网关（或任何 Art-Net 产品）支持超过 1000 个 DMX 端口。这是通过向 4 个 Art-Net 数据包（即：ArtPollReply、ArtAddress、ArtInput 和 ArtTodData）添加一个名为 BindIndex 的字段来实现的。BindIndex 允许所有 Art-Net 设备识别数据包所引用的“DMX 端口页”。

从开发者的角度来看，这个变化非常小，并且可以非常快地应用到产品设计中。它的添加方式也使其与以前的版本 100% 向后兼容。

Art-Net 可以寻址超过 30,000 个 Universe。以前，每组 4 个 DMX 端口被限制为来自连续 16 个块的 Universe。Art-Net 4 允许解决这个限制，因为开发者可以选择单独标识每个 DMX 端口。这仅仅意味着在每个 ArtPollReply 中对单个 DMX 端口进行编码。使用这种机制，所有 DMX 端口都可以被分配一个完全独立的 Universe。

sACN 作为一种传输 DMX 数据的方法越来越受欢迎。然而，它缺乏任何发现设备、配置设备或传输 RDM 数据的能力。Art-Net 4 整合了管理 sACN 的能力，可以选择给定的网关端口应
将 sACN 还是 Art-Net 转换为 DMX 输出。这允许用户选择 Art-Net 作为发现、管理和 RDM 工具，同时使用 sACN 进行实时控制数据传输。

从 Art-Net 3 更新到 Art-Net 4 的开发者应查阅以下变更摘要：

- ArtPollReply->BindAddress 现在可以在所有 BindIndex 中相同。
- ArtAddress->BindIndex 添加，用于区分来自相同 IP 的数据包
- ArtInput->BindIndex 添加，用于区分来自相同 IP 的数据包
- ArtTodData->BindIndex 添加，用于区分来自相同 IP 的数据包。
- ArtAddress->Command 选项添加，用于选择 sACN 或 Art-Net 转换
- 不允许 ArtPollReply 广播。
- 不再允许 ArtRdm 广播。
- ArtPoll 目标选项。
- ArtPollReply – 众多附加的位字段。

## Universe 寻址

Art-Net 4 规范中存在 32,768 个 Universe 的理论限制。实际可以传输的 Universe 数量取决于网络物理层和所使用的广播方式。下表提供了一个经验法则。

| 寻址方式 | 物理层：10BaseT | 物理层：100BaseT | 物理层：1000BaseT |
|---|---|---|---|
| 单播    | 40    | 400    | 4000+    |

每个 DMX512 Universe 的端口地址被编码为一个 15 位数字，如下表所示。

| 位 15 | 位 14-8 | 位 7-4 | 位 3-0 |
|---|---|---|---|
| 0    | Net    | Sub-Net    | Universe    |
| Port-Address    |    |    |    |

高字节被称为“Net”。这是在 Art-Net 3 中引入的，以前为零。每个节点具有单个 Net 值。低字节的高半字节被称为子网地址，并为每个节点设置为单个值。低字节的低半字节用于定义节点内的单个 DMX512 Universe。

这意味着任何节点将具有：

- 一个“Net”开关。
- 一个“Sub-Net”开关。
- 每个实现的 DMX512 输入或输出有一个“Universe”开关。

产品设计者可以选择将这些实现为硬开关或软开关。

## 版权声明

任何在其产品中实现 Art-Net 的个人或实体应在用户指南中注明版权信息："Art-Net™ Designed by and Copyright Artistic Licence"。

## 术语

**节点：** 将 DMX512 与 Art-Net 相互转换的设备称为节点。

**端口地址：** DMX 帧可以定向到的 32,768 个可能地址之一。端口地址是由 Net+Sub-Net+Universe 组成的 15 位数字。

**Net：** 16 个连续的子网或 256 个连续的 Universe 被称为一个网。总共有 128 个 Net。

**子网：** 一组 16 个连续的 Universe 被称为一个子网。（不要与子网掩码混淆）。

**Universe：** 一个包含 512 个通道的单一 DMX512 帧被称为一个 Universe。

**Kiloverse：** 一组 1024 个 Universe。

**控制器：** 中央控制器或监控设备（灯光控制台）被称为控制器。

**IP：** IP 是互联网协议地址。它以长字格式（0x12345678）或点分格式（2.255.255.255）表示。惯例是前者为十六进制，后者为十进制。IP 唯一地标识网络上的任何节点或控制器。

**子网掩码：** 定义 IP 的哪一部分代表网络地址，哪一部分代表节点地址。示例：子网掩码为 255.0.0.0 意味着 IP 的第一个字节是网络地址，其余三个字节是节点地址。

**端口：** Art-Net 上的实际数据传输使用在 TCP/IP 协议“之上”运行的 UDP 协议。UDP 数据传输通过将数据从一个节点或控制器上的特定 IP:端口传输到第二个节点或控制器上的第二个特定 IP:端口来操作。Art-Net 仅使用端口 0x1936。

**定向广播：** 当网络首次连接时，控制器不知道网络上的节点数量，也不知道它们的 IP 地址。定向广播地址允许控制器向网络上的所有节点发送 ArtPoll。

**有限广播：** Art-Net 数据包不应广播到 255.255.255.255 的有限广播地址。

**控制器：** 描述主要任务是生成控制数据的 Art-Net 设备的通用术语。例如，灯光控制台。

**媒体服务器：** 描述能够基于 Art-Net 的“mx”媒体扩展生成控制数据的 Art-Net 设备的通用术语。

## 以太网实现

### 一般说明

所有通信都是 UDP。本文档中定义的每个数据包格式构成封装 UDP 数据包的 Data 字段。

数据包格式的指定方式类似于 C 语言结构，其中所有数据项根据位数被视为无符号整数类型 INT8、INT16 或 INT32。除了在数据包的最末端，没有隐藏的填充字节，为了对齐，末端可能会向上取整为 2 或 4 字节的倍数。有效接收到的数据包末尾的额外字节将被忽略。

协议进行了概括，以处理未来版本中端口数量的增加。

许多位数据字段包含未使用的位置。这些可能在未来的协议版本中使用。发送时应置零，接收方不应测试。

所有数据包定义的设计都使其长度可以在未来的修订版中增加，同时保持兼容性。因此，在本协议中仅检查最小数据包长度。

### 协议操作

节点在一种模式下运行，每个节点具有从其以太网 MAC 地址派生的唯一 IP 地址。用作源和目标的 UDP 端口是 0x1936。

### IP 地址配置

Art-Net 协议可以在 DHCP 管理的地址方案或使用静态地址上运行。默认情况下，Art-Net 产品在出厂时将使用 A 类 IP 地址方案启动。这使得 Art-Net 产品无需 DHCP 服务器连接到网络即可直接通信。

### IP 地址配置 - DHCP

节点在 ArtPollReply 数据包中报告它们是否支持 DHCP。本文档假设使用静态寻址来详细说明数据包。当使用 DHCP 时，寻址和子网掩码将根据 DHCP 服务器的指示进行修改。

### IP 地址配置 – 静态寻址

在封闭网络内允许使用 A 类寻址。确保 Art-Net 数据不被路由到互联网上非常重要。

实现 Art-Net 的产品应默认为主 IP 地址 2.?.?.?.?。

IP 地址由一个 32 位数字组成，指定为 A.B.C.D。字节 B.C.D 根据 MAC 地址计算。高字节“A”设置为下表中所示的两个值之一。

MAC 地址是一个 48 位数字，指定为 u:v:w:x:y:z。这是一个全局唯一的数字。前三个字节“u:v:w”注册给特定的组织。后三个字节“x:y:z”由该组织分配。为了确保支持 Art-Net 的不同制造商之间发生 IP 地址冲突的可能性最小，产品 OEM 代码被添加到 MAC 地址中。

IP 地址的“B”字段是通过将 OEM 代码的高字节与 OEM 代码的低字节以及 MAC 地址的“x”字段相加来计算的。

上电时，节点检查其 IP 寻址模式的配置。如果已编程为使用自定义 IP 地址，则不使用以下过程。

| | IP 地址 A.B.C.D | | | | 子网掩码 |
|---|---|---|---|---|---|
| 产品开关设置 | A | B | C | D | |
| 自定义 IP 已编程 | 按编程设置 | | | | 按编程设置 |
| 网络开关关闭 | 2 | x+OEM | y | z | 255.0.0.0 |
| 网络开关打开 | 10 | x+OEM | y | z | 255.0.0.0 |

除非使用自定义 IP 地址，否则子网掩码始终初始化为 255.0.0.0。这意味着网络地址是 IP 地址的最高有效 8 位，节点地址是 IP 地址的最低有效 24 位。这是一个 A 类网络地址，因此连接到其他网络时必须小心。如果安装需要将 Art-Net 网络连接到具有 Internet 访问权限的另一个网络，则必须通过过滤掉 A 类地址的路由器来实现连接。

#### IP 地址示例

给定以下设置，IP 地址计算如下：

- 网络开关 = 关闭
- MAC 地址 = 12:45:78:98:34:76（十六进制数）
- OEM 代码 = 0x0010

计算：

- IP 地址 A = 2.
- IP 地址 B = 168 (0x98 + 0 + 16)。
- IP 地址 C = 52（来自 MAC 地址的 0x34）。
- IP 地址 D = 118（来自 MAC 地址的 0x76）。
- IP 地址 = 2.168.52.118。

## 控制器默认轮询

默认情况下，控制器应轮询主 Art-Net 地址和次 Art-Net 地址：

- 2.255.255.255:0x1936 - 主 Art-Net 地址
- 10.255.255.255:0x1936 - 次 Art-Net 地址

## Art-Net 数据包定义

节点接受的所有 UDP 数据包均符合以下定义的 Art-Net 协议规范。任何其他数据包都将被忽略。

### ArtPoll

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 全部 | 接收 | 发送 ArtPollReply。 |
| | 单播发送 | 允许，带目标模式。 |
| | 定向广播 | 控制器广播此数据包以轮询网络上的所有控制器和节点。 |
| | 有限广播 | 不推荐。 |

ArtPoll 数据包用于发现其他控制器、节点和媒体服务器的存在。ArtPoll 数据包可以由任何设备发送，但通常仅由控制器发送。控制器和节点都会响应该数据包。

控制器向 IP 地址 2.255.255.255（子网掩码 255.0.0.0）的 UDP 端口 0x1936 广播 ArtPoll 数据包，这是定向广播地址。

控制器可以假设在发送 ArtPoll 和接收所有 ArtPollReply 数据包之间的最大超时时间为 3 秒。如果控制器在此时间内未收到响应，则应认为节点已断开连接。

广播 ArtPoll 的控制器还应（通过单播）使用 ArtPollReply 回复自己的消息。Art-Net 要求所有控制器每 2.5 到 3 秒广播一次 ArtPoll。这确保任何网络设备都能轻松检测到断开连接。

#### 多个控制器

Art-Net 允许并支持网络上的多个控制器。当存在多个控制器时，节点将从不同的控制器接收 ArtPoll，这些 ArtPoll 可能包含冲突的诊断要求。这通过以下方式解决：

如果任何控制器请求诊断，节点将发送诊断信息。（ArtPoll->Flags->2）。

如果有多个控制器请求诊断，诊断信息将被广播。（忽略 ArtPoll->Flags->3）。

应使用最低的优先级最小值。（忽略 ArtPoll->DiagPriority）。

#### 目标模式

目标模式允许 ArtPoll 定义一个端口地址范围。只有当节点订阅的端口地址包含在 TargetPortAddressBottom 到 TargetPortAddressTop 的范围内时，节点才会回复 ArtPoll。位字段 ArtPoll->Flags->5 用于启用目标模式。

### ArtPoll 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpCode 定义了此 UDP 数据包中 ArtPoll 之后的数据类别。先传输低字节。有关 OpCode 列表，请参见表 1。设置为 OpPoll。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14。控制器应忽略与使用低于 14 的协议版本的节点的通信。 |
| 5 | Flags | Int8 | - | 设置节点行为 |
| | | | 7-6 | 未使用，发送为零，接收时不测试。 |
| | | | 5 | 0 = 禁用目标模式。 |
| | | | | 1 = 启用目标模式。 |
| | | | 4 | 0 = 启用 VLC 传输。 |
| | | | | 1 = 禁用 VLC 传输。 |
| | | | 3 | 0 = 诊断消息广播（如果位 2 为 1）。 |
| | | | | 1 = 诊断消息单播（如果位 2 为 1）。 |
| | | | 2 | 0 = 不向我发送诊断消息。 |
| | | | | 1 = 向我发送诊断消息。 |
| | | | 1 | 0 = 仅在响应 ArtPoll 或 ArtAddress 时发送 ArtPollReply。 |
| | | | | 1 = 当节点状态发生变化时发送 ArtPollReply。此选择允许控制器在无需持续轮询的情况下获知变化。 |
| | | | 0 | 0 = 已弃用。 |
| 6 | DiagPriority | Int8 | - | 应发送的诊断消息的最低优先级。参见表 5。 |
| 7 | TargetPortAddressTopHi | Int8 | - | 如果目标模式激活，要测试的端口地址范围的上限。 |
| 8 | TargetPortAddressTopLo | Int8 | - | |
| 9 | TargetPortAddressBottomHi | Int8 | - | 如果目标模式激活，要测试的端口地址范围的下限。 |
| 10 | TargetPortAddressBottomLo | Int8 | - | |

### 表 1 - OpCode

下表详细列出了 Art-Net 数据包中使用的合法 OpCode 值：

| Opcodes | 值 | 定义 |
|---|---|---|
| OpPoll | 0x2000 | 这是一个 ArtPoll 数据包，此 UDP 数据包中不包含其他数据。 |
| OpPollReply | 0x2100 | 这是一个 ArtPollReply 数据包。它包含设备状态信息。 |
| OpDiagData | 0x2300 | 诊断和数据记录数据包。 |
| OpCommand | 0x2400 | 用于发送基于文本的参数命令。 |
| OpOutput / OpDmx | 0x5000 | 这是一个 ArtDmx 数据包。它包含单个 Universe 的零起始码 DMX512 信息。 |
| OpNzs | 0x5100 | 这是一个 ArtNzs 数据包。它包含单个 Universe 的非零起始码（RDM 除外）DMX512 信息。 |
| OpSync | 0x5200 | 这是一个 ArtSync 数据包。它用于强制将 ArtDmx 数据包同步传输到节点的输出。 |
| OpAddress | 0x6000 | 这是一个 ArtAddress 数据包。它包含节点的远程编程信息。 |
| Opinput | 0x7000 | 这是一个 ArtInput 数据包。它包含 DMX 输入的启用 - 禁用数据。 |
| OpTodRequest | 0x8000 | 这是一个 ArtTodRequest 数据包。它用于请求 RDM 发现的设备表 (ToD)。 |
| OpTodData | 0x8100 | 这是一个 ArtTodData 数据包。它用于发送 RDM 发现的设备表 (ToD)。 |
| OpTodControl | 0x8200 | 这是一个 ArtTodControl 数据包。它用于发送 RDM 发现控制消息。 |
| OpRdm | 0x8300 | 这是一个 ArtRdm 数据包。它用于发送所有非发现的 RDM 消息。 |
| OpRdmSub | 0x8400 | 这是一个 ArtRdmSub 数据包。它用于发送压缩的 RDM 子设备数据。 |
| OpVideoSetup | 0xa010 | 这是一个 ArtVideoSetup 数据包。它包含实现扩展视频功能的节点的视频屏幕设置信息。 |
| OpVideoPalette | 0xa020 | 这是一个 ArtVideoPalette 数据包。它包含实现扩展视频功能的节点的调色板设置信息。 |
| OpVideoData | 0xa040 | 这是一个 ArtVideoData 数据包。它包含实现扩展视频功能的节点的显示数据。 |
| OpMacMaster | 0xf000 | 此数据包已弃用。 |
| OpMacSlave | 0xf100 | 此数据包已弃用。 |
| OpFirmwareMaster | 0xf200 | 这是一个 ArtFirmwareMaster 数据包。它用于向节点上传新固件或固件扩展。 |
| OpFirmwareReply | 0xf300 | 这是一个 ArtFirmwareReply 数据包。节点返回它以确认收到 ArtFirmwareMaster 数据包或 ArtFileTnMaster 数据包。 |
| OpFileTnMaster | 0xf400 | 将用户文件上传到节点。 |
| OpFileFnMaster | 0xf500 | 从节点下载用户文件。 |
| OpFileFnReply | 0xf600 | 服务器到节点的下载数据包确认。 |
| OplpProg | 0xf800 | 这是一个 ArtlpProg 数据包。它用于重新编程节点的 IP 地址和掩码。 |
| OplpProgReply | 0xf900 | 这是一个 ArtlpProgReply 数据包。节点返回它以确认收到 ArtlpProg 数据包。 |
| OpMedia | 0x9000 | 这是一个 ArtMedia 数据包。它由媒体服务器单播并由控制器处理。 |
| OpMediaPatch | 0x9100 | 这是一个 ArtMediaPatch 数据包。它由控制器单播并由媒体服务器处理。 |
| OpMediaControl | 0x9200 | 这是一个 ArtMediaControl 数据包。它由控制器单播并由媒体服务器处理。 |
| OpMediaContrlReply | 0x9300 | 这是一个 ArtMediaControlReply 数据包。它由媒体服务器单播并由控制器处理。 |
| OpTimeCode | 0x9700 | 这是一个 ArtTimeCode 数据包。它用于通过网络传输时间码。 |
| OpTimeSync | 0x9800 | 用于同步实时日期和时钟 |
| OpTrigger | 0x9900 | 用于发送触发宏 |
| OpDirectory | 0x9a00 | 请求节点的文件列表 |
| OpDirectoryReply | 0x9b00 | 使用文件列表回复 OpDirectory |

### 表 2 - OemCode

注册的 OEM 代码在 DMX-Workshop 安装的 SDK 目录中的“Art-NetOemCodes.h”文件中有详细说明。

OEM 代码定义了特定制造商的产品类型。OemCode 在 ArtPollReply 中返回。

### 表 3 – NodeReport 代码

下表详细说明了 NodeReport 代码。NodeReport 代码定义了节点和控制器的通用错误、通知和状态消息。NodeReport 在 ArtPollReply 中返回。

| 代码 | 助记符 | 说明 |
|---|---|---|
| 0x0000 | RcDebug | 在调试模式下启动（仅用于开发） |
| 0x0001 | RcPowerOk | 上电测试成功 |
| 0x0002 | RcPowerFail | 上电时硬件测试失败 |
| 0x0003 | RcSocketWr1 | 来自节点的最后一个 UDP 由于长度截断而失败，很可能是由于冲突造成的。 |
| 0x0004 | RcParseFail | 无法识别最后一个 UDP 传输。检查 OpCode 和数据包长度。 |
| 0x0005 | RcUdpFail | 在上次传输尝试中无法打开 UDP 套接字 |
| 0x0006 | RcShNameOk | 确认通过 ArtAddress 编程短名称成功。 |
| 0x0007 | RcLoNameOk | 确认通过 ArtAddress 编程长名称成功。 |
| 0x0008 | RcDmxError | 检测到 DMX512 接收错误。 |
| 0x0009 | RcDmxUdpFull | 内部 DMX 发送缓冲区耗尽。 |
| 0x000a | RcDmxRxFull | 内部 DMX 接收缓冲区耗尽。 |
| 0x000b | RcSwitchErr | 接收 Universe 开关冲突。 |
| 0x000c | RcConfigErr | 产品配置与固件不匹配。 |
| 0x000d | RcDmxShort | 检测到 DMX 输出短路。参见 GoodOutput 字段。 |
| 0x000e | RcFirmwareFail | 上次上传新固件失败。 |
| 0x000f | RcUserFail | 用户更改了远程编程锁定的地址开关设置。用户更改被忽略。 |
| 0x0010 | RcFactoryRes | 已发生工厂重置。 |

### 表 4 – Style 代码

下表详细说明了 Style 代码。Style 代码定义了控制器的通用功能。Style 代码在 ArtPollReply 中返回。

| 代码 | 助记符 | 说明 |
|---|---|---|
| 0x00 | StNode | DMX 到/从 Art-Net 设备 |
| 0x01 | StController | 灯光控制台。 |
| 0x02 | StMedia | 媒体服务器。 |
| 0x03 | StRoute | 网络路由设备。 |
| 0x04 | StBackup | 备份设备。 |
| 0x05 | StConfig | 配置或诊断工具。 |
| 0x06 | StVisual | 可视化器。 |

### ArtPollReply

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 所有设备 | 接收 | 无 Art-Net 动作。 |
| | 单播发送 | 允许。 |
| | 广播 | 不允许。 |

设备在响应控制器的 ArtPoll 时发送 ArtPollReply。设备在发送回复前应等待最多 1 秒的随机延迟。此机制旨在减少在扩展到非常大的系统时的数据包拥塞。

#### ArtPollReply 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpPollReply 先传输低字节。 |
| 3 | IP Address[4] | Int8 | - | 包含节点 IP 地址的数组。第一个数组条目是地址的最高有效字节。实现绑定时，绑定节点可以共享根节点的 IP 地址，并使用 BindIndex 来区分节点。 |
| 4 | Port | Int16 | - | 端口始终为 0x1936，先传输低字节。 |
| 5 | VersInfoH | Int8 | - | 节点固件修订号的高字节。控制器应仅使用此字段决定是否应进行固件更新。惯例是数字越大表示固件版本越新。 |
| 6 | VersInfoL | Int8 | - | 节点固件修订号的低字节。 |
| 7 | NetSwitch | Int8 | - | 15 位端口地址的位 14-8 编码到此字段的低 7 位。这结合 SubSwitch 和 SwIn[] 或 SwOut[] 一起使用以生成完整的 Universe 地址。 |
| 8 | SubSwitch | Int8 | - | 15 位端口地址的位 7-4 编码到此字段的低 4 位。这结合 NetSwitch 和 SwIn[] 或 SwOut[] 一起使用以生成完整的 Universe 地址。 |
| 9 | OemHi | Int8 | - | Oem 值的高字节。 |
| 10 | Oem | Int8 | - | Oem 值的低字节。Oem 字描述设备供应商和可用功能集。位 15 为高表示扩展功能可用。当前注册的代码在表 2 中定义。 |
| 11 | Ubea Version | Int8 | - | 此字段包含用户 BIOS 扩展区 (UBEA) 的固件版本。如果 UBEA 未编程，此字段包含零。 |
| 12 | Status1 | Int8 | - | 通用状态寄存器，包含以下位字段。 |
| | | | 7-6 | 指示灯状态。 |
| | | | | 00 指示灯状态未知。 |
| | | | | 01 指示灯处于定位/识别模式。 |
| | | | | 10 指示灯处于静音模式。 |
| | | | | 11 指示灯处于正常模式。 |
| | | | 5-4 | 端口地址编程权限 |
| | | | | 00 端口地址编程权限未知。 |
| | | | | 01 所有端口地址由前面板控件设置。 |
| | | | | 10 所有或部分端口地址通过网络或 Web 浏览器编程。 |
| | | | | 11 未使用。 |
| | | | 3 | 未实现，发送为零，接收方不测试。 |
| | | | 2 | 0 = 正常固件启动（从闪存）。不支持双启动的节点，请将此字段清零。 |
| | | | | 1 = 从 ROM 启动。 |
| | | | 1 | 0 = 不支持远程设备管理 (RDM)。 |
| | | | | 1 = 支持远程设备管理 (RDM)。 |
| | | | 0 | 0 = UBEA 不存在或损坏 |
| | | | | 1 = UBEA 存在 |
| 13 | EstaManLo | Int8 | - | ESTA 制造商代码。这些代码用于表示设备制造商。它们由 ESTA 分配。此字段可解释为两个 ASCII 字节，代表制造商的首字母。 |
| 14 | EstaManHi | Int8 | - | 上述的高字节 |
| 15 | ShortName [18] | Int8 | - | 数组代表节点的空终止短名称。控制器使用 ArtAddress 数据包编程此字符串。最大长度为 17 个字符加空字符。这是一个固定长度字段，尽管它包含的字符串可以比字段短。 |
| 16 | LongName [64] | Int8 | - | 数组代表节点的空终止长名称。控制器使用 ArtAddress 数据包编程此字符串。最大长度为 63 个字符加空字符。这是一个固定长度字段，尽管它包含的字符串可以比字段短。 |
| 17 | NodeReport [64] | Int8 | - | 数组是节点运行状态或操作错误的文本报告。它主要用于“工程”数据而不是“最终用户”数据。字段格式为：“#xxxx [yyyy..] zzzzz...” |
| | | | | xxxx 是十六进制状态码，定义在表 3 中。yyyy 是十进制计数器，每次节点发送 ArtPollResponse 时递增。这允许控制器监控节点中的事件变化。 |
| | | | | zzzz 是定义状态的英文字符串。这是一个固定长度字段，尽管它包含的字符串可以比字段短。 |
| 18 | NumPortsHi | Int8 | - | 描述输入或输出端口数量的字的高字节。高字节用于未来扩展，当前为零。 |
| 19 | NumPortsLo | Int8 | - | 描述输入或输出端口数量的字的低字节。如果输入数量不等于输出数量，则取最大值。零是合法值，如果未实现输入或输出端口。最大值为 4。节点可以忽略此字段，因为信息隐含在 PortTypes[] 中。 |
| 20 | PortTypes [4] | Int8 | - | 此数组定义每个通道的操作和协议。（具有 4 个输入和 4 个输出的产品将报告 0xc0、0xc0、0xc0、0xc0）。数组长度是固定的，与节点上物理可用的输入或输出数量无关。 |
| | | | 7 | 设置为此通道可以从 Art-Net 网络输出数据。 |
| | | | 6 | 设置为此通道可以将输入上传到 Art-Net 网络。 |
| | | | 5-0 | 000000 = DMX512 |
| | | | | 000001 = MIDI |
| | | | | 000010 = Avab |
| | | | | 000011 = Colortran CMX |
| | | | | 000100 = ADB 62.5 |
| | | | | 000101 = Art-Net |
| | | | | 000110 = DALI |
| 21 | GoodInput [4] | Int8 | - | 此数组定义节点的输入状态。 |
| | | | 7 | 设置 – 已接收数据。 |
| | | | 6 | 设置 – 通道包含 DMX512 测试数据包。 |
| | | | 5 | 设置 – 通道包含 DMX512 SIP。 |
| | | | 4 | 设置 – 通道包含 DMX512 文本数据包。 |
| | | | 3 | 设置 – 输入被禁用。 |
| | | | 2 | 设置 – 检测到接收错误。 |
| | | | 1-0 | 未使用，发送为零。 |
| 22 | GoodOutputA [4] | Int8 | - | 此数组定义节点的输出状态。 |
| | | | 7 | 设置 – 正在传输数据。 |
| | | | 6 | 设置 – 通道包含 DMX512 测试数据包。 |
| | | | 5 | 设置 – 通道包含 DMX512 SIP。 |
| | | | 4 | 设置 – 通道包含 DMX512 文本数据包。 |
| | | | 3 | 设置 – 输出正在合并 ArtNet 数据。 |
| | | | 2 | 设置 – 上电时检测到 DMX 输出短路 |
| | | | 1 | 设置 – 合并模式为 LTP。 |
| | | | 0 | 设置 – 输出被选择为传输 sACN。 |
| | | | | Clr – 输出被选择为传输 Art-Net。 |
| 23 | SwIn [4] | Int8 | - | 4 个可能输入端口各自的 15 位端口地址的位 3-0 编码到低半字节中。 |
| 24 | SwOut [4] | Int8 | - | 4 个可能输出端口各自的 15 位端口地址的位 3-0 编码到低半字节中。 |
| 25 | AcnPriority | Int8 | - | 当任何接收到的 DMX 转换为 sACN 时将使用的 sACN 优先级值。 |
| 26 | SwMacro | Int8 | - | 如果节点支持宏键输入，此字节代表触发值。节点负责“去抖动”输入。当 ArtPollReply 设置为自动传输时（Flags 位 1），ArtPollReply 将在按键按下和按键释放事件时发送。但是，控制器不应假设只有一个位位置发生了变化。宏输入用于远程事件触发或提示。位字段为高电平有效。 |
| | | | 7 | 设置 – 宏 8 激活。 |
| | | | 6 | 设置 – 宏 7 激活。 |
| | | | 5 | 设置 – 宏 6 激活。 |
| | | | 4 | 设置 – 宏 5 激活。 |
| | | | 3 | 设置 – 宏 4 激活。 |
| | | | 2 | 设置 – 宏 3 激活。 |
| | | | 1 | 设置 – 宏 2 激活。 |
| | | | 0 | 设置 – 宏 1 激活。 |
| 27 | SwRemote | Int8 | - | 如果节点支持远程触发输入，此字节代表触发值。节点负责“去抖动”输入。当 ArtPollReply 设置为自动传输时（Flags 位 1），ArtPollReply 将在按键按下和按键释放事件时发送。但是，控制器不应假设只有一个位位置发生了变化。远程输入用于远程事件触发或提示。位字段为高电平有效。 |
| | | | 7 | 设置 – 远程 8 激活。 |
| | | | 6 | 设置 – 远程 7 激活。 |
| | | | 5 | 设置 – 远程 6 激活。 |
| | | | 4 | 设置 – 远程 5 激活。 |
| | | | 3 | 设置 – 远程 4 激活。 |
| | | | 2 | 设置 – 远程 3 激活。 |
| | | | 1 | 设置 – 远程 2 激活。 |
| | | | 0 | 设置 – 远程 1 激活。 |
| 28 | Spare | Int8 | | 未使用，设置为零 |
| 29 | Spare | Int8 | | 未使用，设置为零 |
| 30 | MAC Hi | Int8 | | MAC 地址高字节。如果节点无法提供此信息，则设置为零。 |
| 31 | MAC | Int8 | | MAC 地址 |
| 32 | MAC | Int8 | | MAC 地址 |
| 33 | MAC | Int8 | | MAC 地址 |
| 34 | MAC | Int8 | | MAC 地址 |
| 35 | MAC Lo | Int8 | | MAC 地址低字节 |
| 36 | Bindlp[4] | Int8 | | 如果此单元是较大或模块化产品的一部分，这是根设备的 IP。 |
| 37 | BindIndex | Int8 | | 此数字代表绑定设备的顺序。数字越小表示越接近根设备。值 1 表示根设备。 |
| 38 | Status2 | Int8 | | |
| | | | 0 | 设置 = 产品支持 Web 浏览器配置。 |
| | | | 1 | Clr = 节点的 IP 是手动配置的。 |
| | | | | 设置 = 节点的 IP 是 DHCP 配置的。 |
| | | | 2 | Clr = 节点不支持 DHCP。 |
| | | | | 设置 = 节点支持 DHCP。 |
| | | | 3 | Clr = 节点支持 8 位端口地址（Art-Net II）。 |
| | | | | 设置 = 节点支持 15 位端口地址（Art-Net 3 或 4）。 |
| | | | 4 | Clr = 节点无法在 Art-Net 和 sACN 之间切换。 |
| | | | | 设置 = 节点能够在 Art-Net 和 sACN 之间切换。 |
| | | | 5 | Clr = 未发出警报声。 |
| | | | | 设置 = 发出警报声。 |
| | | | 6 | Clr = 节点不支持使用 ArtAddress 切换输出样式。 |
| | | | | 设置 = 节点支持使用 ArtAddress 切换输出样式。 |
| | | | 7 | Clr = 节点不支持使用 ArtAddress 控制 RDM。 |
| | | | | 设置 = 节点支持使用 ArtAddress 控制 RDM。 |
| 39 | GoodOutputB [4] | Int8 | - | 此数组定义节点的输出状态。 |
| | | | 7 | 设置 – RDM 已禁用。 |
| | | | | Clr – RDM 已启用。 |
| | | | 6 | 设置 – 输出样式为连续 |
| | | | | Clr – 输出样式为增量。 |
| | | | 5 | 未使用，设置为零 |
| | | | 4 | 未使用，设置为零 |
| | | | 3 | 未使用，设置为零 |
| | | | 2 | 未使用，设置为零 |
| | | | 1 | 未使用，设置为零 |
| | | | 0 | 未使用，设置为零 |
| 40 | Status3 | Int8 | - | 通用状态寄存器，包含以下位字段。 |
| | | | 7-6 | 故障安全状态。在网络数据丢失事件中节点的行为方式。 |
| | | | | 00 保持最后状态。 |
| | | | | 01 所有输出归零。 |
| | | | | 10 所有输出满值。 |
| | | | | 11 播放故障安全场景。 |
| | | | 5 | 设置 – 节点支持故障切换。 |
| | | | | Clr – 节点不支持故障切换。 |
| | | | 4 | 设置 – 节点支持 LLRP。 |
| | | | | Clr – 节点不支持 LLRP。 |
| | | | 3 | 设置 – 节点支持在输入和输出之间切换端口。（PortTypes[] 显示当前方向） |
| | | | | Clr – 节点不支持切换端口方向。 |
| | | | 2 | 未使用，设置为零 |
| | | | 1 | 未使用，设置为零 |
| | | | 0 | 未使用，设置为零 |
| 41 | DefaultRespUID Hi | Int8 | | RDMnet & LLRP 默认响应器 UID 最高有效字节 |
| 42 | DefaultRespUID | Int8 | | RDMnet & LLRP 默认响应器 UID |
| 43 | DefaultRespUID | Int8 | | RDMnet & LLRP 默认响应器 UID |
| 44 | DefaultRespUID | Int8 | | RDMnet & LLRP 默认响应器 UID |
| 45 | DefaultRespUID | Int8 | | RDMnet & LLRP 默认响应器 UID |
| 46 | DefaultRespUID Lo | Int8 | | RDMnet & LLRP 默认响应器 UID 最低有效字节 |
| 47 | Filler | 15 x 8 | | 发送为零。用于未来扩展。 |

### ArtlpProg

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 控制器传输到特定节点 IP 地址。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 用 ArtlpProgReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 用 ArtlpProgReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

ArtlpProg 数据包允许重新编程节点的 IP 设置。

ArtlpProg 数据包由控制器发送到节点的私有地址。如果节点支持远程编程 IP 地址，它将使用 ArtlpProgReply 数据包进行响应。在所有情况下，ArtlpProgReply 都发送到发送方的私有地址。

#### ArtlpProg 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OplpProg 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Filler1 | Int8 | - | 填充长度以匹配 ArtPoll。 |
| 6 | Filler2 | Int8 | - | 填充长度以匹配 ArtPoll。 |
| 7 | Command | Int8 | - | 按如下方式处理此数据包：定义如何处理此数据包。如果所有位都清零，则仅为查询。 |
| | | | 7 | 设置以启用任何编程。 |
| | | | 6 | 设置以启用 DHCP（如果设置则忽略低位）。 |
| | | | 5 | 未使用，发送为零 |
| | | | 4 | 编程默认网关 |
| | | | 3 | 设置以将所有三个参数返回默认值 |
| | | | 2 | 编程 IP 地址 |
| | | | 1 | 编程子网掩码 |
| | | | 0 | 编程端口 |
| 8 | Filler4 | Int8 | | 设置为零。为字对齐填充数据结构。 |
| 9 | ProgIpHi | Int8 | | 如果通过 Command 字段启用，要编程到节点中的 IP 地址 |
| 10 | ProgIp2 | Int8 | | |
| 11 | ProgIp1 | Int8 | | |
| 12 | ProgIplo | Int8 | | |
| 13 | ProgSmHi | Int8 | | 如果通过 Command 字段启用，要编程到节点中的子网掩码 |
| 14 | ProgSm2 | Int8 | | |
| 15 | ProgSm1 | Int8 | | |
| 16 | ProgSmLo | Int8 | | |
| 17 | ProgPortHi | Int8 | | （已弃用） |
| 18 | ProgPortLo | Int8 | | |
| 19 | ProgDgHi | Int8 | | 如果通过 Command 字段启用，要编程到节点中的默认网关 |
| 20 | ProgDg2 | Int8 | | |
| 21 | ProgDg1 | Int8 | | |
| 22 | ProgDgLo | Int8 | | |
| 23-26 | Spare4-8 | Int8 | | 发送为零，接收方不测试。 |

### ArtlpProgReply

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 无动作。 |
| | 单播发送 | 传输到特定控制器 IP 地址。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 无动作 |
| | 单播发送 | 传输到特定控制器 IP 地址。 |
| | 广播 | 不允许。 |

ArtlpProgReply 数据包由节点响应 ArtlpProg 数据包而发出。不支持 IP 地址远程编程的节点不回复 ArtlpProg 数据包。在所有情况下，ArtlpProgReply 都发送到发送方的私有地址。

#### ArtlpProgReply 数据包定义

| 字段 | 名称 | 大小 | 说明 |
|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | OplpProgReply 先传输低字节。 |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。(14) |
| 5 | Filler1 | Int8 | 填充长度以匹配 ArtPoll。 |
| 6 | Filler2 | Int8 | 填充长度以匹配 ArtPoll。 |
| 7 | Filler3 | Int8 | 填充长度以匹配 ArtlpProg。 |
| 8 | Filler4 | Int8 | 填充长度以匹配 ArtlpProg。 |
| 9 | ProgIpHi | Int8 | 节点的 IP 地址。 |
| 10 | ProgIp2 | Int8 | |
| 11 | ProgIp1 | Int8 | |
| 12 | ProgIpLo | Int8 | |
| 13 | ProgSmHi | Int8 | 节点的子网掩码。 |
| 14 | ProgSm2 | Int8 | |
| 15 | ProgSm1 | Int8 | |
| 16 | ProgSmLo | Int8 | |
| 17 | ProgPort Hi | Int8 | （已弃用）。 |
| 18 | ProgPort Lo | Int8 | |
| 19 | Status | Int8 | 位 7 0 |
| | | | 位 6 DHCP 已启用。 |
| | | | 位 5-0 0 |
| 20 | Spare2 | Int8 | 发送为零，接收方不测试。 |
| 21 | ProgDgHi | Int8 | 节点的默认网关。 |
| 22 | ProgDg2 | Int8 | |
| 23 | ProgDg1 | Int8 | |
| 24 | ProgDgLo | Int8 | |
| 25 | Spare7 | Int8 | 发送为零，接收方不测试。 |
| 26 | Spare8 | Int8 | 发送为零，接收方不测试。 |

### ArtAddress

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 控制器传输到特定节点 IP 地址。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 通过单播 ArtPollReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 通过单播 ArtPollReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

网络上的控制器或监控设备可以远程重新编程节点的众多控制项。例如，这将允许灯光控制台在远程位置重新路由 DMX512 数据。这是通过向节点的 IP 地址发送 ArtAddress 数据包来实现的。（IP 地址在 ArtPoll 数据包中返回）。节点使用 ArtPollReply 数据包进行回复。

字段 5 到 13 包含将编程到节点中的数据。

#### ArtAddress 数据包定义

| 字段 | 名称 | 大小 | 说明 | |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | |
| 2 | OpCode | Int16 | OpAddress 先传输低字节。 | |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 | |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。当前值 14 | |
| 5 | NetSwitch | Int8 | 15 位端口地址的位 14-8 编码到此字段的低 7 位。这结合 SubSwitch 和 SwIn[] 或 SwOut[] 一起使用以生成完整的 Universe 地址。除非位 7 为高，否则忽略此值。即要编程值 0x07，将值发送为 0x87。发送 0x00 将此值重置为物理开关设置。 | |
| 6 | BindIndex | Int8 | BindIndex 定义了发起此数据包的绑定节点，当使用相同的 IP 地址时，用于唯一标识绑定节点。此数字代表绑定设备的顺序。数字越小表示越接近根设备。值 1 表示根设备。 | |
| 7 | Short Name [18] | Int8 | 数组代表节点的空终止短名称。控制器使用 ArtAddress 数据包编程此字符串。最大长度为 17 个字符加空字符。如果字符串为空，节点将忽略此值。这是一个固定长度字段，尽管它包含的字符串可以比字段短。 | |
| 8 | Long Name [64] | Int8 | 数组代表节点的空终止长名称。控制器使用 ArtAddress 数据包编程此字符串。最大长度为 63 个字符加空字符。如果字符串为空，节点将忽略此值。这是一个固定长度字段，尽管它包含的字符串可以比字段短。 | |
| 9 | SwIn [4] | Int8 | 给定输入端口的 15 位端口地址的位 3-0 编码到此字段的低 4 位。这结合 NetSwitch 和 SubSwitch 一起使用以生成完整的 Universe 地址。除非位 7 为高，否则忽略此值。即要编程值 0x07，将值发送为 0x87。发送 0x00 将此值重置为物理开关设置。 | |
| 10 | SwOut [4] | Int8 | 给定输出端口的 15 位端口地址的位 3-0 编码到此字段的低 4 位。这结合 NetSwitch 和 SubSwitch 一起使用以生成完整的 Universe 地址。除非位 7 为高，否则忽略此值。即要编程值 0x07，将值发送为 0x87。发送 0x00 将此值重置为物理开关设置。 | |
| 11 | SubSwitch | Int8 | 15 位端口地址的位 7-4 编码到此字段的低 4 位。这结合 NetSwitch 和 SwIn[] 或 SwOut[] 一起使用以生成完整的 Universe 地址。除非位 7 为高，否则忽略此值。即要编程值 0x07，将值发送为 0x87。发送 0x00 将此值重置为物理开关设置。 | |
| 12 | AcnPriority | Int8 | 设置编码到此数据包中的所有 4 个端口生成的 sACN 的 sACN 优先级字段。值 255 表示无更改。包含 0 到 200 在内的值有效。 | |
| 13 | Command | Int8 | 节点配置命令： | |
| | | 值 | 助记符 | 动作 |
| | | 0x00 | AcNone | 无动作 |
| | | 0x01 | AcCancel Merge | 如果节点当前处于合并模式，则在收到下一个 ArtDmx 数据包时取消合并模式。请参阅合并模式操作的讨论。 |
| | | 0x02 | AcLedNormal | 节点的前面板指示灯正常操作。 |
| | | 0x03 | AcLedMute | 节点的前面板指示灯被禁用并关闭。 |
| | | 0x04 | AcLedLocate | 节点前面板指示灯快速闪烁。旨在用于大型安装的出口标识。 |
| | | 0x05 | AcResetRx Flags | 重置节点的 SIP、文本、测试和数据错误标志。如果正在标记输出短路，则强制重新运行测试。 |
| | | 0x06 | AcAnalysisOn | 启用分析和调试模式。 |
| | | 0x07 | AcAnalysisOff | 禁用分析和调试模式。 |
| | | 0x08 | AcFailHold | 设置节点在网络数据丢失时保持最后状态。 |
| | | 0x09 | AcFailZero | 设置节点在网络数据丢失时输出归零。 |
| | | 0x0a | AcFailFull | 设置节点在网络数据丢失时输出满值。 |
| | | 0x0b | AcFailScene | 设置节点在网络数据丢失时播放故障安全场景。 |
| | | 0x0c | AcFailRecord | 记录当前输出状态作为故障安全场景。 |
| | | 0x10 | AcMergeLtp0 | 将 DMX 端口 0 设置为 LTP 模式合并。 |
| | | 0x11 | AcMergeLtp1 | 将 DMX 端口 1 设置为 LTP 模式合并。 |
| | | 0x12 | AcMergeLtp2 | 将 DMX 端口 2 设置为 LTP 模式合并。 |
| | | 0x13 | AcMergeLtp3 | 将 DMX 端口 3 设置为 LTP 模式合并。 |
| | | 0x20 | AcDirectionTx0 | 将端口 0 方向设置为输出。 |
| | | 0x21 | AcDirectionTx1 | 将端口 1 方向设置为输出。 |
| | | 0x22 | AcDirectionTx2 | 将端口 2 方向设置为输出。 |
| | | 0x23 | AcDirectionTx3 | 将端口 3 方向设置为输出。 |
| | | 0x30 | AcDirectionRx0 | 将端口 0 方向设置为输入。 |
| | | 0x31 | AcDirectionRx1 | 将端口 1 方向设置为输入。 |
| | | 0x32 | AcDirectionRx2 | 将端口 2 方向设置为输入。 |
| | | 0x33 | AcDirectionRx3 | 将端口 3 方向设置为输入。 |
| | | 0x50 | AcMergeHtp0 | 将 DMX 端口 0 设置为 HTP（默认）模式合并。 |
| | | 0x51 | AcMergeHtp1 | 将 DMX 端口 1 设置为 HTP（默认）模式合并。 |
| | | 0x52 | AcMergeHtp2 | 将 DMX 端口 2 设置为 HTP（默认）模式合并。 |
| | | 0x53 | AcMergeHtp3 | 将 DMX 端口 3 设置为 HTP（默认）模式合并。 |
| | | 0x60 | AcArtNetSel0 | 将 DMX 端口 0 设置为从 Art-Net 协议输出 DMX512 和 RDM 数据包（默认）。 |
| | | 0x61 | AcArtNetSel1 | 将 DMX 端口 1 设置为从 Art-Net 协议输出 DMX512 和 RDM 数据包（默认）。 |
| | | 0x62 | AcArtNetSel2 | 将 DMX 端口 2 设置为从 Art-Net 协议输出 DMX512 和 RDM 数据包（默认）。 |
| | | 0x63 | AcArtNetSel3 | 将 DMX 端口 3 设置为从 Art-Net 协议输出 DMX512 和 RDM 数据包（默认）。 |
| | | 0x70 | AcAcnSel0 | 将 DMX 端口 0 设置为从 sACN 协议输出 DMX512 数据，并从 Art-Net 协议输出 RDM 数据。 |
| | | 0x71 | AcAcnSel1 | 将 DMX 端口 1 设置为从 sACN 协议输出 DMX512 数据，并从 Art-Net 协议输出 RDM 数据。 |
| | | 0x72 | AcAcnSel2 | 将 DMX 端口 2 设置为从 sACN 协议输出 DMX512 数据，并从 Art-Net 协议输出 RDM 数据。 |
| | | 0x73 | AcAcnSel3 | 将 DMX 端口 3 设置为从 sACN 协议输出 DMX512 数据，并从 Art-Net 协议输出 RDM 数据。 |
| | | 0x90 | AcClearOp0 | 清除端口 0 的 DMX 输出缓冲区 |
| | | 0x91 | AcClearOp1 | 清除端口 1 的 DMX 输出缓冲区 |
| | | 0x92 | AcClearOp2 | 清除端口 2 的 DMX 输出缓冲区 |
| | | 0x93 | AcClearOp3 | 清除端口 3 的 DMX 输出缓冲区 |
| | | 0xa0 | AcStyleDelta0 | 将端口 0 的输出样式设置为增量模式（由 ArtDmx 触发的 DMX 帧） |
| | | 0xa1 | AcStyleDelta1 | 将端口 1 的输出样式设置为增量模式（由 ArtDmx 触发的 DMX 帧） |
| | | 0xa2 | AcStyleDelta2 | 将端口 2 的输出样式设置为增量模式（由 ArtDmx 触发的 DMX 帧） |
| | | 0xa3 | AcStyleDelta3 | 将端口 3 的输出样式设置为增量模式（由 ArtDmx 触发的 DMX 帧） |
| | | 0xb0 | AcStyleConst0 | 将端口 0 的输出样式设置为恒定模式（DMX 输出连续） |
| | | 0xb1 | AcStyleConst1 | 将端口 1 的输出样式设置为恒定模式（DMX 输出连续） |
| | | 0xb2 | AcStyleConst2 | 将端口 2 的输出样式设置为恒定模式（DMX 输出连续） |
| | | 0xb3 | AcStyleConst3 | 将端口 3 的输出样式设置为恒定模式（DMX 输出连续） |
| | | 0xc0 | AcRdmEnabled0 | 为端口 0 启用 RDM |
| | | 0xc1 | AcRdmEnabled1 | 为端口 1 启用 RDM |
| | | 0xc2 | AcRdmEnable2 | 为端口 2 启用 RDM |
| | | 0xc3 | AcRdmEnable3 | 为端口 3 启用 RDM |
| | | 0xd0 | AcRdmDisabled0 | 为端口 0 禁用 RDM |
| | | 0xd1 | AcRdmDisabled1 | 为端口 1 禁用 RDM |
| | | 0xd2 | AcRdmDisable2 | 为端口 2 禁用 RDM |
| | | 0xd3 | AcRdmDisable3 | 为端口 3 禁用 RDM |

### ArtDiagData

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 按 ArtPoll 定义。 |
| | 广播 | 按 ArtPoll 定义。 |
| 节点 | 接收 | 无动作 |
| | 单播发送 | 按 ArtPoll 定义。 |
| | 广播 | 按 ArtPoll 定义。 |
| 媒体服务器 | 接收 | 无动作 |
| | 单播发送 | 按 ArtPoll 定义。 |
| | 广播 | 按 ArtPoll 定义。 |

ArtDiagData 是一个通用数据包，允许节点或控制器发送诊断数据进行显示。

控制器发送的 ArtPoll 数据包定义了这些消息应发送到的目标。

#### ArtDiagData 数据包定义

##### ArtDiagData

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpDiagData，先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Filler1 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 6 | DiagPriority | Int8 | - | 此诊断数据的优先级。参见表 5。 |
| 7 | LogicalPort | Int8 | - | 消息相关的产品的逻辑 DMX 端口。通用消息设置为零。此字段仅为信息性，旨在允许开发工具过滤诊断信息。 |
| 8 | Filler3 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 9 | LengthHi | Int8 | - | 下面文本数组的长度。高字节。 |
| 10 | LengthLo | Int8 | - | 低字节。 |
| 11 | Data | Int8 | - | ASCII 文本数组，空终止。最大长度为 [长度] 512 字节，包括空终止符。 |

##### 表 5 – 优先级代码

下表详细说明了诊断优先级代码。这些用于 ArtPoll 和 ArtDiagData。

| 代码 | 助记符 | 说明 |
|---|---|---|
| 0x10 | DpLow | 低优先级消息。 |
| 0x40 | DpMed | 中等优先级消息。 |
| 0x80 | DpHigh | 高优先级消息。 |
| 0xe0 | DpCritical | 关键优先级消息。 |
| 0xf0 | DpVolatile | 易失性消息。此类消息显示在 DMX-Workshop 诊断显示的单行上。所有其他类型显示在列表框中。 |

### ArtTimeCode

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 节点 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 媒体服务器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |

ArtTimeCode 允许通过网络传输时间码。数据格式兼容纵向时间码和 MIDI 时间码。电影、EBU、丢帧和 SMPTE 四种关键类型也被编码。

数据包的使用是应用程序特定的，但通常单个控制器会向网络广播该数据包。

#### ArtTimeCode 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpTimeCode 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Filler1 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 6 | Filler2 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 7 | Frames | Int8 | - | 帧时间。根据模式为 0 – 29。 |
| 8 | Seconds | Int8 | - | 秒。0 - 59。 |
| 9 | Minutes | Int8 | - | 分钟。0 - 59。 |
| 10 | Hours | Int8 | - | 小时。0 - 23。 |
| 11 | Type | Int8 | - | 0 = 电影 (24fps) |
| | | | | 1 = EBU (25fps) |
| | | | | 2 = DF (29.97fps) |
| | | | | 3 = SMPTE (30fps) |

### ArtCommand

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 节点 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 媒体服务器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |

ArtCommand 数据包用于发送属性设置样式的命令。数据包可以是单播或广播，具体决定由应用程序特定。

#### ArtCommand 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpCommand 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | EstaManHi | Int8 | - | ESTA 制造商代码。这些代码用于表示设备制造商。它们由 ESTA 分配。此字段可解释为两个 ASCII 字节，代表制造商的首字母。 |
| 6 | EstaManLo | Int8 | - | 上述的高字节 |
| 7 | LengthHi | Int8 | - | 下面文本数组的长度。高字节。 |
| 8 | LengthLo | Int8 | - | 低字节。 |
| 9 | Data [Length] | Int8 | - | ASCII 文本命令字符串，空终止。最大长度为 512 字节，包括空终止符。 |

Data 字段包含命令文本。文本是 ASCII 编码的，以空字符结尾，并且不区分大小写。将 Data 数组大小设置为最大值 512 并用空字符填充未使用的条目是合法的，尽管效率低下。

命令文本可能包含多个命令，并遵循以下语法：

*命令=数据&*

“&”是命令之间的分隔符。另请注意，文本大写是为了可读性；它不区分大小写。

到目前为止，Art-Net 定义了两个命令。预计随着其他制造商注册具有行业相关性的命令，将会添加更多命令。

这些命令应使用 EstaMan = 0xFFFF 传输。

##### 表 6 – ArtCommand 命令

下表详细说明了 ArtCommand 中定义的命令。

| 命令 | 说明 |
|---|---|
| SwoutText | 此命令用于重新编程与 ArtPollReply->Swout 字段关联的标签。语法："SwoutText=Playback&" |
| SwinText | 此命令用于重新编程与 ArtPollReply->Swin 字段关联的标签。语法："SwinText=Record&" |

### ArtTrigger

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 节点 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |
| 媒体服务器 | 接收 | 应用程序特定。 |
| | 单播发送 | 应用程序特定。 |
| | 广播 | 应用程序特定。 |

ArtTrigger 数据包用于向网络发送触发宏。最常见的实现涉及单个控制器向所有其他设备广播。

在某些情况下，控制器可能只想触发单个设备或一小群设备，此时将使用单播。

#### ArtTrigger 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpTrigger，先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Filler1 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 6 | Filler2 | Int8 | - | 接收方忽略，发送方设置为零。 |
| 7 | OemCodeHi | Int8 | - | 应接受此触发的节点的制造商代码（高字节）。 |
| 8 | OemCodeLo | Int8 | - | 应接受此触发的节点的制造商代码（低字节）。 |
| 9 | Key | Int8 | - | 触发键。 |
| 10 | SubKey | Int8 | - | 触发子键。 |
| 11 | Data [512] | Int8[] | - | 有效载荷的解释由 Key 定义。 |

**Key**

Key 是一个 8 位数字，用于定义数据包的目的。此字段的解释取决于 Oem 字段。如果 Oem 字段设置为除 0xffff 之外的值，则 Key 和 SubKey 字段是制造商特定的。

然而，当 Oem 字段 = 0xffff 时，Key、SubKey 和 Payload 的含义由表 7 定义。

**表 7 – ArtTrigger Key 值。**

下表详细说明了 ArtCommand 中定义的命令。

| Key | 名称 | 目的 |
|---|---|---|
| 0 | KeyAscil | SubKey 字段包含一个 ASCII 字符，接收设备应将其视为键盘按键进行处理。（未使用 Payload）。 |
| 1 | KeyMacro | SubKey 字段包含接收设备应执行的宏的编号。（未使用 Payload）。 |
| 2 | KeySoft | SubKey 字段包含一个软键编号，接收设备应将其视为软键键盘按键进行处理。（未使用 Payload）。 |
| 3 | KeyShow | SubKey 字段包含接收设备应运行的表演的编号。（未使用 Payload）。 |
| 4-255 | 未定义 | 未定义 |

**SubKey**

SubKey 是一个 8 位数字。此字段的解释取决于 Oem 字段。如果 Oem 字段设置为除 ffff16 之外的值，则 Key 和 SubKey 字段是制造商特定的。

然而，当 Oem 字段 = ffff16 时，SubKey 字段的含义由上表定义。

**Payload**

Payload 是一个固定长度为 512 字节的 8 位字节数组。此字段的解释取决于 Oem 字段。如果 Oem 字段设置为除 0xffff 之外的值，则 Payload 是制造商特定的。

### ArtDmx

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |
| 节点 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |
| 媒体服务器 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |

ArtDmx 是用于传输 DMX512 数据的数据包。格式对于节点到控制器、节点到节点和控制器到节点是相同的。

数据通过对应于 Universe 设置的 DMX O/P 端口输出。在没有收到 ArtDmx 数据包的情况下，每个 DMX O/P 端口会连续重新传输相同的帧。

在每个输入端口接收到的第一个完整的 DMX 帧被放入一个如上所述的 ArtDmx 数据包中，并作为一个包含相关 Universe 参数的 ArtDmx 数据包传输。每个包含新数据（不同长度或不同内容）的后续 DMX 帧也会作为 ArtDmx 数据包传输。

对于自通电以来尚未接收数据的 DMX512 输入，节点不传输 ArtDmx。

但是，处于活动状态但未发生变化的输入将大约每 4 秒重新传输最后一个有效的 ArtDmx 数据包。（注意：为了满足 Art-Net 和 sACN 的需求，建议 Art-Net 设备实际使用 800 毫秒到 1000 毫秒的重新传输时间）。

发生故障的 DMX 输入将不会继续传输 ArtDmx 数据。

#### 单播订阅

ArtDmx 数据包必须单播给 ArtDmx 数据包中包含的特定 Universe 的订阅者。

发送设备必须定期对网络进行 ArtPoll 轮询，以检测任何已订阅设备的更改。已订阅的节点将在 ArtPollReply 中列出订阅的 Universe。*已订阅意味着在 Swin 或 Swout 数组中列出的任何 Universe。*

如果一个 Universe 没有订阅者，控制器不应发送 ArtDmx。任何情况下都不允许广播。

#### ArtDmx 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpOutput 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Sequence | Int8 | - | 序列号用于确保 ArtDmx 数据包按正确顺序使用。当 Art-Net 通过互联网等媒介传输时，ArtDmx 数据包可能不按顺序到达接收器。此字段在 0x01 到 0xff 范围内递增，以允许接收节点重新排序数据包。将 Sequence 字段设置为 0x00 可禁用此功能。 |
| 6 | Physical | Int8 | - | 输入 DMX512 数据的物理输入端口。此字段由接收设备用于区分来自不同输入端口（或相同 IP 地址上的不同物理端口）生成的具有相同端口地址的数据包，这些数据包需要合并。 |
| 7 | SubUni | Int8 | - | 15 位端口地址的低字节，此数据包的目标地址。 |
| 8 | Net | Int8 | - | 15 位端口地址的高 7 位，此数据包的目标地址。 |
| 9 | LengthHi | Int8 | - | DMX512 数据数组的长度。此值应为 2 – 512 范围内的偶数。它表示数据包中编码的 DMX512 通道数。注意：将 Art-Net 转换为 DMX512 的产品可以选择始终发送 512 个通道。高字节。 |
| 10 | Length | Int8 | - | 上述的低字节。 |
| 11 | Data | Int8 | - | DMX512 照明数据的可变长度数组。 |

**刷新率：**

ArtDmx 数据包旨在传输 DMX512 数据。因此，针对特定 IP 地址的 ArtDmx 数据包的重复传输速率不应超过包含 512 个数据槽的 DMX 数据包的最大重复速率。

**同步数据：**

在视频或媒体墙应用中，同步多个 Universe 的 ArtDmx 数据是有益的。这可以通过 ArtSync 数据包实现。

**数据合并：**

Art-Net 协议允许多个节点或控制器向同一个 Universe 传输 ArtDmx 数据。

节点可以通过比较接收到的 ArtDmx 数据包的 IP 地址来检测这种情况。如果从不同的 IP 地址（或相同 IP 地址上的不同物理端口）接收到针对相同端口地址的 ArtDmx 数据包，则存在潜在冲突。

节点可以通过以下两种方法之一合法地处理这种情况：

- 将其视为错误情况并等待用户干预。
- 自动合并数据。

产品用户指南中应记录产品中实现的方法。合并选项是首选，因为它提供了更高级别的功能。

合并以 ArtAddress 数据包指定的 LTP 或 HTP 模式实现。

合并模式的实现如下：

如果从不同的 IP 地址接收到具有相同端口地址的 ArtDmx，数据将合并到 DMX 输出。在这种情况下，设置 ArtPollReply-GoodOutput-Bit3。如果 Art-Poll-Flags 位 1 已设置，则应在合并开始时传输 ArtPollReply。

如果从相同的 IP 地址但不同的物理字段接收到具有相同端口地址的 ArtDmx，数据将合并到 DMX 输出。在这种情况下，设置 ArtPollReply-GoodOutput-Bit3。如果 Art-Poll-Flags 位 1 已设置，则应在合并开始时传输 ArtPollReply。

退出合并模式的处理方式如下：

如果收到 ArtAddress AcCancelMerge，则下一个收到的 ArtDmx 消息将结束合并模式。然后，节点丢弃从与终止合并模式的 ArtDmx 数据包的 IP 地址不匹配的 IP 地址接收到的任何 ArtDmx 数据包。

如果 ArtDmx 的一个（但不是两个）来源停止，故障来源将在合并缓冲区中保持 10 秒。如果在 10 秒超时期间，故障来源恢复，则合并模式继续。如果故障来源未恢复，则在超时期结束时，节点退出合并模式。

如果两个 ArtDmx 来源都发生故障，输出将保持最后的合并结果。

合并仅限于两个来源，任何其他来源将被节点忽略。

合并实现允许以下两种关键操作模式。

- 组合控制：两个控制器（控制台）可以在网络上运行，并将数据合并到多个节点。
- 备份：一个控制器（控制台）可以监控网络以检测主控制器的故障。如果发生故障，它可以使用 ArtAddress AcCancelMerge 命令立即接管网络控制。

当节点提供多个 DMX512 输入时，由节点负责处理数据合并。这是因为节点将只有一个 IP 地址。如果不在节点处处理，具有相同 IP 地址和相同 Universe 编号但冲突的电平数据的 ArtDmx 数据包将被传输到网络。

### ArtSync

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 定向广播 | 控制器广播此数据包以将先前的 ArtDmx 数据包同步传输到节点的输出。 |
| 节点 | 接收 | 将先前的 ArtDmx 数据包传输到输出。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 将先前的 ArtDmx 数据包传输到输出。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

ArtSync 数据包可用于强制节点同步将 ArtDmx 数据包输出到其输出。这在视频和媒体墙应用中很有用。

希望实现同步传输的控制器将单播多个 Universe 的 ArtDmx，然后广播一个 ArtSync，以便同时将所有 ArtDmx 数据包同步传输到节点的输出。

#### 管理同步和非同步模式

上电或复位时，节点应在非同步模式下运行。这意味着 ArtDmx 数据包将立即处理并输出。

当节点收到 ArtSync 数据包时，它应切换到同步操作。这意味着接收到的 ArtDmx 数据包将被缓冲，并在收到下一个 ArtSync 时输出。

为了允许在同步和非同步模式之间转换，如果超过 4 秒未收到 ArtSync，节点应超时进入非同步操作。

#### ArtSync 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpCode 定义了此 UDP 数据包内的数据类别。先传输低字节。有关 OpCode 列表，请参见表 1。设置为 OpSync。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14。控制器应忽略与使用低于 14 的协议版本的节点的通信。 |
| 5 | Aux1 | Int8 | - | 发送为零。 |
| 6 | Aux2 | Int8 | - | 发送为零。 |

**多个控制器**

为了允许网络上的多个控制器，节点应将 ArtSync 的源 IP 与最近 ArtDmx 数据包的源 IP 进行比较。如果 IP 地址不匹配，则应忽略 ArtSync。

当端口正在合并来自不同 IP 地址的多个 ArtDmx 流时，应忽略 ArtSync 数据包。

### ArtNzs

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |
| 节点 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |
| 媒体服务器 | 接收 | 应用程序特定。 |
| | 单播发送 | 是。 |
| | 广播 | 否。 |

ArtNzs 是用于传输具有非零起始码（RDM 除外）的 DMX512 数据的数据包。格式对于节点到控制器、节点到节点和控制器到节点是相同的。

#### ArtNzs 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | OpNzs 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Sequence | Int8 | - | 序列号用于确保 ArtNzs 数据包按正确顺序使用。当 Art-Net 通过互联网等媒介传输时，ArtNzs 数据包可能不按顺序到达接收器。此字段在 0x01 到 0xff 范围内递增，以允许接收节点重新排序数据包。将 Sequence 字段设置为 0x00 可禁用此功能。 |
| 6 | StartCode | Int8 | - | 此数据包的 DMX512 起始码。不得为零或 RDM。 |
| 7 | SubUni | Int8 | - | 15 位端口地址的低字节，此数据包的目标地址。 |
| 8 | Net | Int8 | - | 15 位端口地址的高 7 位，此数据包的目标地址。 |
| 9 | LengthHi | Int8 | - | 数据数组的长度。此值应为 1 – 512 范围内的数字。它表示数据包中编码的 DMX512 通道数。高字节。 |
| 10 | Length | Int8 | - | 上述的低字节。 |
| 11 | Data [Length] | Int8 | - | DMX512 照明数据的可变长度数组。 |

### ArtVlc

ArtVlc 是 ArtNzs 数据包的一个特定实现，用于通过 Art-Net 传输 VLC（可见光通信）数据。（数据包的有效负载也可用于通过 DMX512 物理层传输 VLC）。

字段 2、6、11、12 和 13 应视为“魔术数字”以检测此数据包。

#### ArtVlc 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | |
| 2 | OpCode | Int16 | - | OpNzs 先传输低字节。 | |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 | |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 | |
| 5 | Sequence | Int8 | - | 序列号用于确保 ArtNzs 数据包按正确顺序使用。当 Art-Net 通过互联网等媒介传输时，ArtNzs 数据包可能不按顺序到达接收器。此字段在 0x01 到 0xff 范围内递增，以允许接收节点重新排序数据包。将 Sequence 字段设置为 0x00 可禁用此功能。 | |
| 6 | StartCode | Int8 | - | 此数据包的 DMX512 起始码设置为 91.16。不允许其他值。 | |
| 7 | SubUni | Int8 | - | 15 位端口地址的低字节，此数据包的目标地址。 | |
| 8 | Net | Int8 | - | 15 位端口地址的高 7 位，此数据包的目标地址。 | |
| 9 | LengthHi | Int8 | - | Vlc 数据数组的长度。此值应在 1 – 512 范围内。它表示数据包中编码的 DMX512 通道数。高字节。 | |
| 10 | Length | Int8 | - | 上述的低字节。 | |
| 11 | Vic [Length] | - | - | VLC 数据的可变长度数组，如下所述： | |
| 11 | Vic[0] MandHi | Int8 | - | 41$_{16}$ 用于标识此数据包的魔术数字。 | |
| 12 | Vic[1] MandLo | Int8 | - | 4C$_{16}$ 用于标识此数据包的魔术数字。 | |
| 13 | Vic[2] SubCode | Int8 | - | 45$_{16}$ 用于标识此数据包的魔术数字。 | |
| 14 | Vic[3] Flags | Int8 | - | 用于控制 VLC 操作的位字段。未使用的位应发送为零。 | |
| | Flags.Ieee | 7 | 如果设置，有效载荷区域中的数据应解释为 IEEE VLC 数据。如果清零，则 PayLanguage 定义有效载荷内容。 | | |
| | Flags.Reply | 6 | 如果设置，这是一个回复数据包，用于响应在事务编号中具有匹配数字的请求：TransHi/Lo。如果清零，则这不是回复。 | | |
| | Flags.Beacon | 5 | 如果设置，发送器应连续重复传输此数据包，直到收到另一个数据包。如果清零，发送器应传输此数据包一次。 | | |
| 15 | Vic[4] TransHi | Int8 | - | 事务编号是一个 16 位值，允许同步 VLC 事务。值 0 表示事务中的第一个数据包。值 ffff$_{16}$ 表示事务中的最后一个数据包。所有其他数据包包含连续的数字，每个数据包递增，并在 fffe$_{16}$ 处回滚到 1。 | |
| 16 | Vic[5] TransLo | Int8 | - | 上述的低字节 | |
| 17 | Vic[6] SlotAddrHi | Int8 | - | 此数据包指向的设备的槽号，范围 1-512。值 0 表示应接受此数据包的所有连接到此数据包端口地址的设备。 | |
| 18 | Vic[7] SlotAddrLo | Int8 | - | 上述的低字节 | |
| 19 | Vic[8] PayCountHi | Int8 | - | 16 位有效载荷大小，范围为 0 到 480$_{10}$。 | |
| 20 | Vic[9] PayCountLo | Int8 | - | 上述的低字节 | |
| 21 | Vic[10] PayCheckHi | Int8 | - | 有效载荷中数据的 16 位无符号加法校验和。 | |
| 22 | Vic[11] PayCheckLo | Int8 | - | 上述的低字节 | |
| 23 | Vic[12] Spare1 | Int8 | - | 发送为零，接收方不检查。 | |
| 24 | Vic[13] VicDepth | Int8 | - | 8 位 VLC 调制深度，以百分比表示，范围 1 到 100。值 0 表示发送器应使用其默认值。 | |
| 25 | Vic[14] VicFreqHi | Int8 | - | VLC 发送器的 16 位调制频率，以赫兹表示。值 0 表示发送器应使用其默认值。 | |
| 26 | Vic[15] VicFreqLo | Int8 | - | 上述的低字节 | |
| 27 | Vic[16] VicModHi | Int8 | - | 发送器应用于传输 VLC 的 16 位调制类型编号。0000$_{16}$ – 使用发送器默认值。 | |
| 28 | Vic[17] VicModLo | Int8 | - | 上述的低字节 | |
| 29 | Vic[18] PayLangHi | Int8 | - | 16 位有效载荷语言代码。当前注册的值：0000$_{16}$ – BeaconURL – 有效载荷包含一个代表 URL 的简单文本字符串。 | |
| 30 | Vic[19] PayLangLo | Int8 | - | 0001$_{16}$ – BeaconText – 有效载荷包含一条简单的 ASCII 文本消息。0002$_{16}$ – BeaconLocationID – 有效载荷包含一个大端 16 位数字。 | |
| 31 | Vic[20] BeachRepHi | Int8 | - | 上述的低字节 | |
| 32 | Vic[21] BeachRepLo | Int8 | - | 16 位信标模式重复频率。如果 Flags.Beacon 已设置，此 16 位值表示应重复 VLC 数据包的频率，单位为赫兹。0000$_{16}$ – 使用发送器默认值。 | |
| 33 | Vic[22] Payload | 变量 | - | 上述的低字节 | |
| 34 | Vic[23] BeachRepLo | 变量 | - | 实际的 VLC 有效载荷。 | |

### ArtInput

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 控制器传输到特定节点 IP 地址。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 用 ArtPollReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 用 ArtPollReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

网络上的控制器或监控设备可以启用或禁用任何网络节点上的单个 DMX512 输入。这允许控制器直接控制网络流量，并确保未使用的输入被禁用，从而不浪费带宽。

所有节点上电时所有输入均已启用。

在控制器中实现此功能时应小心。请记住，某些网络流量可能在节点之间运行。

#### ArtInput 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 |
| 2 | OpCode | Int16 | - | Opinput 先传输低字节。 |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 |
| 5 | Filler1 | Int8 | - | 填充长度以匹配 ArtPoll。 |
| 6 | BindIndex | Int8 | - | BindIndex 定义了发起此数据包的绑定节点，当使用相同的 IP 地址时，用于唯一标识绑定节点。此数字代表绑定设备的顺序。数字越小表示越接近根设备。值 1 表示根设备。 |
| 7 | NumPortsHi | Int8 | - | 描述输入或输出端口数量的字的高字节。高字节用于未来扩展，当前为零。 |
| 8 | NumPortsLo | Int8 | - | 描述输入或输出端口数量的字的低字节。如果输入数量不等于输出数量，则取最大值。最大值为 4。 |
| 9 | Input [4] | Int8 | - | 此数组定义每个通道的输入禁用状态。（示例 = 0x01、0x00、0x01、0x00 以禁用第一个和第三个输入） |
| | | | 7-1 | 当前未使用 |
| | | | 0 | 设置以禁用此输入。 |

### 固件和 UBEA 升级

本节定义了用于向节点发送固件修订版的数据包。在所有情况下，通信都是私有的。任何情况下都不应使用广播地址。

事务涉及控制器向节点的 IP 地址发送多个 ArtFirmwareMaster 数据包。每个数据包由节点使用 ArtFirmwareReply 进行确认。

控制器允许最多 30 秒的延迟来接收 ArtFirmwareReply。

如果在此时间内未收到回复，控制器将中止事务。较长的时间段是为了允许节点直接写入慢速非易失性存储器。

节点允许在发送 ArtFirmwareReply 和接收下一个连续的 ArtFirmwareMaster 之间有 30 秒的延迟。如果在此时间内未收到下一个连续块，节点将中止事务。在这种情况下，节点返回到其先前的操作系统，并相应地设置 ArtPollReply->Status 和 ArtPollReply ->NodeReport。

固件更新文件包含一个标头，定义了此更新有效的节点 OEM 值。控制器在发送到节点之前必须检查此值。节点还在收到第一个数据包时检查此数据。如果节点收到具有无效代码的数据包，它会发送错误响应。

UBEA 是用户 BIOS 扩展区。这是一种有限的固件上传机制，允许向节点添加第三方固件扩展。

实现此功能的制造商必须记录软件接口要求。

### ArtFirmwareMaster

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 控制器传输到特定节点 IP 地址。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 使用 OpFirmwareReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 使用 OpFirmwareReply 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

#### ArtFirmwareMaster 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | |
| 2 | OpCode | Int16 | - | OpFirmwareMaster。先传输低字节。 | |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 | |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 | |
| 5 | Filler1 | Int8 | - | 填充长度以匹配 ArtPoll。 | |
| 6 | Filler2 | Int8 | - | 填充长度以匹配 ArtPoll。 | |
| 7 | Type | Int8 | - | 定义数据包内容如下： | |
| | | | 值 | 助记符 | 功能 |
| | | | 0x00 | FirmFirst | 固件上传的第一个数据包。 |
| | | | 0x01 | FirmCont | 固件上传的连续延续数据包。 |
| | | | 0x02 | FirmLast | 固件上传的最后一个数据包。 |
| | | | 0x03 | UbeaFirst | UBEA 上传的第一个数据包。 |
| | | | 0x04 | UbeaCont | UBEA 上传的连续延续数据包。 |
| | | | 0x05 | UbeaLast | UBEA 上传的最后一个数据包。 |
| 8 | BlockId | Int8 | - | 计算固件上传的连续块数。从 FirmFirst 或 UbeaFirst 数据包的 0x00 开始。 | |
| 9 | Firmware Length3 | Int8 | - | 此 Int64 参数描述固件上传的总字数（Int16）加上固件标头大小。例如，32K 字上传加上 530 字的标头信息 == 0x00008212。此值也是要上传的文件大小（以字为单位）。 | |
| 10 | Firmware Length2 | Int8 | - | | |
| 11 | Firmware Length1 | Int8 | - | | |
| 12 | Firmware Length0 | Int8 | - | 最低有效字节 | |
| 13 | Spare[20] | Int8 | - | 控制器设置为零，节点不测试。 | |
| 14 | Data[512] | Int16 | - | 此数组包含固件或 UBEA 数据块。顺序为先高字节。此数据的解释是制造商特定的。如果所需字节少于 512，则最终数据包应使用空值填充。 | |

### ArtFirmwareReply

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 发送下一个 OpFirmwareMaster。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 节点 | 接收 | 无动作。 |
| | 单播发送 | 节点传输到特定控制器 IP 地址。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 节点传输到特定控制器 IP 地址。 |
| | 广播 | 不允许。 |

此数据包由节点发送给控制器，以确认每个 OpFirmwareMaster 数据包。

#### ArtFirmwareReply 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | |
| 2 | OpCode | Int16 | - | OpFirmwareReply。先传输低字节。 | |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 | |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 | |
| 5 | Filler1 | Int8 | - | 填充长度以匹配 ArtPoll。 | |
| 6 | Filler2 | Int8 | - | 填充长度以匹配 ArtPoll。 | |
| 7 | Type | Int8 | - | 定义数据包内容如下。代码用于固件和 UBEA。 | |
| | | | 值 | 助记符 | 功能 |
| | | | 0x00 | FirmBlockGood | 最后一个数据包接收成功。 |
| | | | 0x01 | FirmAllGood | 所有固件接收成功。 |
| | | | 0xff | FirmFail | 固件上传失败。（所有错误情况）。 |
| 8 | Spare[21] | Int8 | - | 节点设置为零，控制器不测试。 | |

**固件文件格式：**

所有固件和 UBEA 上传文件应具有以下格式。

固件文件扩展名为 .alf。

UBEA 文件扩展名为 .alu。

| 字节 | 名称 | 说明 |
|---|---|---|
| 1 | ChecksumHi | 这是固件数据区域的 16 位反码校验和。 |
| 2 | ChecksumLo | 上述的最低有效字节 |
| 3 | VersInfoHi | 节点固件修订号的高字节。控制器应仅使用此字段决定是否应进行固件更新。惯例是数字越大表示固件版本越新。 |
| 4 | VersInfoLo | 上述的最低有效字节 |
| 5-34 | UserName | 30 字节的用户名信息字段。节点不检查此信息。它纯粹供控制器显示。它应包含文件版本号的人类可读描述。虽然这是一个固定长度字段，但它必须包含空终止符。 |
| 35-546 | Oem[256] | 256 个字的数组。每个字先高字节，代表此文件有效的 Oem 代码。未使用的条目必须用 0x0000 填充。 |
| 547-1056 | Spare[255] | 255 个字的数组。当前未使用，应设置为零。 |
| 1057 | Length3 | 此字段之后的固件信息的总长度（以字为单位）。 |
| 1058 | Length2 | |
| 1059 | Length1 | |
| 1060 | Length0 | 最低有效字节 |
| 1061 | Data[] | 固件数据为 16 位值的数组，先高字节排序。实际数据是制造商特定的。 |

## RDM 支持

本节定义了用于通过 Art-Net 网关远程设备管理 (RDM) 协议的数据包结构。假定读者熟悉 RDM 文档。

Art-Net 设备支持 RDM 如下：

- 所有 RDM 发现命令都是代理的；Art-Net 设备维护本地 RDM 设备列表并进行自己的发现。
- 所有 RDM Get / Set 命令是非代理的；它们传递给终端设备进行响应。

本文档定义了以下术语：

**输入网关：** 将 DMX512 输入到 Art-Net 网络的设备（例如 Art-Lynx IP）。

**输出网关：** 从 Art-Net 网络输出 DMX512 的设备（例如 Art-Lynx OP）

**设备表 (TOD)：** 输入和输出网关都维护的 RDM 设备列表。

### RDM 发现

#### 输出网关操作

输出网关独立于网络操作执行 RDM 发现。这包括上电时的完全发现和作为后台任务的增量发现。输出网关按如下方式通知网络其 TOD：

收到 ArtTodRequest 数据包后，输出网关定向广播包含整个 TOD 的 ArtTodData 数据包。所有输入网关解析 ArtTodData 数据包。如果子网和 Universe 字段匹配，输入网关将 TOD 内容添加到它们自己的内部 TOD 中。这允许输入网关响应它们收到的任何物理层 RDM 发现命令。

完成初始 RDM 发现后，输出网关在其 ArtTodData 数据包中定向广播它们的 TOD。

当 RDM 设备在增量发现过程中被添加到输出网关的 TOD 或从中移除时，会自动广播 ArtTodData 数据包。

#### 输入网关操作

输入网关通过监控 Art-Net 流量生成 TOD。然后使用 TOD 通过代理回复 RDM 发现命令。操作如下：

上电时，输入网关定向广播 ArtTodRequest 数据包。

监控网络中的 ArtTodData 数据包。如果子网和 Universe 字段匹配，输入网关将 TOD 内容添加到其自己的内部 TOD 中。这允许输入网关响应它们收到的任何 RDM 发现命令。

输入网关不向网络传输任何 RDM 发现消息。

#### 控制器操作

控制器模拟输入网关的操作。

### ArtTodRequest

此数据包用于请求 RDM 设备表 (TOD)。接收此数据包的节点不得将其解释为强制完全发现。完全发现仅在上电或收到 ArtTodControl.AttFlush 时启动。响应是 ArtTodData。

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 控制器定向广播到所有节点。 |
| 节点输出网关 | 接收 | 使用 ArtTodData 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 节点输入网关 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 输入网关定向广播到所有节点。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

#### ArtTodRequest 数据包定义

| 字段 | 名称 | 大小 | 位 | 说明 | | |
|---|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | - | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | | |
| 2 | OpCode | Int16 | - | OpTodRequest。先传输低字节。 | | |
| 3 | ProtVerHi | Int8 | - | Art-Net 协议修订号的高字节。 | | |
| 4 | ProtVerLo | Int8 | - | Art-Net 协议修订号的低字节。当前值 14 | | |
| 5 | Filler1 | Int8 | - | 填充长度以匹配 ArtPoll。 | | |
| 6 | Filler2 | Int8 | - | 填充长度以匹配 ArtPoll。 | | |
| 7 | Spare1 | Int8 | - | 发送为零，接收方不测试。 | | |
| 8 | Spare2 | Int8 | - | 发送为零，接收方不测试。 | | |
| 9 | Spare3 | Int8 | - | 发送为零，接收方不测试。 | | |
| 10 | Spare4 | Int8 | - | 发送为零，接收方不测试。 | | |
| 11 | Spare5 | Int8 | - | 发送为零，接收方不测试。 | | |
| 12 | Spare6 | Int8 | - | 发送为零，接收方不测试。 | | |
| 13 | Spare7 | Int8 | - | 发送为零，接收方不测试。 | | |
| 14 | Net | Int8 | - | 必须响应此数据包的节点的 15 位端口地址的高 7 位。 | | |
| 15 | Command | Int8 | - | 值 | 助记符 | 功能 |
| | | | | 0x00 | TodFull | 发送整个 TOD。 |
| 16 | AddCount | Int8 | - | Address 中使用的条目数。最大值为 32。 | | |
| 17 | Address [32] | Int8 | - | 此数组定义了必须响应此数据包的输出网关节点的端口地址的低字节。这与上面的“Net”字段结合形成 15 位地址。 | | |

### ArtTodData

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 节点输出网关 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 输出网关始终定向广播此数据包。 |
| 节点输入网关 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

#### ArtTodData 数据包定义

| 字段 | 名称 | 大小 | 说明 | | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | | |
| 2 | OpCode | Int16 | OpTodData。先传输低字节。 | | |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 | | |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。当前值 14 | | |
| 5 | RdmVer | Int8 | 仅支持 RDM 草案 V1.0 的 Art-Net 设备将字段设置为 0x00。支持 RDM 标准 V1.0 的设备将字段设置为 0x01。 | | |
| 6 | Port | Int8 | 物理端口索引。范围 1-4。此数字与 BindIndex 结合使用，以标识生成数据包的物理端口。这是通过引用具有匹配 BindIndex 的 ArtPollReply 中的数据来完成的：ArtPollReplyData->BindIndex == ArtTodData->BindIndex | | |
| | | | ArtPollReply 可以编码 1 到 4 个物理端口，由 ArtPollReply->NumPortsLo 定义。计算物理端口时必须使用此数字，以允许可变编码。计算如下： | | |
| | | | 物理端口 = (BindIndex-1) * ArtPollReply->NumPortsLo + ArtTodData->Port | | |
| | | | 由于大多数现代 Art-Net 网关在每个 ArtPollReply 中实现一个 Universe，ArtTodData->Port 通常设置为值 1。 | | |
| 7 | Spare1 | Int8 | 发送为零，接收方不测试。 | | |
| 8 | Spare2 | Int8 | 发送为零，接收方不测试。 | | |
| 9 | Spare3 | Int8 | 发送为零，接收方不测试。 | | |
| 10 | Spare4 | Int8 | 发送为零，接收方不测试。 | | |
| 11 | Spare5 | Int8 | 发送为零，接收方不测试。 | | |
| 12 | Spare6 | Int8 | 发送为零，接收方不测试。 | | |
| 13 | BindIndex | Int8 | BindIndex 定义了发起此数据包的绑定节点。结合 Port 和源 IP 地址，它唯一标识了发送方。这必须与 ArtPollReply 中的 BindIndex 字段匹配。此数字代表绑定设备的顺序。数字越小表示越接近根设备。值 1 表示根设备。 | | |
| 14 | Net | Int8 | 生成此数据包的输出网关 DMX 端口的端口地址的高 7 位。 | | |
| 15 | Command Response | Int8 | 定义数据包内容如下。 | | |
| | | | 值 | 助记符 | 功能 |
| | | | 0x00 | TodFull | 数据包包含整个 TOD，或者是包含整个 TOD 的数据包序列中的第一个数据包。 |
| | | | 0xff | TodNak | TOD 不可用或发现未完成。 |
| 16 | Address | Int8 | 生成此数据包的输出网关 DMX 端口的端口地址的低 8 位。高半字节是子网开关。低半字节对应于 Universe。 | | |
| 17 | UidTotalHi | Int8 | 此 Universe 发现的 RDM 设备总数。 | | |
| 18 | UidTotalLo | Int8 | | | |
| 19 | BlockCount | Int8 | 此数据包的索引号。当 UidTotal 超过 200 时，使用多个 ArtTodData 数据包。BlockCount 对于第一个数据包设置为零，并为包含 TOD 信息块的每个后续数据包递增。 | | |
| 20 | UidCount | Int8 | 此数据包中编码的 UID 数量。这是以下数组的索引。 | | |
| 21 | ToD | 48 位 | RDM UID 数组。 | | |

### ArtTodControl

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 允许。 |
| | 广播 | 控制器定向广播到所有节点。 |
| 节点输出网关 | 接收 | 使用 ArtTodData 回复。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |
| 节点输入网关 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 输入网关定向广播到所有节点。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

ArtTodControl 数据包用于通过 Art-Net 发送 RDM 控制参数。响应是 ArtTodData。

#### ArtTodControl 数据包定义

| 字段 | 名称 | 大小 | 说明 | | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | | |
| 2 | OpCode | Int16 | OpTodControl。先传输低字节。 | | |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 | | |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。当前值 14 | | |
| 5 | Filler1 | Int8 | 填充长度以匹配 ArtPoll。 | | |
| 6 | Filler2 | Int8 | 填充长度以匹配 ArtPoll。 | | |
| 7 | Spare1 | Int8 | 发送为零，接收方不测试。 | | |
| 8 | Spare2 | Int8 | 发送为零，接收方不测试。 | | |
| 9 | Spare3 | Int8 | 发送为零，接收方不测试。 | | |
| 10 | Spare4 | Int8 | 发送为零，接收方不测试。 | | |
| 11 | Spare5 | Int8 | 发送为零，接收方不测试。 | | |
| 12 | Spare6 | Int8 | 发送为零，接收方不测试。 | | |
| 13 | Spare7 | Int8 | 发送为零，接收方不测试。 | | |
| 14 | Net | Int8 | 应执行此命令的输出网关 DMX 端口的端口地址的高 7 位。 | | |
| 15 | Command | Int8 | 定义数据包操作。 | | |
| | | | 值 | 助记符 | 功能 |
| | | | 0x00 | AtcNone | 无动作。 |
| | | | 0x01 | AtcFlush | 节点刷新其 TOD 并启动完全发现。 |
| 16 | Address | Int8 | 应执行此命令的 DMX 端口的 15 位端口地址的低字节。 | | |

### ArtRdm

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 节点输出网关 | 接收 | 无动作 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 节点输入网关 | 接收 | 无动作。 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |

ArtRdm 数据包用于通过 Art-Net 传输所有非发现的 RDM 消息。

#### ArtRdm 数据包定义

| 字段 | 名称 | 大小 | 说明 | | |
|---|---|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | | |
| 2 | OpCode | Int16 | OpRdm。先传输低字节。 | | |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 | | |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。当前值 14 | | |
| 5 | RdmVer | Int8 | 仅支持 RDM 草案 V1.0 的 Art-Net 设备将字段设置为 0x00。支持 RDM 标准 V1.0 的设备将字段设置为 0x01。 | | |
| 6 | Filler2 | Int8 | 填充长度以匹配 ArtPoll。 | | |
| 7 | Spare1 | Int8 | 发送为零，接收方不测试。 | | |
| 8 | Spare2 | Int8 | 发送为零，接收方不测试。 | | |
| 9 | Spare3 | Int8 | 发送为零，接收方不测试。 | | |
| 10 | Spare4 | Int8 | 发送为零，接收方不测试。 | | |
| 11 | Spare5 | Int8 | 发送为零，接收方不测试。 | | |
| 12 | Spare6 | Int8 | 发送为零，接收方不测试。 | | |
| 13 | Spare7 | Int8 | 发送为零，接收方不测试。 | | |
| 14 | Net | Int8 | 应执行此命令的 15 位端口地址的高 7 位。 | | |
| 15 | Command | Int8 | 定义数据包操作。 | | |
| | | | 值 | 助记符 | 功能 |
| | | | 0x00 | ArProcess | 处理 RDM 数据包。 |
| 16 | Address | Int8 | 应执行此命令的端口地址的低 8 位。 | | |
| 17 | RdmPacket | Int8 [变量] | RDM 数据包，不包括 DMX 起始码。 | | |

### ArtRdmSub

数据包策略。

| 实体 | 方向 | 动作 |
|---|---|---|
| 控制器 | 接收 | 无动作。 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 节点输出网关 | 接收 | 无动作 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 节点输入网关 | 接收 | 无动作。 |
| | 单播发送 | 是。 |
| | 广播 | 不允许。 |
| 媒体服务器 | 接收 | 无动作。 |
| | 单播发送 | 不允许。 |
| | 广播 | 不允许。 |

ArtRdmSub 数据包用于将 Get、Set、GetResponse 和 SetResponse 数据传输到 RDM 设备内的多个子设备以及从这些设备接收数据。此数据包主要由代理或模拟 RDM 的 Art-Net 设备使用。与发送多个 ArtRdm 数据包的方法相比，它提供了显著的带宽增益。

请注意，此数据包是在 Art-Net II 发布时添加的。为了向后兼容，仅允许在 ArtRdm 之外实现此数据包。它不能代替 ArtRdm 使用。

#### ArtRdmSub 数据包定义

| 字段 | 名称 | 大小 | 说明 | |
|---|---|---|---|---|
| 1 | ID[8] | Int8 | 8 个字符的数组，最后一个字符是空终止符。值 = 'A' 'r' 't' '-' 'N' 'e' 't' 0x00 | |
| 2 | OpCode | Int16 | OpRdmSub。先传输低字节。 | |
| 3 | ProtVerHi | Int8 | Art-Net 协议修订号的高字节。 | |
| 4 | ProtVerLo | Int8 | Art-Net 协议修订号的低字节。当前值 14 | |
| 5 | RdmVer | Int8 | 仅支持 RDM 草案 V1.0 的 Art-Net 设备将字段设置为 0x00。支持 RDM 标准 V1.0 的设备将字段设置为 0x01。 | |
| 6 | Filler2 | Int8 | 发送为零，接收方不测试。 | |
| 7 | UID | Int8[6] | 目标 RDM 设备的 UID。 | |
| 8 | Spare1 | Int8 | 发送为零，接收方不测试。 | |
| 9 | CommandClass | Int8 | 根据 RDM 规范。此字段定义这是 Get、Set、GetResponse 还是 SetResponse。 | |
| 10 | ParameterId | Int16 | 根据 RDM 规范。此字段定义此数据包中包含的参数类型。大端序。 | |
| 11 | SubDevice | Int16 | 定义数据包中包含的第一个设备信息。这遵循 RDM 约定，0 = 根设备，1 = 第一个子设备。大端序。 | |
| 12 | SubCount | Int16 | 打包到数据包中的子设备数量。零是非法的。大端序。 | |
| 13 | Spare2 | Int8 | 发送为零，接收方不测试。 | |
| 14 | Spare3 | Int8 | 发送为零，接收方不测试。 | |
| 15 | Spare4 | Int8 | 发送为零，接收方不测试。 | |
| 16 | Spare5 | Int8 | 发送为零，接收方不测试。 | |
| 17 | Data | Int16 | 打包的 16 位大端序数据。数据数组的大小由 CommandClass 和 SubCount 的内容定义： | |
| | | | CommandClass | 数组大小 |
| | | | Get | 0 |
| | | | Set | SubCount |
| | | | GetResponse | SubCount |
| | | | SetResponse | 0 |

## 状态显示

大多数符合 Art-Net 标准的设备将提供某种级别的状态指示。建议使用以下格式：

| 名称 | 助记符 | 功能 |
|---|---|---|
| 电源 | Pow | 通常亮起，如果检测到故障则闪烁。 |
| 通信 | Com | 如果检测到网络上的任何 Art-Net 数据包，则亮起，6 秒后超时。 |
| DMX512 输入 | DMX | 如果接收到良好的 DMX，则亮起。如果检测到错误，则闪烁。其他起始码不是错误！ |
| DMX512 输出 | DMX | 如果为此输出接收 ArtDmx，则亮起。6 秒后超时。 |

## 数据完整性

Art-Net 接收器应检查一项：

比较 ID[8] 字段

© Artistic Licence. 1998-2022

E: <Support@ArtisticLicence.com>

W: <http://www.ArtisticLicence.com>

W: <www.Art-Net.info>

请注意，虽然 Art-Net SDK、Art-Net View & DMX-Workshop 是免费的，但它们不是“免费软件”，版权仍归 Artistic Licence 所有。未经 Artistic Licence 明确书面许可，不得将其包含在商业产品中或通过互联网提供。

本文档中包含的信息如有更改，恕不另行通知。Artistic Licence. 对此材料不作任何形式的保证，包括但不限于针对特定用途的适销性和适用性的暗示保证。

Artistic Licence 对本文中的错误或与此材料的提供、性能或使用相关的附带或后果性损害概不负责。

所有商标均得到承认。
