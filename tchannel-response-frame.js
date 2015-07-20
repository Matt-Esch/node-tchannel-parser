'use strict';

var Buffer = require('buffer').Buffer;
var EMPTY_BUFFER = Buffer(0);

module.exports = TChannelResponseFrame;

function TChannelResponseFrame(
    buffer, size, id, flags, code, traceflags, headers, csumtype,
    csumval, arg1start, arg1end, arg2start, arg2end, arg3start,
    arg3end
) {
    this.size = size;
    this.type = 0x04;
    this.id = id;
    this.body = new TChannelCallResponseBody(
        buffer, flags, code, traceflags, headers, csumtype,
        csumval, arg1start, arg1end, arg2start, arg2end,
        arg3start, arg3end
    );
}

function TChannelCallResponseBody(
    buffer, flags, code, traceflags, headers, csumtype, csumval,
    arg1start, arg1end, arg2start, arg2end, arg3start, arg3end
) {
    this.type = 0x04;
    this.flags = flags;
    this.code = code;
    this.tracing = new TChannelResponseTracing(buffer, traceflags);
    this.headers = new TChannelResponseHeaders(headers);
    this.csum = new TChannelResponseChecksum(csumtype, csumval);
    this.args = createArgs(
        buffer, arg1start, arg1end, arg2start, arg2end, arg3start, arg3end
    );
}

function TChannelResponseTracing(buffer, traceflags) {
    this.spanid = buffer.slice(18, 26);
    this.parentid = buffer.slice(26, 34);
    this.traceid = buffer.slice(34, 41);
    this.flags = traceflags;
}

function TChannelResponseHeaders(headers) {
    for (var i = 0; i < headers.length; i += 2) {
        this[headers[i]] = headers[i + 1];
    }
}
function TChannelResponseChecksum(csumtype, csumval) {
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
