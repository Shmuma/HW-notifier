#include <stdio.h>
#include <stdlib.h>
#include <ftdi.h>

char* product_string = "Shmuma FTDI HWNotify board";


int main (int argc, char** argv)
{
    struct ftdi_context ctx;
    int f, write_mode = 0;
    unsigned char* eeprom;

    if (argc == 2) {
        if (strcmp (argv[1], "-w") == 0)
            write_mode = 1;
        else {
            printf ("Unknown option given! Use -w to update EEPROM\n");
            return 1;
        }
    }

    if (ftdi_init (&ctx) < 0) {
        printf ("ftdi_init failed\n");
        return 1;
    }

    f = ftdi_usb_open (&ctx, 0x0403, 0x6001);

    if (f < 0 && f != -5) {
        printf ("unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string (&ctx));
        ftdi_deinit (&ctx);
        exit (-1);
    }

    printf ("Read %d bytes from EEPROM\n", ctx.eeprom_size);
    eeprom = (unsigned char*)malloc (ctx.eeprom_size);
    if (ftdi_read_eeprom (&ctx, eeprom) < 0)
        printf ("EEPROM read failed\n");
    else {
        struct ftdi_eeprom ee_str;

        if (ftdi_eeprom_decode (&ee_str, eeprom, ctx.eeprom_size) < 0)
            printf ("EEPROM decode failed\n");
        else {
            printf ("Vendor %X, product %X\n", ee_str.vendor_id, ee_str.product_id);
            printf ("Manufacturer '%s', product '%s'\n", ee_str.manufacturer, ee_str.product);
            printf ("Pull down on suspend: %d\n", ee_str.suspend_pull_downs);
            printf ("Wake up on RI#: %d\n", ee_str.remote_wakeup);

            if (write_mode) {
                printf ("Perform EEPROM update: pull down on suspend and product string\n");
                ee_str.suspend_pull_downs = 1;
                ee_str.product = product_string;

                ee_str.size = ctx.eeprom_size;
                f = ftdi_eeprom_build (&ee_str, eeprom);
                if (f < 0)
                    printf ("EEPROM build failed: %d\n", f);
                else {
                    printf ("EEPROM built successfully, free bytes: %d\n", f);
                    if (ftdi_write_eeprom (&ctx, eeprom) < 0)
                        printf ("Write failed for unknown reason\n");
                    else
                        printf ("EEPROM updated successfully\n");
                }
            }
        }
    }

    ftdi_usb_close (&ctx);
    ftdi_deinit (&ctx);
    return 0;
}
