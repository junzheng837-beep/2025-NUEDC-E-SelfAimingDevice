# 2026-NUEDC-MSPM0-Car

> 🏎️ 2026年全国大学生电子设计竞赛（NUEDC）备赛核心代码库
>
> 基于 **TI MSPM0G3507** 的智能小车控制系统底层框架与驱动源码。

## 📑 目录结构与分层架构

本项目采用了清晰的分层设计，以降低代码耦合度，方便团队成员分工协作：

### 1. 业务应用层 (`/app`)
负责具体的功能逻辑和交互实现。
* `app_lcd`: LCD 屏幕UI显示逻辑与页面管理。
* `app_protocol`: 顶层通信协议处理与指令执行。

### 2. 硬件驱动层 (`/Hardware`)
负责单片机外设与具体传感器的底层交互。
* **运动控制**: 
  * `motor_ctrl`: 直流电机驱动与 PWM 控制。
  * `Encoder`: 编码器脉冲捕获与速度/里程计算。
* **姿态与感知**: 
  * `bsp_gyro` / `mpu6050`: MPU6050 六轴陀螺仪数据读取与姿态解算。
  * `gw_gray`: 灰度传感器阵列读取（用于自动循迹）。
* **通信与交互**: 
  * `bsp_hc05`: HC-05 蓝牙模块透传通信。
  * `hw_lcd` / `hw_spi`: 基于 SPI 的屏幕底层驱动。
  * `key`: 物理按键扫描与防抖。
* **系统框架**:
  * `task`: 轻量级的时间片轮询任务调度器。
  * `process_frame` / `protocol`: 数据帧解析与打包。

### 3. 核心中间件 (`/middle`)
封装底层库（DriverLib），为硬件层提供统一接口。
* `delay`: 毫秒/微秒级精准延时函数。
* `timer`: 定时器中断回调与基础计时服务。
* `usart`: 串口底层收发封装。

### 4. 硬件配置 (SysConfig)
* `empty.syscfg`: 图形化硬件配置源文件（包含 GPIO, UART, SPI, TIMER 等底层引脚与时钟树配置）。
* *注：主函数入口目前位于 `empty.c` (TI 默认模板命名)。*

---

## 🛠️ 开发环境配置

1. **集成开发环境 (IDE)**: [Code Composer Studio (CCS)](https://www.ti.com/tool/CCSTUDIO) 12.x 或更高版本。
2. **编译器**: TI Clang Compiler。
3. **软件开发套件 (SDK)**: 安装最新版 [MSPM0-SDK](https://www.ti.com/tool/MSPM0-SDK)。
4. **图形化配置工具**: CCS 内置的 SysConfig 工具。

### 📥 如何在本地导入本项目
1. 打开 CCS，选择你的本地工作空间（Workspace）。
2. 点击菜单栏 `File` -> `Import...`。
3. 选择 `C/C++` -> `CCS Projects`，点击 Next。
4. 在 `Select search-directory` 中，浏览并选择通过 Git 克隆到本地的本仓库文件夹。
5. 勾选识别到的工程，点击 `Finish` 完成导入。

---

## 💡 团队协作规范 (必读)

* **严禁直接推送 (Push) 主分支**：所有新功能开发必须从 `main` 分支拉取新的 `feature/*` 分支，测试无误后提交 PR (Pull Request)。
* **引脚修改规范**：如需修改硬件引脚配置，**只能且必须**通过双击 `empty.syscfg` 使用图形化界面修改。严禁在代码中直接写死寄存器配置。
* **提交信息格式**：
  * `feat:` 增加新功能（如：`feat: 增加灰度传感器读取逻辑`）
  * `fix:` 修复 Bug（如：`fix: 修复右轮电机PWM输出反向问题`）
  * `docs:` 修改文档
  * `refactor:` 代码重构

---
*Developed by TeamSpark | 备战 2026 NUEDC*
