'use strict';

var Buffer = require('buffer').Buffer;
var EMPTY_BUFFER = Buffer(0);

module.exports = TChannelRequestFrame;

function TChannelRequestFrame(
    buffer, size, id, flags, ttl, traceflags, service, headers,
    csumtype, csumval, arg1start, arg1end, arg2start, arg2end,
    arg3start, arg3end
) {
    this.size = size;
    this.type = 0x03;
    this.id = id;
    this.body = new TChannelCallRequestBody(
        buffer, flags, ttl, traceflags, service, headers,
        csumtype, csumval, arg1start, arg1end, arg2start,
        arg2end, arg3start, arg3end
    );
}

function TChannelCallRequestBody(
    buffer, flags, ttl, traceflags, service, headers, csumtype,
    csumval, arg1start, arg1end, arg2start, arg2end, arg3start,
    arg3end
) {
    this.type = 0x03;
    this.flags = flags;
    this.ttl = ttl;
    this.tracing = new TChannelRequestTracing(buffer, traceflags);
    this.service = service;
    this.headers = new TChannelRequestHeaders(headers);
    this.csum = new TChannelRequestChecksum(csumtype, csumval);
    this.args = createArgs(
        buffer, arg1start, arg1end, arg2start, arg2end, arg3start, arg3end
    );
}

function TChannelRequestTracing(buffer, traceflags) {
    this.spanid = buffer.slice(21, 29);
    this.parentid = buffer.slice(29, 37);
    this.traceid = buffer.slice(37, 45);
    this.flags = traceflags;
}

function TChannelRequestHeaders(headers) {
    for (var i = 0; i < headers.length; i += 2) {
        this[headers[i]] = headers[i + 1];
    }
}

function TChannelRequestChecksum(csumtype, csumval) {
    this.type = csumtype;
    this.val = csumval;
}

function createArgs(
    buffer, arg1start, arg1end, arg2start, arg2end, arg3start, arg3end
) {
    var args = [];

    if (arg1start !== 0) {
        pushArg(args, buffer, arg1start, arg1end);
    } else {
        return args;
    }

    if (arg2start !== 0) {
        pushArg(args, buffer, arg2start, arg2end);
    } else {
        return args;
    }

    if (arg3start !== 0) {
        pushArg(args, buffer, arg3start, arg3end);
    }

    return args;
}

function pushArg(args, buffer, start, end) {
    if (start === end) {
        args.push(EMPTY_BUFFER);
    } else {
        args.push(buffer.slice(start, end));
    }
}
