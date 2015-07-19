'use strict';

var tchannel = require('bindings')('tchannel_parser');

var Buffer = require('buffer').Buffer;

var tcBuffer = null;
var tcError = false;
var tcValue = null;

tchannel.setup(
    handleTChannelParserError,
    handleTChannelReqFrame
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
    size, id, flags, ttl, traceflags, headers, service, csumtype,
    csumval, arg1start, arg1end, arg2start, arg2end, arg3start,
    arg3end
) {
    tcValue = new TChannelRequestFrame(
        tcBuffer, size, id, flags, ttl, traceflags, headers,
        service, csumtype, csumval, arg1start, arg1end,
        arg2start, arg2end, arg3start, arg3end
    );
}

function TChannelRequestFrame(
    buffer, size, id, flags, ttl, traceflags, headers, service,
    csumtype, csumval, arg1start, arg1end, arg2start, arg2end,
    arg3start, arg3end
) {
    this.size = size;
    this.type = 0x03;
    this.id = id;
    this.body = new TChannelCallRequestBody(
        buffer,
        flags,
        ttl,
        traceflags,
        headers,
        service,
        csumtype,
        csumval,
        arg1start,
        arg1end,
        arg2start,
        arg2end,
        arg3start,
        arg3end
    );
}

function TChannelCallRequestBody(
    buffer,
    flags,
    ttl,
    traceflags,
    headers,
    service,
    csumtype,
    csumval,
    arg1start,
    arg1end,
    arg2start,
    arg2end,
    arg3start,
    arg3end
) {
    this.type = 0x03;
    this.flags = flags;
    this.tracing = new TChannelTracing(buffer, traceflags);
    this.service = service;
    this.headers = new TChannelHeaders(headers);
    this.csum = new TChannelChecksum(csumtype, csumval);
    this.args = [
        buffer.slice(arg1start, arg1end),
        buffer.slice(arg2start, arg2end),
        buffer.slice(arg3start, arg3end)
    ];
}

function TChannelTracing(buffer, traceflags) {
    this.spanid = buffer.slice(21, 29);
    this.parentid = buffer.slice(29, 37);
    this.traceid = buffer.slice(37, 45);
    this.flags = traceflags;
}

function TChannelHeaders(headers) {
    for (var i = 0; i < headers.length; i += 2) {
        this[headers[i]] = headers[i + 1];
    }
}
function TChannelChecksum(csumtype, csumval) {
    this.type = csumtype;
    this.val = csumval;
}
