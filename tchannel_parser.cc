#include <nan.h>

using namespace v8;

namespace TChannel {
    enum FrameType {
        // First message on every connection must be init
        TC_INIT_REQ_TYPE=0x01,

        // Remote response to init req
        TC_INIT_RES_TYPE=0x02,

        // RPC method request
        TC_CALL_REQ_TYPE=0x03,

        // RPC method response
        TC_CALL_RES_TYPE=0x04,

        // RPC request continuation fragment
        TC_CALL_REQ_CONTINUE_TYPE=0x13,

        // RPC response continuation fragment
        TC_CALL_RES_CONTINUE_TYPE=0x14,

        // Cancel an outstanding call req / forward req (no body)
        TC_CANCEL_TYPE=0xc0,

        // Claim / cancel a redundant request
        TC_CLAIM_TYPE=0xc1,

        // Protocol level ping req (no body)
        TC_PING_REQ_TYPE=0xd0,

        // Ping res (no body)
        TC_PING_RES_TYPE=0xd1,

        // Protocol level error.
        TC_ERROR_TYPE=0xff
    };

    inline uint16_t readFrameSize(char* buffer) {
        return ((unsigned char)buffer[0] << 8) | ((unsigned char)buffer[1]);
    }

    inline FrameType readFrameType(char* buffer) {
        return (FrameType)buffer[2];
    }

    inline uint32_t readFrameId(char* buffer) {
        // Note: Assuming architecture is little endian
        return (uint32_t)(
            ((unsigned char)buffer[4] << 24) |
            ((unsigned char)buffer[5] << 16) |
            ((unsigned char)buffer[6] << 8) |
            ((unsigned char)buffer[7])
        );
    }
}

static Persistent<String> args_sym;
static Persistent<String> body_sym;
static Persistent<String> csum_sym;
static Persistent<String> flags_sym;
static Persistent<String> headers_sym;
static Persistent<String> id_sym;
static Persistent<String> service_sym;
static Persistent<String> size_sym;
static Persistent<String> tracing_sym;
static Persistent<String> ttl_sym;
static Persistent<String> type_sym;
static Persistent<String> val_sym;
static Persistent<String> argstart_sym;
static Persistent<String> arg1len_sym;
static Persistent<String> arg2len_sym;
static Persistent<String> arg3len_sym;



inline uint8_t readUint8(char* buffer, size_t offset) {
    return (uint8_t)buffer[offset];
}

inline uint16_t readUint16BE(char* buffer, size_t offset) {
    return (uint16_t)(
        ((unsigned char)buffer[offset] << 8) |
        ((unsigned char)buffer[offset + 1])
    );
}

inline uint32_t readUint32BE(char* buffer, size_t offset) {
    return (uint32_t)(
        ((unsigned char)buffer[offset] << 24) |
        ((unsigned char)buffer[offset + 1] << 16) |
        ((unsigned char)buffer[offset + 2] << 8) |
        ((unsigned char)buffer[offset + 3])
    );
}

inline uint32_t readUint8StringToObject(
    Local<Object> obj,
    Persistent<String> key,
    char* buffer,
    size_t length,
    size_t offset
) {
    uint8_t stringLength = readUint8(buffer, offset);
    size_t newOffset = offset + 1 + stringLength;

    if (newOffset <= length) {
        obj->Set(
            key,
            NanNew<String>(&buffer[offset + 1], stringLength)
        );
    }

    return newOffset;
}

inline uint32_t readUint8StringMapToObject(
    Local<Object> obj,
    char* buffer,
    size_t length,
    size_t offset
) {
    uint8_t keyLength = readUint8(buffer, offset);
    size_t valueOffset = offset + 1 + keyLength;

    if (valueOffset >= length) {
        return valueOffset;
    }

    size_t valueLength = readUint8 (buffer, valueOffset);
    size_t newOffset = valueOffset + 1 + valueLength;

    if (newOffset >= length) {
        return newOffset;
    }

    Local<String> key = NanNew<String>(&buffer[offset + 1], keyLength);
    Local<String> value = NanNew<String>(&buffer[valueOffset + 1], valueLength);

    obj->Set(key, value);

    return newOffset;
}

inline uint32_t readUint16BufferLengthToObject(
    Local<Object> obj,
    Persistent<String> key,
    char* buffer,
    size_t length,
    size_t offset
) {
    uint16_t bufferLength = readUint16BE(buffer, offset);
    size_t newOffset = offset + 2 + bufferLength;

    if (newOffset <= length) {
        obj->Set(
            key,
            NanNew<Uint32>((uint32_t)bufferLength)
        );
    }

    return newOffset;
}

