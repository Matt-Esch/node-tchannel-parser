'use strict';

var Buffer = require('buffer').Buffer;
var parseFrame = require('../index');

var b = new Buffer([
    0x00, 0x74,             // size 116 - Big Endian
    0x04,                   // type 4 (call response)
    0x00,                   // reserved
    0xFF, 0xFF, 0xFF, 0xFE, // id (max non-reserved value)
    0x00, 0x00, 0x00, 0x00, // reserved
    0x00, 0x00, 0x00, 0x00, // reserved
    0x08,                   // flags
    0x09,                   // code

    // Tracing 25 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,

    // Headers: 3
    0x03,

    // header1: "header1string"
    0x07, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x31,
    0x0D, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x31,
    0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,

    // header2: "header2string"
    0x07, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x32,
    0x0D, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x32,
    0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,

    // auth: "moose"
    0x04, 0x61, 0x75, 0x74, 0x68,
    0x05, 0x6D, 0x6F, 0x6F, 0x73, 0x65,

    // Checksum
    0x01, 0xFF, 0xFF, 0xFF, 0xFF,

    // arg1: length 1
    0x00, 0x01, 0xFF,

    // arg2: length 2
    0x00, 0x02, 0xFF, 0xFE,

    // arg3: length 3
    0x00, 0x03, 0xFF, 0xFE, 0xFD
]);

var frame = parseFrame(b);

console.log(frame);
