// Copyright Pablo Martinez (@elpekenin) <elpekenin@elpekenin.dev>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ls0xx.h"

#include <sys/cdefs.h>

#include "ls0xx_opcodes.h"
#include "qp_comms_spi.h"
#include "qp_surface_internal.h"
#include "spi_master.h"

typedef struct {
    painter_driver_t         base; // must be first, so it can be cast to/from the painter_device_t* type
    qp_comms_spi_config_t    spi_config;
    surface_painter_device_t surface;
} ls0xx_painter_device_t;

// need custom comms vtable for inverted CS logic
bool inverted_cs_init(painter_device_t device) {
    painter_driver_t      *driver       = (painter_driver_t *)device;
    qp_comms_spi_config_t *comms_config = (qp_comms_spi_config_t *)driver->comms_config;

    // Initialize the SPI peripheral
    spi_init();

    // Set up CS as output high
    gpio_set_pin_output(comms_config->chip_select_pin);
    gpio_write_pin_low(comms_config->chip_select_pin);

    return true;
}

bool inverted_cs_start(painter_device_t device) {
    painter_driver_t      *driver       = (painter_driver_t *)device;
    qp_comms_spi_config_t *comms_config = (qp_comms_spi_config_t *)driver->comms_config;

    spi_start_config_t config = (spi_start_config_t){
        .slave_pin     = comms_config->chip_select_pin,
        .lsb_first     = comms_config->lsb_first,
        .mode          = comms_config->mode,
        .divisor       = comms_config->divisor,
        .cs_active_low = false,
    };

    return spi_start_extended(&config);
}

const painter_comms_vtable_t spi_comms_inverted_cs_vtable = {
    .comms_init  = inverted_cs_init,
    .comms_start = inverted_cs_start,
    .comms_send  = qp_comms_spi_send_data,
    .comms_stop  = qp_comms_spi_stop,
};

ls0xx_painter_device_t ls0xx_device_t_drivers[LS0XX_NUM_DEVICES] = {0};

__weak_symbol bool qp_ls0xx_init(painter_device_t device, painter_rotation_t rotation) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    // Change the surface geometry based on the panel rotation
    if (rotation == QP_ROTATION_90 || rotation == QP_ROTATION_270) {
        surface->panel_width  = ls0xx->base.panel_height;
        surface->panel_height = ls0xx->base.panel_width;
    } else {
        surface->panel_width  = ls0xx->base.panel_width;
        surface->panel_height = ls0xx->base.panel_height;
    }

    // Init the internal surface
    if (!surface->driver_vtable->init(surface, QP_ROTATION_0)) {
        qp_dprintf("Failed to init internal surface in qp_ls0xx_init\n");
        return false;
    }

    ls0xx->base.rotation = rotation;

    writePinHigh(ls0xx->spi_config.chip_select_pin);
    const uint8_t ls0xx_init_sequence[] = {LS0XX_CLEAR, 0};
    spi_transmit(ls0xx_init_sequence, ARRAY_SIZE(ls0xx_init_sequence));
    writePinLow(ls0xx->spi_config.chip_select_pin);

    return true;
}

bool qp_ls0xx_passthru_power(painter_device_t device, bool power_on) {
    // No-op
    return true;
}

bool qp_ls0xx_passthru_clear(painter_device_t device) {
    // Just re-init the display
    painter_driver_t *driver = (painter_driver_t *)device;
    return qp_ls0xx_init(device, driver->rotation);
}

bool qp_ls0xx_flush(painter_device_t device) {
    ls0xx_painter_device_t   *ls0xx   = (ls0xx_painter_device_t *)device;
    surface_painter_device_t *surface = &(ls0xx->surface);
    surface_dirty_data_t      dirty   = surface->dirty;

    if (!dirty.is_dirty) {
        // nothing to be sent
        return true;
    }

    // find out dirty area. update is done on **complete** horizontal lines,
    // which means that if device is sideways, we need to check l/r instead
    uint16_t top    = 0;
    uint16_t bottom = 0;
    switch (ls0xx->base.rotation) {
        case QP_ROTATION_0:
            top    = dirty.t;
            bottom = dirty.b;
            break;

        // TODO: Check whether this are backwards
        case QP_ROTATION_90:
            top    = dirty.l;
            bottom = dirty.r;
            break;

        case QP_ROTATION_180:
            top    = dirty.b;
            bottom = dirty.t;
            break;

        case QP_ROTATION_270:
            top    = dirty.r;
            bottom = dirty.l;
            break;
    }

    // bytes sent for each row's data
    uint8_t bytes_per_line = (ls0xx->base.panel_width + 7) / 8 * ls0xx->base.native_bits_per_pixel;
    // offset used to access such data
    uint16_t buffer_offset = top * bytes_per_line;

    // start sending
    writePinHigh(ls0xx->spi_config.chip_select_pin);
    uint8_t cmd = LS0XX_WRITE | LS0XX_VCOM;
    spi_transmit(&cmd, 1);

    // dummy data for alignment, value doesn't matter
    uint8_t dummy = 0;

    // iterate over the lines
    for (uint8_t i = 0; i < bottom + 1 - top; ++i) {
        // set y-pos (counts from 1, needs the `+1`)
        uint8_t n_line = bitrev(i + top + 1);
        spi_transmit(&n_line, 1);

        // send data
        spi_transmit(&surface->u8buffer[buffer_offset], bytes_per_line);
        buffer_offset += bytes_per_line;

        spi_transmit(&dummy, 1);
    }

    spi_transmit(&dummy, 1);
    writePinLow(ls0xx->spi_config.chip_select_pin);

    // clear surface's dirty area, no API to prevent extra prints
    surface->base.driver_vtable->flush(surface);

    return true;
}

