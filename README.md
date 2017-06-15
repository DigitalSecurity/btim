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
var btim = require('btim')
var interfaces = JSON.parse('[' + btim.list()  + ']');
console.log(interfaces);
```

Spoof a MAC address
-------------------

```
var btim = require('btim')
// Param 1: interface number; Param 2: new MAC address
btim.spoof_mac(0, '11:22:33:44:55:66');
```

Bring an interface up or down
----------------------------

```
var btim = require('btim')
// Param 1: interface number
btim.interface_up(1);
btim.interface_down(1);
```
