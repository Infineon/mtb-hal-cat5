# CAT5 (CYW55500/CYW55900) Hardware Abstraction Layer (HAL) Release Notes
The CAT5 Hardware Abstraction Layer (HAL) provides an implementation of the Hardware Abstraction Layer for the CYW55500/CYW55900 chip families. This API provides convenience methods for initializing and manipulating different hardware peripherals. Depending on the specific chip being used, not all features may be supported.

### What Changed?
#### v2.0.0
* Extended CTSS GPIO support for CYW55900
* Added Level triggered wakeup support
#### v1.2.0
* Added CTSS and WLSS GPIO support for CYW55900
#### v1.1.0
* Fixed documentation
* Added KSO bit sleep support for SDIO
* Enabled Low Power Mode in CYW55500 devices
* Enabled Low Power Comparator (LPCOMP) wake-up handling
#### v1.0.0
* Initial release

### Supported Software and Tools
This version of the CAT5 Hardware Abstraction Layer was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox™ Software Environment        | 3.2.0   |
| GCC Compiler                              | 11.3.1  |
| ARM Compiler                              | 6.16    |

Minimum required ModusToolbox™ Software Environment: v3.2.0

### More information
Use the following links for more information, as needed:
* [API Reference Guide](https://infineon.github.io/mtb-hal-cat5/html/modules.html)
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.cypress.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox™](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2023.
