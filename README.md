# LPC SAKEE

LPC84x SAKEE (Swiss Army Knife for Electric Engineers), is a portable debug
tool based on the LPC845 ARM Cortex M0+ MCU from NXP Semiconductors.

## Features

While users are free to extend SAKEE with new modules, the following
functionality is currently provided as part of this codebase:

- 12-bit oscilloscope with 1K sample buffer and HW triggering
- 10-bit waveform generator with user configurable output
- I2C bus scanner
- Voltmeter
- Continuity tester

## SW Requirements

- [MCUXpresso 10.0 or higher](https://www.nxp.com/products/developer-resources/run-time-software/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide-v10.0.2:MCUXpresso-IDE)
- [The LPC845 Example Code Bundle MCUXpresso](https://www.nxp.com/products/microcontrollers-and-processors/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc800-series-cortex-m0-plus-mcus/low-cost-microcontrollers-mcus-based-on-arm-cortex-m0-plus-cores:LPC84X?&tab=Design_Tools_Tab)

## Installation

This repository assumes that it is located in a workspace with the LPC845
Code Bundle, including the following projects:

- `utilities_lib`
- `peripherals_lib`
- `common`

For convenience sake, .zip files of these libraries are included in the
`libs` folder, but more recent versions may be available from NXP.

The project should be located at the same level as the above library
projects.

## Related Links

- [LPC84x Datasheet](https://www.nxp.com/docs/en/data-sheet/LPC84x.pdf)
- [LPC84x User Manual](https://www.nxp.com/docs/en/user-guide/UM11029.pdf)
- [LPC845 Example Code Bundle MCUXpresso](https://www.nxp.com/products/microcontrollers-and-processors/arm-based-processors-and-mcus/lpc-cortex-m-mcus/lpc800-series-cortex-m0-plus-mcus/low-cost-microcontrollers-mcus-based-on-arm-cortex-m0-plus-cores:LPC84X?&tab=Design_Tools_Tab)
