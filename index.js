'use strict';

var tchannel = require('bindings')('tchannel_parser');

var Buffer = require('buffer').Buffer;

module.exports = parseFrame;

function parseFrame(buffer) {
    if (!Buffer.isBuffer(buffer)) {
        return null;
    }

    // The frame has to be at least 16 bits
    if (buffer.length < 16) {
        return null;
    }

    return tchannel.parse(buffer);
}

/*
----------------------------------------------
| 0-7  | size:2 | type:1 | reserved:1 | id:4 |
|------|-------------------------------------|
| 8-15 | reserved:8                          |
|------|-------------------------------------|
| 16+  | payload - based on type             |
----------------------------------------------
*/

var b = new Buffer([
    0x10, 0x00,             // size 16 - Big Endian
    0x03,                   // type 3 (call request)
    0x00,                   // reserved
    0xFF, 0xFF, 0xFF, 0xFE, // id (max non-reserved value)
    0x00, 0x00, 0x00, 0x00, // reserved
    0x00, 0x00, 0x00, 0x00  // reserved
]);

var frame = parseFrame(b);

console.log(frame);
