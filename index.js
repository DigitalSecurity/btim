'use strict';

var btim = require('./build/Release/btim');

module.exports.list = function list() {
  return btim.list();
}

module.exports.spoof_mac = function spoof_mac(interface_number, mac_address) {
  return btim.spoof_mac(interface_number, mac_address);
}

module.exports.up = function interface_up(interface_number) {
  return btim.interface_up(interface_number);
}

module.exports.down = function interface_down(interface_number) {
  return btim.interface_down(interface_number);
}
