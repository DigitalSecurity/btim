var hci = require('./build/Release/btim');
var interfaces = JSON.parse('[' + hci.list() + ']');
console.log(interfaces);
//hci.spoof_mac(0, "C0:26:DF:00:67:3E");
//hci.interface_down(1);
//hci.interface_up(1);
