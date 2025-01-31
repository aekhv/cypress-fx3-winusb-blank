#include <stdio.h>
#include <libusb.h>

#define CY_FX_USB_VID           (0x04B4)
#define CY_FX_USB_PID           (0x0101)
#define CY_FX_VENDOR_REQUEST    (0xFF) /* Vendor request type code */
#define DEFAULT_USB_TIMEOUT     (1000) /* 1000 ms */

libusb_context *ctx = nullptr;
libusb_device_handle *handle = nullptr;
unsigned char ep0Buffer[64];

int main()
{

    // Init library
    int rc = libusb_init(&ctx);
    if (rc < 0) {
        printf("FAIL on 'libusb_init'! ( %s )\n", libusb_error_name(rc));
        return -1;
    }

    // Printing lib version
    const struct libusb_version *v;
    v = libusb_get_version();
    printf("LibUSB %d.%d.%d.%d\n", v->major, v->minor, v->micro, v->nano);

    // Getting device list
    ssize_t cnt;
    libusb_device **dev_list;
    cnt = libusb_get_device_list(ctx, &dev_list);
    if (cnt < 0) {
        printf("FAIL on 'libusb_get_device_list'! ( %s )\n", libusb_error_name(cnt));
        return -1;
    }

    // Searching for device
    int err;
    int idx = -1;
    struct libusb_device_descriptor dev_desc;
    for (int i = 0; dev_list[i]; i++) {
        err = libusb_get_device_descriptor(dev_list[i], &dev_desc);
        if (err != LIBUSB_SUCCESS)
        {
            printf("FAIL on 'libusb_get_device_descriptor'! ( %s )\n", libusb_error_name(err));
            continue;
        }
        if ((dev_desc.idVendor == CY_FX_USB_VID) && (dev_desc.idProduct == CY_FX_USB_PID))
        {
            printf("Device found : VID_0x%04X&PID_0x%04X USB %X.%X REV %X.%X\n",
                   dev_desc.idVendor, dev_desc.idProduct,
                   dev_desc.bcdUSB >> 8, dev_desc.bcdUSB & 0xFF,
                   dev_desc.bcdDevice >> 8, dev_desc.bcdDevice & 0xFF);
            idx = i;
            break;
        }
    }

    // Check if device not found
    if (idx == -1) {
        printf("No device found.\n");
        libusb_free_device_list(dev_list, 1);
        libusb_exit(ctx);
        return 0;
    }

    // Opening the device
    err = libusb_open(dev_list[idx], &handle);
    if (err != LIBUSB_SUCCESS)
    {
        printf("FAIL on 'libusb_open'! ( %s )\n", libusb_error_name(err));
        libusb_free_device_list(dev_list, 1);
        libusb_exit(ctx);
        return -1;
    }

    // Device additional information
    unsigned char data[128];
    libusb_get_string_descriptor_ascii(handle, dev_desc.iManufacturer, data, sizeof(data));
    printf("Manufacturer : %s\n", data);
    libusb_get_string_descriptor_ascii(handle, dev_desc.iProduct, data, sizeof(data));
    printf("Product      : %s\n", data);
    libusb_get_string_descriptor_ascii(handle, dev_desc.iSerialNumber, data, sizeof(data));
    printf("Serial number: %s\n", data);

    // Fill buffer by a pattern value
    memset(ep0Buffer, 0xAA, sizeof(ep0Buffer));

    // Control transfer to device
    err = libusb_control_transfer(handle,
                                  LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                  CY_FX_VENDOR_REQUEST, // bRequest
                                  0x00,                 // wValue
                                  0x00,                 // wIndex
                                  ep0Buffer,            // Buffer to send or receive
                                  sizeof(ep0Buffer),    // Always 16 bytes
                                  DEFAULT_USB_TIMEOUT);
    if (err < 0)
    {
        printf("FAIL on 'libusb_control_transfer'! ( %s )\n", libusb_error_name(err));
        libusb_close(handle);
        libusb_free_device_list(dev_list, 1);
        libusb_exit(ctx);
        return -1;
    }
    printf("EP0 buffer first byte sent    : 0x%02X\n", ep0Buffer[0]);

    // Control transfer from device
    err = libusb_control_transfer(handle,
                                  LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                  CY_FX_VENDOR_REQUEST, // bRequest
                                  0x00,                 // wValue
                                  0x00,                 // wIndex
                                  ep0Buffer,            // Buffer to send or receive
                                  sizeof(ep0Buffer),    // Always 16 bytes
                                  DEFAULT_USB_TIMEOUT);
    if (err < 0)
    {
        printf("FAIL on 'libusb_control_transfer'! ( %s )\n", libusb_error_name(err));
        libusb_close(handle);
        libusb_free_device_list(dev_list, 1);
        libusb_exit(ctx);
        return -1;
    }
    printf("EP0 buffer first byte received: 0x%02X\n", ep0Buffer[0]);

    libusb_close(handle);
    libusb_free_device_list(dev_list, 1);
    libusb_exit(ctx);
    return 0;
}
