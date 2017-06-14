# btim
Bluetooth Interface Management library

Install locally
===============

```
> git clone https://github.com/DigitalSecurity/btim
> npm install <path/to/btim>
```


Just compiling
==============

```
> node-gyp clean
> node-gyp configure
> node-gyp build
```

Usage examples
==============

List bluetooth devices
----------------------

```
var hci = require('btim')
var interfaces = JSON.parse('[' + hci.list()  + ']');
console.log(interfaces);
```

Spoof a MAC address
-------------------

```
var hci = require('btim')
// Param 1: interface number; Param 2: new MAC address
hci.spoof_mac(0, '11:22:33:44:55:66');
```

Bring an interface up or down
----------------------------

```
var hci = require('btim')
// Param 1: interface number
hci.interface_up(1);
hci.interface_down(1);
```
