#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <nan.h>

#define OCF_ERICSSON_WRITE_BD_ADDR     0x000d
#define ERICSSON_WRITE_BD_ADDR_CP_SIZE 6

#define OCF_TI_WRITE_BD_ADDR           0x0006
#define TI_WRITE_BD_ADDR_CP_SIZE       6

#define OCF_BCM_WRITE_BD_ADDR          0x0001
#define BCM_WRITE_BD_ADDR_CP_SIZE      6

#define OCF_ZEEVO_WRITE_BD_ADDR        0x0001
#define ZEEVO_WRITE_BD_ADDR_CP_SIZE    6

typedef struct {
    bdaddr_t    bdaddr;
} __attribute__ ((packed)) write_bd_addr_cp;

/*
 * Reset a device after a MAC address change on a generic device.
 * Params:
 *     - descriptor: device's file descriptor.
 */
static int generic_reset_device(int descriptor)
{
    bdaddr_t bdaddr;
    int status;

    status = hci_send_cmd(descriptor, 0x03, 0x0003, 0, NULL);
    if (status < 0)
        return status;

    return hci_read_bd_addr(descriptor, &bdaddr, 10000);
}

/*
 * A callback function to change a MAC address on generic devices.
 * Params:
 *     - descriptor: device's file descriptor.
 *     - bdaddr: new mac address.
 *     - opcode_command_field: Command opcode.
 *     - command_length: Command size.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static int generic_write_bd_addr(int descriptor, bdaddr_t *bdaddr, int opcode_command_field, int command_length)
{
    struct hci_request request;
    write_bd_addr_cp command_params;

    memset(&command_params, 0, sizeof(command_params));
    bacpy(&command_params.bdaddr, bdaddr);

    memset(&request, 0, sizeof(request));
    request.ogf    = OGF_VENDOR_CMD;
    request.ocf    = opcode_command_field;
    request.cparam = &command_params;
    request.clen   = command_length;
    request.rparam = NULL;
    request.rlen   = 0;

    if (hci_send_req(descriptor, &request, 1000) < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


#define OCF_ERICSSON_STORE	0x0022
typedef struct {
    uint8_t    user_id;
    uint8_t    flash_length;
    uint8_t    flash_data[253];
} __attribute__ ((packed)) ericsson_store_in_flash_cp;
#define ERICSSON_STORE_CP_SIZE 255

/*
 * A callback function to change a MAC address on Ericsson devices.
 * Params:
 *     - descriptor: device's descriptor.
 *     - flash_data: MAC address.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static int ericsson_write(int descriptor, uint8_t *flash_data)
{
    struct hci_request request;
    ericsson_store_in_flash_cp command_params;
    uint8_t flash_length = 6;

    memset(&command_params, 0, sizeof(command_params));
    command_params.user_id      = 0xfe;	// user id
    command_params.flash_length = flash_length;
    if (flash_length > 0)
        memcpy(command_params.flash_data, flash_data, flash_length);

    memset(&request, 0, sizeof(request));
    request.ogf    = OGF_VENDOR_CMD;
    request.ocf    = OCF_ERICSSON_STORE;
    request.cparam = &command_params;
    request.clen   = ERICSSON_STORE_CP_SIZE;
    request.rparam = NULL;
    request.rlen   = 0;

    if (hci_send_req(descriptor, &request, 1000) < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/*
 * A callback function to change a MAC address on CSR devices.
 * Params:
 *     - descriptor: device's descriptor.
 *     - bdaddr: new MAC address.
 *     - opcode_command_field: not used (0).
 *     - command_length: not used (0).
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static int csr_write_bd_addr(int descriptor, bdaddr_t *bdaddr, int opcode_command_field, int command_length)
{
    unsigned char cmd[] =
    {
        0x02, 0x00, 0x0c, 0x00, 0x11, 0x47, 0x03, 0x70,
        0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char command_params[254], returned_parameters[254];
    struct hci_request request;

    cmd[16] = bdaddr->b[2];
    cmd[17] = 0x00;
    cmd[18] = bdaddr->b[0];
    cmd[19] = bdaddr->b[1];
    cmd[20] = bdaddr->b[3];
    cmd[21] = 0x00;
    cmd[22] = bdaddr->b[4];
    cmd[23] = bdaddr->b[5];

    memset(&command_params, 0, sizeof(command_params));
    command_params[0] = 0xc2;
    memcpy(command_params + 1, cmd, sizeof(cmd));

    memset(&request, 0, sizeof(request));
    request.ogf    = OGF_VENDOR_CMD;
    request.ocf    = 0x00;
    request.event  = EVT_VENDOR;
    request.cparam = command_params;
    request.clen   = sizeof(cmd) + 1;
    request.rparam = returned_parameters;
    request.rlen   = sizeof(returned_parameters);

    if (hci_send_req(descriptor, &request, 2000) < 0)
        return EXIT_FAILURE;

    if (returned_parameters[0] != 0xc2)
    {
        errno = EIO;
        return EXIT_FAILURE;
    }

    if ((returned_parameters[9] + (returned_parameters[10] << 8)) != 0)
    {
        errno = ENXIO;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
 * Reset a CSR device after a MAC address change.
 * Params:
 *     - descriptor: CSR device's file descriptor.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static int csr_reset_device(int descriptor)
{
    unsigned char cmd[] =
    {
        0x02, 0x00, 0x09, 0x00,
        0x00, 0x00, 0x01, 0x40, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    unsigned char command_params[254], returned_parameters[254];
    struct hci_request request;

    memset(&command_params, 0, sizeof(command_params));
    command_params[0] = 0xc2;
    memcpy(command_params + 1, cmd, sizeof(cmd));

    memset(&request, 0, sizeof(request));
    request.ogf    = OGF_VENDOR_CMD;
    request.ocf    = 0x00;
    request.event  = EVT_VENDOR;
    request.cparam = command_params;
    request.clen   = sizeof(cmd) + 1;
    request.rparam = returned_parameters;
    request.rlen   = sizeof(returned_parameters);

    if (hci_send_req(descriptor, &request, 2000) < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/*
 * A callback function to change a MAC address on ST microelecronics devices.
 * Params:
 *     - descriptor: device's descriptor.
 *     - bdaddr: new MAC address.
 *     - opcode_command_field: not used (0).
 *     - command_length: not used (0).
 */
