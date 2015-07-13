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
flags:1 ttl:4 tracing:25
service~1 nh:1 (hk~1 hv~1){nh}
csumtype:1 (csum:4){0,1} arg1~2 arg2~2 arg3~2
*/

/*
-----------------------------------------------
|  0-7  | size:2 | type:1 | reserved:1 | id:4 |
|-------|-------------------------------------|
|  8-15 | reserved:8                          |
|-------|-------------------------------------|
| 16-23 | flags:1 | ttl:4 | tracing:3         |
|-------|-------------------------------------|
| 24-31 | tracing:8                           |
|-------|-------------------------------------|
| 32-39 | tracing:8                           |
|-------|-------------------------------------|
| 40-47 | tracing:6 | serviceLen:1 | dynamic..|
-----------------------------------------------

*/

var b = new Buffer([
    0x00, 0x7D,             // size 125 - Big Endian
    0x03,                   // type 3 (call request)
    0x00,                   // reserved
    0xFF, 0xFF, 0xFF, 0xFE, // id (max non-reserved value)
    0x00, 0x00, 0x00, 0x00, // reserved
    0x00, 0x00, 0x00, 0x00, // reserved
    0x08,                   // flags
    0x01, 0x02, 0x03, 0x04,  // TTL Big Endian

    // Tracing 25 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,

    // Service name: 5 bytes - moose
    0x05, 0x6D, 0x6F, 0x6F, 0x73, 0x65,

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

var frame = fillBuffers(parseFrame(b), b);

console.log(frame);

function fillBuffers(rawFrame, buffer) {
    if (!rawFrame ||
        rawFrame.type !== 3 ||
        !rawFrame.body ||
        buffer.length < 46
    ) {
        return rawFrame;
    }

    var body = rawFrame.body;
    var tracing = rawFrame.body.tracing;

    if (tracing) {
        tracing.spanid = buffer.slice(21, 29);
        tracing.parentid = buffer.slice(29, 37);
        tracing.traceid = buffer.slice(37, 45);
    }

    var argpos = body.argstart;

    if (!argpos) {
        return rawFrame;
    }

    var args = body.ags = [];

    var arg1len = body.arg1len;
    var arg2len = body.arg2len;
    var arg3len = body.arg3len;

    argpos += 2;

    if (!arg1len) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg1len));
    argpos += arg1len + 2;

    if (!arg2len) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg2len));
    argpos += arg2len + 2;

    if (!arg3len) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg3len));

    delete body.argstart;
    delete body.arg1len;
    delete body.arg2len;
    delete body.arg3len;

    return rawFrame;
}
