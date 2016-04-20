This library contains micro-layer for Pico]OS.

Currently it contains:

*    File system abstraction layer
*    FAT filesystem from [elm-chan.org][1]
*    ROM filesystem (can be flashed into MCU)
*    MMC/SD card support (SPI mode)
*    microsecond spin-wait function
*    diagnostic functions about Pico]OS resource usage
*    HAL stuff for msp430 5xx/6xx families (compiled only for msp430 port)

There is a [spiffs][4] addon in [picoos-micro-spiffs][5] library.

For more info, see [this blog entry][2] or [manual][3].

[1]: http://elm-chan.org/fsw/ff/00index_e.html
[2]: http://stonepile.fi/micro-layer-for-picoos/
[3]: http://arizuu.github.io/picoos-micro
[4]: http://github.com/pellepl/spiffs
[5]: http://github.com/AriZuu/picoos-micro-spiffs
