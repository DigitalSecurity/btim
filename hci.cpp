#include <nan.h>

#include "hci.hpp"


void Init(v8::Local<v8::Object> exports)
{
    exports->Set(Nan::New("list").ToLocalChecked(),
                 Nan::New<v8::FunctionTemplate>(HCI_list)->GetFunction());

    exports->Set(Nan::New("spoof_mac").ToLocalChecked(),
                 Nan::New<v8::FunctionTemplate>(HCI_spoof_mac)->GetFunction());

    exports->Set(Nan::New("interface_up").ToLocalChecked(),
                 Nan::New<v8::FunctionTemplate>(HCI_Up)->GetFunction());

    exports->Set(Nan::New("interface_down").ToLocalChecked(),
                 Nan::New<v8::FunctionTemplate>(HCI_Down)->GetFunction());
}

NODE_MODULE(hcifuctions, Init)
