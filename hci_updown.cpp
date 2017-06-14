#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <nan.h>

/*
 * A helper function to bring HCI interafaces up or down.
 * Params:
 *     - device_id: device ID.
 *     - status: true (up), false (down).
 * Return values:
 *     - EXIT_FAILURE: on failure.
 *     - EXIT_SUCCESS: on success.
 */
static int hci_interface_up_down(int device_id, bool status)
{
    int hci_socket;

    if ((hci_socket = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0)
    {
        perror("Can't open HCI socket.");
        return EXIT_FAILURE;
    }

    if (status)
        ioctl(hci_socket, HCIDEVUP, device_id);
    else
        ioctl(hci_socket, HCIDEVDOWN, device_id);

    close(hci_socket);

    return EXIT_SUCCESS;
}

/*
 * A wrapper to bring an interface up.
 * Params:
 *     - info: Contains arguments and a return value.
 */
void HCI_Up(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info[0]->IsNumber())
    {
        Nan::ThrowTypeError("1st argument isn't a number");
        return;
    }

    int device_id = info[0]->NumberValue();

    v8::Local<v8::Number> status = Nan::New(hci_interface_up_down(device_id, true));

    info.GetReturnValue().Set(status);
}

/*
 * A wrapper to bring an interface down.
 * Params:
 *     - info: Contains arguments and a return value.
 */
void HCI_Down(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info[0]->IsNumber())
    {
        Nan::ThrowTypeError("1st argument isn't a number");
        return;
    }

    int device_id = info[0]->NumberValue();

    v8::Local<v8::Number> status = Nan::New(hci_interface_up_down(device_id, false));

    info.GetReturnValue().Set(status);
}