// No intermediate struct parsing, direct js object creation
inline void readTChannelHeader(
    Local<Object> frame,
    TChannel::FrameType frameType,
    char* buffer,
    size_t length
) {
    // size:2
    uint32_t frameSize = TChannel::readFrameSize(buffer);
    frame->Set(size_sym, NanNew<Uint32>(frameSize));

    // type:1 (we know it's a call request in this case)
    frameType = TChannel::readFrameType(buffer);
    frame->Set(
        type_sym,
        NanNew<Uint32>((uint32_t)TChannel::TC_CALL_REQ_TYPE)
    );

    // id:4
    uint32_t frameId = TChannel::readFrameId(buffer);
    frame->Set(id_sym, NanNew<Uint32>(frameId));
}

/*
{
    size: Number,
    type: Number,
    id: Number
}

function readCallReqFrom(buffer, offset) {
    var res;
    var body = new CallRequest();

    // flags:1 - Number
    res = bufrw.UInt8.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    body.flags = res.value;

    // ttl:4 - Number
    res = bufrw.UInt32BE.readFrom(buffer, offset);
    if (res.err) return res;

    if (res.value <= 0) {
        return bufrw.ReadResult.error(errors.InvalidTTL({
            ttl: res.value
        }), offset, body);
    }

    offset = res.offset;
    body.ttl = res.value;

    // tracing:24 traceflags:1
    res = Tracing.RW.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    body.tracing = res.value;

    // service~1 String (first byte string length)
    res = bufrw.str1.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    body.service = res.value;

    
    // nh:1 (hk~1 hv~1){nh} Dictionary key-value string-string
    res = header.header1.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    body.headers = res.value;


    // Object: { type: Number, val: Number | 0 }
    // csumtype:1 (csum:4){0,1} (arg~2)*
    res = argsrw.readFrom(body, buffer, offset);
    if (!res.err) res.value = body;

    return res;
}
*/

/*
function readTracingFrom(buffer, offset) {
    var tracing = new Tracing();
    var res;

    // Buffer (64bit number)
    res = fix8.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    tracing.spanid = res.value;

    // ditto
    res = fix8.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    tracing.parentid = res.value;

    // ditto
    res = fix8.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    tracing.traceid = res.value;

    // Number
    res = bufrw.UInt8.readFrom(buffer, offset);
    if (res.err) return res;
    offset = res.offset;
    tracing.flags = res.value;

    return bufrw.ReadResult.just(offset, tracing);
}
*/

/*
 body: {
    type: Number,
    flags: Number,
    ttl: Number,
    tracing: {
        spanid: Buffer,
        parentid: Buffer,
        traceid: Buffer,
        flags: Number,
    },
    service: String,
    headers: Object<String, String>,
    csum: {
        type: Number,
        val: Number | 0
    },
    args: Array<Buffer>
}
*/