static int st_write_bd_addr(int descriptor, bdaddr_t *bdaddr, int opcode_command_field, int command_length)
{
    return ericsson_write(descriptor, (uint8_t *)bdaddr);
}

static struct {
    uint16_t compid;
    int (*write_bd_addr)(int descriptor, bdaddr_t *bdaddr, int opcode_command_field, int command_length);
    int (*reset_device)(int descriptor);
    int opcode_command_field;
    int command_length;
} vendor[] = {
    {0,  generic_write_bd_addr, NULL,                 OCF_ERICSSON_WRITE_BD_ADDR, ERICSSON_WRITE_BD_ADDR_CP_SIZE},
    {10, csr_write_bd_addr,     csr_reset_device,     0,                          0                             },
    {13, generic_write_bd_addr, NULL,                 OCF_TI_WRITE_BD_ADDR,       TI_WRITE_BD_ADDR_CP_SIZE      },
    {15, generic_write_bd_addr, generic_reset_device, OCF_BCM_WRITE_BD_ADDR,      BCM_WRITE_BD_ADDR_CP_SIZE     },
    {18, generic_write_bd_addr, NULL,                 OCF_ZEEVO_WRITE_BD_ADDR,    ZEEVO_WRITE_BD_ADDR_CP_SIZE   },
    {48, st_write_bd_addr,      generic_reset_device, 0,                          0                             },
    {57, generic_write_bd_addr, generic_reset_device, OCF_ERICSSON_WRITE_BD_ADDR, ERICSSON_WRITE_BD_ADDR_CP_SIZE},
    {65535,    NULL,            NULL,                 0,                          0                             },
};