bool qp_ls0xx_passthru_viewport(painter_device_t device, uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    return surface->driver_vtable->viewport(surface, left, top, right, bottom);
}

bool qp_ls0xx_passthru_pixdata(painter_device_t device, const void *pixel_data, uint32_t native_pixel_count) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    return surface->driver_vtable->pixdata(surface, pixel_data, native_pixel_count);
}

bool qp_ls0xx_passthru_palette_convert(painter_device_t device, int16_t palette_size, qp_pixel_t *palette) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    return surface->driver_vtable->palette_convert(surface, palette_size, palette);
}

bool qp_ls0xx_passthru_append_pixels(painter_device_t device, uint8_t *target_buffer, qp_pixel_t *palette, uint32_t pixel_offset, uint32_t pixel_count, uint8_t *palette_indices) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    return surface->driver_vtable->append_pixels(surface, target_buffer, palette, pixel_offset, pixel_count, palette_indices);
}

bool qp_ls0xx_passthru_append_pixdata(painter_device_t device, uint8_t *target_buffer, uint32_t pixdata_offset, uint8_t pixdata_byte) {
    ls0xx_painter_device_t *ls0xx   = (ls0xx_painter_device_t *)device;
    painter_driver_t       *surface = (painter_driver_t *)&ls0xx->surface;

    return surface->driver_vtable->append_pixdata(surface, target_buffer, pixdata_offset, pixdata_byte);
}

const painter_driver_vtable_t ls0xx_driver_vtable = {
    .init            = qp_ls0xx_init,
    .power           = qp_ls0xx_passthru_power,
    .clear           = qp_ls0xx_passthru_clear,
    .flush           = qp_ls0xx_flush,
    .pixdata         = qp_ls0xx_passthru_pixdata,
    .viewport        = qp_ls0xx_passthru_viewport,
    .palette_convert = qp_ls0xx_passthru_palette_convert,
    .append_pixels   = qp_ls0xx_passthru_append_pixels,
    .append_pixdata  = qp_ls0xx_passthru_append_pixdata,
};

painter_device_t qp_ls0xx_device_t_make_spi_device(uint16_t panel_width, uint16_t panel_height, pin_t chip_select_pin, uint16_t spi_divisor, int spi_mode, void *buf) {
    for (uint32_t i = 0; i < LS0XX_NUM_DEVICES; ++i) {
        ls0xx_painter_device_t *driver = &ls0xx_device_t_drivers[i];
        if (!driver->base.driver_vtable) {
            painter_device_t surface = qp_make_mono1bpp_surface_advanced(&driver->surface, 1, panel_width, panel_height, buf);
            if (!surface) {
                return NULL;
            }
            driver->base.driver_vtable         = (const painter_driver_vtable_t *)&ls0xx_driver_vtable;
            driver->base.comms_vtable          = (const painter_comms_vtable_t *)&spi_comms_inverted_cs_vtable;
            driver->base.native_bits_per_pixel = 1;
            driver->base.panel_width           = panel_width;
            driver->base.panel_height          = panel_height;
            driver->base.rotation              = QP_ROTATION_0;
            driver->base.offset_x              = 0;
            driver->base.offset_y              = 0;

            // SPI and other pin configuration
            driver->base.comms_config          = &driver->spi_config;
            driver->spi_config.chip_select_pin = chip_select_pin;
            driver->spi_config.lsb_first       = false;
            driver->spi_config.divisor         = spi_divisor;
            driver->spi_config.mode            = spi_mode;

            if (!qp_internal_register_device((painter_device_t)driver)) {
                memset(driver, 0, sizeof(ls0xx_painter_device_t));
                return NULL;
            }

            return (painter_device_t)driver;
        }
    }

    return NULL;
}
