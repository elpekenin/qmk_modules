// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "qp.h"

/**
 * How many displays of this type there are.
 */
#ifndef LS0XX_NUM_DEVICES
#    define LS0XX_NUM_DEVICES 1
#endif

/**
 * Factory method for an LS0XX MIP SPI device.
 *
 * Args:
 *     panel_width: Width of the display
 *     panel_height: Height of the display
 *     chip_select_pin: GPIO pin used for SPI chip select (CS)
 *     spi_divisor: SPI divisor to use when communicating with the display
 *     spi_mode: SPI mode to use when communicating with the display
 *     buf: Address of the buffer where data will be stored
 */
painter_device_t qp_ls0xx_device_t_make_spi_device(uint16_t panel_width, uint16_t panel_height, pin_t chip_select_pin, uint16_t spi_divisor, int spi_mode, void *buf);
