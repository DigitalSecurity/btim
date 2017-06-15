#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <nan.h>

#define INTERFACE_DESCRIPTION_SIZE 1024
#define HEADER_DESCRIPTION_SIZE 512

static struct hci_dev_info device_info;
STAILQ_HEAD(, interface) interfaces;

struct interface
{
    char description[INTERFACE_DESCRIPTION_SIZE];
    STAILQ_ENTRY(interface) next;
};

/*
 * Get device header infos
 * Params:
 *     - devices_info: info about a HCI device.
 */
static char *devices_header(struct hci_dev_info *device_info)
{
    static int header = -1;
    char mac_address[18];
    char header_info[HEADER_DESCRIPTION_SIZE];

    if (header == device_info->dev_id)
        return NULL;

    header = device_info->dev_id;

    ba2str(&device_info->bdaddr, mac_address);

    // Fill the header
    snprintf(header_info, HEADER_DESCRIPTION_SIZE,
        "{\"%s\" : {\"Type\": \"%s\", \"Bus\": \"%s\", \"BD Address\": \"%s\", \"ACL MTU\": \"%d:%d\", \"SCO MTU\": \"%d:%d\"",
        device_info->name,
        hci_typetostr((device_info->type & 0x30) >> 4),
        hci_bustostr(device_info->type & 0x0f),
        mac_address,
        device_info->acl_mtu,
        device_info->acl_pkts,
        device_info->sco_mtu,
        device_info->sco_pkts);

    return strdup(header_info);
}

/*
 * Get device infos
 * Params:
 *     - devices_info: info about a HCI device.
 */
static void devices_info(struct hci_dev_info *device_info)
{
    struct hci_dev_stats *device_stats = &device_info->stat;
    char *device_status, *header;
    char buffer[INTERFACE_DESCRIPTION_SIZE];

    header = devices_header(device_info);

    if (!header)
    {
        printf("Error while filling with device header infos\n");
        return;
    }

    device_status = hci_dflagstostr(device_info->flags);

    // Fill the buffer
    snprintf(buffer, INTERFACE_DESCRIPTION_SIZE,
        "%s, \"status\": \"%s\",\
	\"RX\": {\"bytes\": %d, \"ACL\": %d, \"SCO\": %d, \"events\": %d, \"errors\": %d},\
	\"TX\": {\"bytes\": %d, \"ACL\": %d, \"SCO\": %d, \"events\": %d, \"errors\": %d}}}",
        header, device_status,
        device_stats->byte_rx, device_stats->acl_rx,
        device_stats->sco_rx, device_stats->evt_rx,
        device_stats->err_rx,
        device_stats->byte_tx, device_stats->acl_tx,
        device_stats->sco_tx, device_stats->cmd_tx,
        device_stats->err_tx);

    // Add to the list
    struct interface *bluetooth_interface
        = (struct interface *)malloc(sizeof(struct interface));
    strncpy(bluetooth_interface->description, buffer, 1024);
    STAILQ_INSERT_TAIL(&interfaces, bluetooth_interface, next);

    free(header);
    bt_free(device_status);
}

/*
 * Put HCI interfaces into a linked list.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static void list_devices(void)
{
    struct hci_dev_list_req *devices_list;
    struct hci_dev_req *dr;
    int hci_socket;

    if ((hci_socket = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0)
    {
        perror("Can't open HCI socket.");
        return EXIT_FAILURE;
    }

    if (!(devices_list = (hci_dev_list_req *)malloc(HCI_MAX_DEV * sizeof(struct hci_dev_req))))
    {
        perror("Can't allocate memory");
        return EXIT_FAILURE;
    }

    devices_list->dev_num = HCI_MAX_DEV;
    dr = devices_list->dev_req;

    if (ioctl(hci_socket, HCIGETDEVLIST, (void *)devices_list) < 0)
    {
        perror("Can't get device list");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < devices_list->dev_num; i++)
    {
        device_info.dev_id = (dr+i)->dev_id;

        if (ioctl(hci_socket, HCIGETDEVINFO, (void *)&device_info) < 0)
            continue;

        if (hci_test_bit(HCI_RAW, &device_info.flags) &&
            !bacmp(&device_info.bdaddr, BDADDR_ANY))
        {
            int device = hci_open_dev(device_info.dev_id);
            hci_read_bd_addr(device, &device_info.bdaddr, 1000);
            hci_close_dev(device);
        }

        devices_info(&device_info);
    }

    free(devices_list);
    close(hci_socket);
    return EXIT_SUCCESS;
}


/*
 * List HCI devices.
 * Params:
 *     - info: Contains arguments and a return value.
 */
void HCI_list(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate *isolate = info.GetIsolate();
    v8::Local<v8::Array> array = v8::Array::New(isolate);
    int idx_item = 0;
    struct interface *bluetooth_interface;

    STAILQ_INIT(&interfaces);

    // Get bluetooth interfaces
    list_devices();

    // Populate the array
    STAILQ_FOREACH(bluetooth_interface, &interfaces, next)
    {
        array->Set(idx_item, v8::String::NewFromUtf8(isolate, bluetooth_interface->description));
        idx_item++;
    }

    // Delete the list and free allocated memory
    while (!STAILQ_EMPTY(&interfaces))
    {
        bluetooth_interface = STAILQ_FIRST(&interfaces);
        STAILQ_REMOVE_HEAD(&interfaces, next);
        free(bluetooth_interface);
    }

    info.GetReturnValue().Set(array);
}
