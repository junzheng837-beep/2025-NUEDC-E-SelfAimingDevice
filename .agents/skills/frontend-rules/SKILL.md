---
name: mspm0-car-rules
description: 2026 NUEDC TI MSPM0G3507 智能车开发规范
---

# 核心编程规范
- **驱动使用**：必须使用 TI MSPM0 Driverlib API (ti_msp_dl_config.h)。
- **ISR 限制**：中断服务函数保持极简，禁止阻塞操作。
- **数据类型**：严格使用 stdint.h 定义的类型 (uint32_t 等)。
- **硬件外设**：PID 控制算法在 motor_ctrl.c 中实现；编码器采样在 Encoder.c 中实现。
- **注释风格**：生成函数时，必须包含符合 Doxygen 风格的文档注释，说明输入参数和返回值含义。
- **硬件平台**：本项目使用的是 LCKFB 天枢星 (Tianmengxing) MSPM0G3507 核心板。在分配引脚和编写代码时，必须严格遵守天枢星核心板的引脚防坑指南（如尽量避免分配 PA21, PA23, PA2, PA18, PA10, PA11 等特殊引脚给常规外设）。