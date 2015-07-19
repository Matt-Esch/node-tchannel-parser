'use strict';

var tchannel = require('bindings')('tchannel_parser');
var Buffer = require('buffer').Buffer;

var TChannelRequestFrame = require('./tchannel-request-frame.js');
var TChannelResponseFrame = require('./tchannel-response-frame.js');

var tcBuffer = null;
var tcError = false;
var tcValue = null;

tchannel.setup(
    handleTChannelParserError,
    handleTChannelReqFrame,
    handleTChannelResFrame
);

module.exports = parseFrame;

function parseFrame(buffer) {
    if (!Buffer.isBuffer(buffer)) {
        return null;
    }

    tcBuffer = buffer;
    tchannel.parse(buffer);

    var result;

    if (tcError) {
        result = null;
    } else {
        result = tcValue;
    }

    tcBuffer = null;
    tcError = false;
    tcValue = null;
    return result;
}

function handleTChannelParserError() {
    tcError = true;
}

function handleTChannelReqFrame(
    size, id, flags, ttl, traceflags, service, headers, csumtype,
    csumval, arg1start, arg1end, arg2start, arg2end, arg3start,
    arg3end
) {
    tcValue = new TChannelRequestFrame(
        tcBuffer, size, id, flags, ttl, traceflags, service,
        headers, csumtype, csumval, arg1start, arg1end,
        arg2start, arg2end, arg3start, arg3end
    );
}

function handleTChannelResFrame(
    size, id, flags, code, traceflags, headers, csumtype,
    csumval, arg1start, arg1end, arg2start, arg2end, arg3start,
    arg3end
) {
    tcValue = new TChannelResponseFrame(
        tcBuffer, size, id, flags, code, traceflags, headers,
        csumtype, csumval, arg1start, arg1end, arg2start,
        arg2end, arg3start, arg3end
    );
}

