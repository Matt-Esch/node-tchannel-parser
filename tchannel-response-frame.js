'use strict';

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
    this.args = [
        buffer.slice(arg1start, arg1end),
        buffer.slice(arg2start, arg2end),
        buffer.slice(arg3start, arg3end)
    ];
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
