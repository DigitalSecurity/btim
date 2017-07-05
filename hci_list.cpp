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

static struct hci_dev_info device_info;
STAILQ_HEAD(, interface) interfaces;

struct interface
{
    v8::Local<v8::Object> description;
    STAILQ_ENTRY(interface) next;
};

/*
 * Get device header infos
 * Params:
 *     - devices_info: info about a HCI device.
 */
static void interface_infos(v8::Isolate *isolate, struct hci_dev_info *device_info)
{
    char mac_address[18];
    char buffer[101]; // Let be large
    struct hci_dev_stats *device_stats = &device_info->stat;
    char *device_status;

    ba2str(&device_info->bdaddr, mac_address);

    device_status = hci_dflagstostr(device_info->flags);

    /*
     * Fill an object with all infos
     */

    // RX
    v8::Local<v8::Object> obj_rx = v8::Object::New(isolate);

    obj_rx->Set(v8::String::NewFromUtf8(isolate, "bytes"),
                v8::Number::New(isolate, device_stats->byte_rx));

    obj_rx->Set(v8::String::NewFromUtf8(isolate, "acl"),
                v8::Number::New(isolate, device_stats->acl_rx));

    obj_rx->Set(v8::String::NewFromUtf8(isolate, "sco"),
                v8::Number::New(isolate, device_stats->sco_rx));

    obj_rx->Set(v8::String::NewFromUtf8(isolate, "events"),
                v8::Number::New(isolate, device_stats->evt_rx));

    obj_rx->Set(v8::String::NewFromUtf8(isolate, "errors"),
                v8::Number::New(isolate, device_stats->err_rx));

    // TX
    v8::Local<v8::Object> obj_tx = v8::Object::New(isolate);

    obj_tx->Set(v8::String::NewFromUtf8(isolate, "bytes"),
                v8::Number::New(isolate, device_stats->byte_tx));

    obj_tx->Set(v8::String::NewFromUtf8(isolate, "acl"),
                v8::Number::New(isolate, device_stats->acl_tx));

    obj_tx->Set(v8::String::NewFromUtf8(isolate, "sco"),
                v8::Number::New(isolate, device_stats->sco_tx));

    obj_tx->Set(v8::String::NewFromUtf8(isolate, "events"),
                v8::Number::New(isolate, device_stats->cmd_tx));

    obj_tx->Set(v8::String::NewFromUtf8(isolate, "errors"),
                v8::Number::New(isolate, device_stats->err_tx));

    // Other info
    v8::Local<v8::Object> obj_info = v8::Object::New(isolate);

    obj_info->Set(v8::String::NewFromUtf8(isolate, "type"),
                  v8::String::NewFromUtf8(isolate,
                      hci_typetostr((device_info->type & 0x30) >> 4)));

    obj_info->Set(v8::String::NewFromUtf8(isolate, "bus"),
                  v8::String::NewFromUtf8(isolate,
                      hci_bustostr(device_info->type & 0x0f)));

    obj_info->Set(v8::String::NewFromUtf8(isolate, "address"),
                  v8::String::NewFromUtf8(isolate, mac_address));

    snprintf(buffer, 100, "%d:%d", device_info->acl_mtu, device_info->acl_pkts);
    obj_info->Set(v8::String::NewFromUtf8(isolate, "acl_mtu"),
                  v8::String::NewFromUtf8(isolate, buffer));

    snprintf(buffer, 100, "%d:%d", device_info->sco_mtu, device_info->sco_pkts);
    obj_info->Set(v8::String::NewFromUtf8(isolate, "sco_mtu"),
                  v8::String::NewFromUtf8(isolate, buffer));

    obj_info->Set(v8::String::NewFromUtf8(isolate, "status"),
                  v8::String::NewFromUtf8(isolate, device_status));

    // Construct the object
    obj_info->Set(v8::String::NewFromUtf8(isolate, "rx"), obj_rx);
    obj_info->Set(v8::String::NewFromUtf8(isolate, "tx"), obj_tx);

    v8::Local<v8::Object> obj = v8::Object::New(isolate);
    obj->Set(v8::String::NewFromUtf8(isolate, device_info->name), obj_info);

    // Add to the list
    struct interface *bluetooth_interface
        = (struct interface *)malloc(sizeof(struct interface));
    bluetooth_interface->description = obj;
    STAILQ_INSERT_TAIL(&interfaces, bluetooth_interface, next);

    bt_free(device_status);
}

/*
 * Put HCI interfaces into a linked list.
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static void list_devices(v8::Isolate *isolate)
{
    struct hci_dev_list_req *devices_list;
    struct hci_dev_req *dr;
    int hci_socket;

    if ((hci_socket = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0)
    {
        perror("Can't open HCI socket.");
        return EXIT_FAILURE;
    }

    if ((devices_list = (hci_dev_list_req *)malloc(HCI_MAX_DEV * sizeof(struct hci_dev_req))) == NULL)
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

        interface_infos(isolate, &device_info);
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
    list_devices(isolate);

    // Populate the array
    STAILQ_FOREACH(bluetooth_interface, &interfaces, next)
    {
        array->Set(idx_item, bluetooth_interface->description);
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