/*
 * Spoof a MAC address.
 * Params:
 *     - new_mac_address: MAC address which will be assigned to the device.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 *     - EIO: New MAC address wasn't written.
 *     - ECANCELED: The device should be reset manually.
 */
int hci_spoof_mac(int device_id, char const *new_mac_address)
{
    struct hci_dev_info device_info;
    struct hci_version version;
    bdaddr_t bdaddr;
    char mac_address[18];
    int i, descriptor;
    int status = ECANCELED; // Worst case by default: manual reset

    descriptor = hci_open_dev(device_id);
    if (descriptor < 0)
    {
        fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
            device_id, strerror(errno), errno);
        return EXIT_FAILURE;
    }

    if (hci_devinfo(device_id, &device_info) < 0)
    {
        fprintf(stderr, "Can't get device info for hci%d: %s (%d)\n",
            device_id, strerror(errno), errno);
        hci_close_dev(descriptor);
        return EXIT_FAILURE;
    }

    if (hci_read_local_version(descriptor, &version, 1000) < 0)
    {
        fprintf(stderr, "Can't read version info for hci%d: %s (%d)\n",
            device_id, strerror(errno), errno);
        hci_close_dev(descriptor);
        return EXIT_FAILURE;
    }

    if (!bacmp(&device_info.bdaddr, BDADDR_ANY))
    {
        if (hci_read_bd_addr(descriptor, &bdaddr, 1000) < 0)
        {
            fprintf(stderr, "Can't read address for hci%d: %s (%d)\n",
                device_id, strerror(errno), errno);
            hci_close_dev(descriptor);
            return EXIT_FAILURE;
        }
    }
    else
    {
        bacpy(&bdaddr, &device_info.bdaddr);
    }

    ba2str(&bdaddr, mac_address);

    str2ba(new_mac_address, &bdaddr);

    if (!bacmp(&bdaddr, BDADDR_ANY))
    {
        hci_close_dev(descriptor);
        return EXIT_FAILURE;
    }

    for (i = 0; vendor[i].compid != 65535; i++)
    {
        if (version.manufacturer == vendor[i].compid)
        {
            ba2str(&bdaddr, mac_address);

            if (vendor[i].write_bd_addr(descriptor, &bdaddr,
                vendor[i].opcode_command_field,
                vendor[i].command_length) < 0)
            {
                fprintf(stderr, "Can't write new MAC address\n");
                hci_close_dev(descriptor);
                return EIO;
            }

            if (vendor[i].reset_device)
            {
                if (vendor[i].reset_device(descriptor) < 0)
                {
                    // The device should be reset manually.
                    status = ECANCELED;
                }
                else
                {
                    // Reset the device
                    ioctl(descriptor, HCIDEVRESET, device_id);
                    status = EXIT_SUCCESS;
                }
            }
            else
            {
               // The device should be reset manually.
               status = ECANCELED;
            }

            hci_close_dev(descriptor);
            return status;
        }
    }

    hci_close_dev(descriptor);

    return EXIT_FAILURE;
}

/*
 * A wrapper to spoof a MAC address.
 * Params:
 *     - info: Contains arguments and a return value.
 */
void HCI_spoof_mac(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    if(!info[0]->IsNumber() || !info[1]->IsString())
    {
        Nan::ThrowTypeError("1st argument should be a number and the 2nd one a string");
        return;
    }

    int device_id = info[0]->NumberValue();
    v8::String::Utf8Value new_mac_address(info[1]);

    v8::Local<v8::Number> status = Nan::New(hci_spoof_mac(device_id, *new_mac_address));

    info.GetReturnValue().Set(status);
}
