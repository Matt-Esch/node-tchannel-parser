'use strict';

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
    this.args = [
        buffer.slice(arg1start, arg1end),
        buffer.slice(arg2start, arg2end),
        buffer.slice(arg3start, arg3end)
    ];
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
