/*  KallistiOS ##version##

    lcd.c
    Copyright (C) 2023 Paul Cercueil

*/

/*
    This example demonstrates drawing dynamic contents to the
    VMU's LCD display. It does so by rendering to a virtual
    framebuffer and then presenting it, which sends the updated
    framebuffer to the VMU over the maple protocol which displays
    it.

    This demo also shows off rendering dynamic text using an
    embedded font.
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include <kos/init.h>

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/vmu.h>
#include <dc/vmu_fb.h>

#include <arch/arch.h>

/* 4x6 font from the Linux kernel:
   https://github.com/torvalds/linux/blob/master/lib/fonts/font_mini_4x6.c

   Modified locally to pack the data better.

   Created by Kenneth Albanowski.
   No rights reserved, released to the public domain.
 */
static const char fontdata_4x6[] = {
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0x00, 0x00, 0x00, 0x44, 0x40, 0x40, 0xaa, 0x00,
    0x00, 0xaf, 0xfa, 0x00, 0x46, 0xec, 0x40, 0xa2,
    0x48, 0xa0, 0x69, 0x6a, 0xd0, 0x24, 0x00, 0x00,
    0x24, 0x44, 0x20, 0x42, 0x22, 0x40, 0x0e, 0xee,
    0x00, 0x04, 0xe4, 0x00, 0x00, 0x04, 0x80, 0x00,
    0xe0, 0x00, 0x00, 0x00, 0x40, 0x02, 0x48, 0x00,
    0x4a, 0xaa, 0x40, 0x4c, 0x44, 0xe0, 0xc2, 0x48,
    0xe0, 0xe2, 0x62, 0xe0, 0xaa, 0xe2, 0x20, 0xe8,
    0xe2, 0xe0, 0xe8, 0xea, 0xe0, 0xe2, 0x22, 0x20,
    0xea, 0xea, 0xe0, 0xea, 0xe2, 0x20, 0x00, 0x40,
    0x40, 0x00, 0x40, 0x48, 0x24, 0x84, 0x20, 0x0e,
    0x0e, 0x00, 0x84, 0x24, 0x80, 0xe2, 0x60, 0x40,
    0x4e, 0xe8, 0x40, 0x4a, 0xea, 0xa0, 0xca, 0xca,
    0xc0, 0x68, 0x88, 0x60, 0xca, 0xaa, 0xc0, 0xe8,
    0xe8, 0xe0, 0xe8, 0xe8, 0x80, 0x68, 0xea, 0x60,
    0xaa, 0xea, 0xa0, 0xe4, 0x44, 0xe0, 0x22, 0x2a,
    0x40, 0xaa, 0xca, 0xa0, 0x88, 0x88, 0xe0, 0xae,
    0xea, 0xa0, 0xae, 0xee, 0xa0, 0x4a, 0xaa, 0x40,
    0xca, 0xc8, 0x80, 0x4a, 0xae, 0x60, 0xca, 0xec,
    0xa0, 0x68, 0x42, 0xc0, 0xe4, 0x44, 0x40, 0xaa,
    0xaa, 0x60, 0xaa, 0xa4, 0x40, 0xaa, 0xee, 0xa0,
    0xaa, 0x4a, 0xa0, 0xaa, 0x44, 0x40, 0xe2, 0x48,
    0xe0, 0x64, 0x44, 0x60, 0x08, 0x42, 0x00, 0x62,
    0x22, 0x60, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x0f,
    0x84, 0x00, 0x00, 0x00, 0x6a, 0xe0, 0x88, 0xca,
    0xc0, 0x00, 0x68, 0x60, 0x22, 0x6a, 0x60, 0x0e,
    0xe8, 0x60, 0x24, 0xe4, 0x40, 0x06, 0xa6, 0xe0,
    0x88, 0xca, 0xa0, 0x40, 0x44, 0x40, 0x40, 0x44,
    0x80, 0x08, 0xac, 0xa0, 0x0c, 0x44, 0xe0, 0x00,
    0xee, 0xa0, 0x00, 0xca, 0xa0, 0x04, 0xaa, 0x40,
    0x00, 0xca, 0xc8, 0x00, 0x6a, 0x62, 0x0c, 0xa8,
    0x80, 0x06, 0xc2, 0xc0, 0x04, 0xe4, 0x40, 0x00,
    0xaa, 0x60, 0x00, 0xae, 0x40, 0x00, 0xae, 0xe0,
    0x00, 0xa4, 0xa0, 0x00, 0xae, 0x2c, 0x0e, 0x6c,
    0xe0, 0x24, 0xc4, 0x20, 0x44, 0x44, 0x40, 0x84,
    0x64, 0x80, 0x5a, 0x00, 0x00, 0x4a, 0xae, 0x00,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0x06, 0xc6, 0x00, 0x0c, 0x6c, 0x00,
    0x82, 0x82, 0x82, 0xa5, 0xa5, 0xa5, 0xdb, 0xdb,
    0xdb, 0x44, 0x44, 0x44, 0x44, 0xc4, 0x44, 0x44,
    0xcc, 0x44, 0x66, 0xe6, 0x66, 0x00, 0xe6, 0x66,
    0x00, 0xcc, 0x44, 0x66, 0xee, 0x66, 0x66, 0x66,
    0x66, 0x00, 0xee, 0x66, 0x66, 0xee, 0x00, 0x66,
    0xe0, 0x00, 0x44, 0xcc, 0x00, 0x00, 0xc4, 0x44,
    0x44, 0x70, 0x00, 0x44, 0xf0, 0x00, 0x00, 0xf4,
    0x44, 0x44, 0x74, 0x44, 0x00, 0xf0, 0x00, 0x44,
    0xf4, 0x44, 0x44, 0x77, 0x44, 0x66, 0x76, 0x66,
    0x66, 0x77, 0x00, 0x00, 0x77, 0x66, 0x66, 0xff,
    0x00, 0x00, 0xff, 0x66, 0x66, 0x77, 0x66, 0x00,
    0xff, 0x00, 0x66, 0xff, 0x66, 0x44, 0xff, 0x00,
    0x66, 0xf0, 0x00, 0x00, 0xff, 0x44, 0x00, 0xf6,
    0x66, 0x66, 0x70, 0x00, 0x44, 0x77, 0x00, 0x00,
    0x77, 0x44, 0x00, 0x76, 0x66, 0x66, 0xf6, 0x66,
    0x44, 0xff, 0x44, 0x44, 0xc0, 0x00, 0x00, 0x74,
    0x44, 0xff, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xcc,
    0xcc, 0xcc, 0x33, 0x33, 0x33, 0xff, 0xf0, 0x00,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0,
    0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee,
    0xe0, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xe0, 0xee,
    0xee, 0xe0, 0x00, 0x66, 0x00, 0xee, 0xee, 0xe0,
};

static const vmufb_font_t vmufb_font4x6 = {
    .w = 4,
    .h = 6,
    .stride = 3,
    .fontdata = fontdata_4x6,
};

static const char smiley[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100,
};

static vmufb_t vmufb;
static const char message[] = "        Hello World!        ";

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* Your program's main entry point */
int main(int argc, char **argv) {
    unsigned int x, y, i, vmu;
    maple_device_t *dev;
    float val;

    /* If start is pressed, exit the app. */
    cont_btn_callback(0, CONT_START,
                      (cont_btn_callback_t)arch_exit);

    for(i = 0; ; i++) {
        vmufb_clear(&vmufb);

        val = (float)i * M_PI / 360.0f;
        x = 20 + (int)(20.0f * cosf(val));
        y = 12 + (int)(12.0f * sinf(val));

        vmufb_paint_area(&vmufb, x, y, 8, 8, smiley);

        vmufb_print_string_into(&vmufb, &vmufb_font4x6,
                    12, 12, 24, 6, 0,
                    &message[(i / 16) % sizeof(message)]);


        for(vmu = 0; !!(dev = maple_enum_type(vmu, MAPLE_FUNC_LCD)); vmu++) {
            vmufb_present(&vmufb, dev);
        }
    }

    return 0;
}
