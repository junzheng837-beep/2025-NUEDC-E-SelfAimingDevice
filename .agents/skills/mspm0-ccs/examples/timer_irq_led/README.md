# TIMG12 Periodic Interrupt LED Example

Minimal reference for a 1 ms periodic timer interrupt on the LCKFB Tianmengxing MSPM0G3507.

This example is based on the verified `26testproject7` smoke test. It is not a complete CCS import project. It shows the `.syscfg` and C patterns that were compiled, flashed with DSLite + J-Link, and confirmed by hitting the TIMG12 ISR through CCS-DSS.

## Configuration

- CPUCLK: 80 MHz
- HFXT: 40 MHz on PA5 / PA6
- Timer: TIMG12
- Timer mode: periodic
- Timer interrupt: ZERO event every 1 ms
- PB22 onboard LED: toggled after 500 interrupts

The generated header contained:

```c
#define CPUCLK_FREQ                    80000000
#define TIMER_0_INST                   (TIMG12)
#define TIMER_0_INST_INT_IRQN          (TIMG12_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE        (79999U)
#define LED_PORT                       (GPIOB)
#define LED_PIN_22_PIN                 (DL_GPIO_PIN_22)
```

At the 32 MHz baseline, the same 1 ms TIMER setting generated `31999U`. Rebuild and inspect generated output after changing CPUCLK.

## Runtime Pattern

- Call `SYSCFG_DL_init()`.
- Enable `TIMER_0_INST_INT_IRQN`.
- Start `TIMER_0_INST`.
- Keep the main loop empty.
- In `TIMER_0_INST_IRQHandler`, handle `DL_TIMER_IIDX_ZERO`.
- Count 500 interrupts, then toggle PB22.

The LED state changes every 500 ms, so one complete on/off cycle takes one second.

## Copying The Clock Tree

Do not blindly copy:

```js
SYSCTL.peripheral.$suggestSolution = "SYSCTL";
```

In the verified timer project that line produced `TypeError: Cannot set properties of undefined`. Remove it if SysConfig reports that `SYSCTL.peripheral` is undefined. HFXT pinmux suggestions belong to:

```js
system.clockTree["HFXT"].peripheral
```
