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

    return fillBuffers(tchannel.parse(buffer), buffer);
}

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

    var args = body.agrs = [];

    var arg1len = body.arg1len;
    var arg2len = body.arg2len;
    var arg3len = body.arg3len;

    argpos += 2;

    if (arg1len === undefined) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg1len));
    argpos += arg1len + 2;

    if (arg2len === undefined) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg2len));
    argpos += arg2len + 2;

    if (arg3len === undefined) {
        return rawFrame;
    }

    args.push(buffer.slice(argpos, argpos + arg3len));

    delete body.argstart;
    delete body.arg1len;
    delete body.arg2len;
    delete body.arg3len;

    return rawFrame;
}
