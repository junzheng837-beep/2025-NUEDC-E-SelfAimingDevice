# TIMG12 1 ms IRQ + PB22 LED SysConfig Snippet

## Use Case

Configure a periodic 1 ms TIMG12 interrupt on the LCKFB Tianmengxing MSPM0G3507. The verified runtime example counts 500 interrupts before toggling the PB22 onboard LED.

## SysConfig Snippet

```js
const GPIO   = scripting.addModule("/ti/driverlib/GPIO", {}, false);
const GPIO1  = GPIO.addInstance();
const TIMER  = scripting.addModule("/ti/driverlib/TIMER", {}, false);
const TIMER1 = TIMER.addInstance();

GPIO1.$name                         = "LED";
GPIO1.port                          = "PORTB";
GPIO1.associatedPins[0].$name       = "PIN_22";
GPIO1.associatedPins[0].assignedPin = "22";

TIMER1.$name              = "TIMER_0";
TIMER1.timerPeriod        = "1 ms";
TIMER1.timerMode          = "PERIODIC";
TIMER1.interrupts         = ["ZERO"];
TIMER1.peripheral.$assign = "TIMG12";

GPIO1.associatedPins[0].pin.$suggestSolution = "PB22";
```

Combine this with `clock_80mhz_mfclk.syscfg.md` when CPUCLK should be 80 MHz.

## Checks

After rebuilding an 80 MHz project, confirm generated output includes:

```c
#define CPUCLK_FREQ                    80000000
#define TIMER_0_INST                   (TIMG12)
#define TIMER_0_INST_INT_IRQN          (TIMG12_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE        (79999U)
```

Do not blindly copy `SYSCTL.peripheral.$suggestSolution = "SYSCTL"` into this HFXT configuration. Remove it if SysConfig reports that `SYSCTL.peripheral` is undefined. HFXT pinmux suggestions belong to the HFXT clock-tree object.