/*
flags:1 ttl:4 tracing:25
service~1 nh:1 (hk~1 hv~1){nh}
csumtype:1 (csum:4){0,1} arg1~2 arg2~2 arg3~2
*/
inline void readTChannelCallReq(
    Local<Object> frame,
    char* buffer,
    size_t length
) {
    // Minimum call req size is 55 bytes
    if (length < 55) {
        // TODO: return error
        return;
    }

    Local<Object> body = NanNew<Object>();
    frame->Set(body_sym, body); 

    // Apparently we want this repeated for convenience
    // type:1 (we know it's a call request in this case)
    body->Set(
        type_sym,
        NanNew<Uint32>((uint32_t)TChannel::TC_CALL_REQ_TYPE)
    );

    // flags:1
    uint8_t flags = readUint8(buffer, 16);
    body->Set(flags_sym, NanNew<Uint32>((uint32_t)flags));

    // ttl:4
    uint32_t ttl = readUint32BE(buffer, 17);
    body->Set(ttl_sym, NanNew<Uint32>(ttl));

    // Read tracing info bytes 21 to 45
    Local<Object> tracing = NanNew<Object>();
    body->Set(tracing_sym, tracing);
    uint8_t traceFlags = readUint8(buffer, 45);
    tracing->Set(flags_sym, NanNew<Uint32>((uint32_t)traceFlags));

    size_t offset = 46; // Dynamic positions begin

    // service~1
    offset = readUint8StringToObject(
        body,
        service_sym,
        buffer,
        length,
        offset
    );

    if (offset >= length) {
        // TODO: Error
        return;
    }

    // nh:1 (hk~1 hv~1){nh}
    Local<Object> headers = NanNew<Object>();
    body->Set(headers_sym, headers);

    uint8_t headerCount = readUint8(buffer, offset);
    offset += 1;

    if (offset >= length) {
        // TODO: Error
        return;
    }

    for (uint8_t i = 0; i < headerCount; i++) {
        offset = readUint8StringMapToObject(
            headers,
            buffer,
            length,
            offset
        );

        if (offset >= length) {
            // TODO: Error
            return;
        }
    }

    // csumtype:1 (csum:4){0,1}
    uint8_t csumtype = readUint8(buffer, offset);
    offset += 1;

    uint32_t csumval = 0;

    if (csumtype != 0x00) {
        // check we have 4 bytes of headroom
        if (offset + 4 >= length) {
            // TODO: Error
            return;
        }

        csumval = readUint32BE(buffer, offset);
        offset += 4;
    }

    Local<Object> csum = NanNew<Object>();
    body->Set(csum_sym, csum);
    csum->Set(type_sym, NanNew<Uint32>(((uint32_t)csumtype)));
    csum->Set(val_sym, NanNew<Uint32>(csumval));

    // Check 2 byte overhead for arg1 length
    if (offset + 1 >= length) {
        // TODO: Error
        return;
    }

    body->Set(argstart_sym, NanNew<Uint32>((uint32_t)offset));

    // arg1
    offset = readUint16BufferLengthToObject(
        body,
        arg1len_sym,
        buffer,
        length,
        offset
    );

    if (offset + 1 >= length) {
        // TODO: Error
        return;
    }

    // arg2
    offset = readUint16BufferLengthToObject(
        body,
        arg2len_sym,
        buffer,
        length,
        offset
    );

    if (offset + 1 >= length) {
        // TODO: Error
        return;
    }

    // arg3
    offset = readUint16BufferLengthToObject(
        body,
        arg3len_sym,
        buffer,
        length,
        offset
    );
}

NAN_METHOD(Parse) {
    NanScope();

    Local<Object> frame;
    TChannel::FrameType frameType;
    Local<Object> bufferObject = args[0].As<Object>();

    char* bufferData = node::Buffer::Data(bufferObject);
    size_t bufferLength = node::Buffer::Length(bufferObject);

    if (bufferLength < 16) {
        // TODO: return frame length error
        NanReturnNull();
    }

    frameType = TChannel::readFrameType(bufferData);

    switch (frameType) {
        case TChannel::TC_CALL_REQ_TYPE:
            frame = NanNew<Object>();
            readTChannelHeader(frame, frameType, bufferData, bufferLength);
            readTChannelCallReq(frame, bufferData, bufferLength);
            NanReturnValue(frame);
        default:
            // TODO: Implement the rest of the frames
            NanReturnNull();
    }
}

void Init(Handle<Object> exports) {
    exports->Set(
        NanNew("parse"),
        NanNew<FunctionTemplate>(Parse)->GetFunction()
    );

    NanAssignPersistent(args_sym, NanNew<String>("args"));
    NanAssignPersistent(body_sym, NanNew<String>("body"));
    NanAssignPersistent(csum_sym, NanNew<String>("csum"));
    NanAssignPersistent(flags_sym, NanNew<String>("flags"));
    NanAssignPersistent(headers_sym, NanNew<String>("headers"));
    NanAssignPersistent(id_sym, NanNew<String>("id"));
    NanAssignPersistent(service_sym, NanNew<String>("service"));
    NanAssignPersistent(size_sym, NanNew<String>("size"));
    NanAssignPersistent(tracing_sym, NanNew<String>("tracing"));
    NanAssignPersistent(ttl_sym, NanNew<String>("ttl"));
    NanAssignPersistent(type_sym, NanNew<String>("type"));
    NanAssignPersistent(val_sym, NanNew<String>("val"));
    NanAssignPersistent(argstart_sym, NanNew<String>("argstart"));
    NanAssignPersistent(arg1len_sym, NanNew<String>("arg1len"));
    NanAssignPersistent(arg2len_sym, NanNew<String>("arg2len"));
    NanAssignPersistent(arg3len_sym, NanNew<String>("arg3len"));
}

NODE_MODULE(tchannel_parser, Init);
