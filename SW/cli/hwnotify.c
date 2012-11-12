#include <stdio.h>
#include <stdlib.h>

#include <ftdi.h>


int parse_options (int argc, char** argv, int* red, int* green, int* blue)
{
    char c;

    *red = *green = *blue = 0;

    while ((c = getopt (argc, argv, "rgbRGBh")) != -1) {
        switch (c) {
        case 'r':
            *red = -1;
            break;
        case 'R':
            *red = 1;
            break;
        case 'g':
            *green = -1;
            break;
        case 'G':
            *green = 1;
            break;
        case 'b':
            *blue = -1;
            break;
        case 'B':
            *blue = 1;
            break;
        case 'h':
            printf ("Usage: hwnotify -[rgbRGB]\n");
            return 0;
        }
    }

    return 1;
}


int handle_bit (int bit, int action, unsigned char* buf)
{
    if (action > 0)
        *buf |= 1 << bit;
    if (action < 0)
        *buf &= ~(1 << bit);
}



int main (int argc, char** argv)
{
    struct ftdi_context ctx;
    int f;
    int red, green, blue;
    
    unsigned char buf;

    if (ftdi_init (&ctx) < 0) {
        fprintf (stderr, "ftdi_init failed\n");
        return 1;
    }

    f = ftdi_usb_open (&ctx, 0x0403, 0x6001);

    if (f < 0 && f != -5) {
        fprintf (stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string (&ctx));
        ftdi_deinit (&ctx);
        exit (-1);
    }

    // enable bitbang
    ftdi_set_bitmode (&ctx, 0xFF, BITMODE_BITBANG);

    // parse options
    if (parse_options (argc, argv, &red, &green, &blue)) {
        // read current state first
        ftdi_read_pins (&ctx, &buf);
        // set colors
        handle_bit (0, red, &buf);
        handle_bit (5, green, &buf);
        handle_bit (6, blue, &buf);
        ftdi_write_data (&ctx, &buf, 1);
    }

    ftdi_usb_close (&ctx);
    ftdi_deinit (&ctx);
    return 0;
}
