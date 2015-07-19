#include <nan.h>

using namespace v8;

namespace tchannel {

struct BufferResult
{
    char* buffer;
    uint32_t length;
};

struct BufferRange
{
    size_t start;
    size_t end;
};

class BufferReader
{
public:
    BufferReader();
    BufferReader(char* buffer, size_t length);

    bool Error();

    uint8_t ReadUint8();
    uint16_t ReadUint16BE();
    uint32_t ReadUint32BE();
    void Skip(size_t bytes);
    void ReadUint8Buffer(struct BufferResult &result);
    void ReadUint16BERange(struct BufferRange &range);

private:
    bool CheckRead(size_t length);
    bool error;
    size_t offset;
    char* buffer;
    size_t length;
};

BufferReader::BufferReader()
{
    this->error = false;
    this->offset = (size_t)0;
    this->buffer = (char*)0;
    this->length = (size_t)0;
}

BufferReader::BufferReader(char* buffer, size_t length)
{
    this->error = false;
    this->offset = 0;
    this->buffer = buffer;
    this->length = length;
}

bool BufferReader::Error()
{
    return this->error;
}

uint8_t BufferReader::ReadUint8()
{
    if (this->error || !this->CheckRead(1))
    {
        this->error = true;
        return 0;
    }

    uint8_t result = (uint8_t)buffer[offset];

    this->offset += 1;

    return result;
}

uint16_t BufferReader::ReadUint16BE()
{
    if (this->error || !this->CheckRead(2))
    {
        this->error = true;
        return 0;
    }

    uint16_t result = (uint16_t)(
        ((unsigned char)buffer[this->offset] << 8) |
        ((unsigned char)buffer[this->offset + 1])
    );

    this->offset += 2;

    return result;
}

uint32_t BufferReader::ReadUint32BE()
{
    if (this->error || !this->CheckRead(4))
    {
        this->error = true;
        return 0;
    }

    uint32_t result = (uint32_t)(
        ((unsigned char)buffer[this->offset] << 24) |
        ((unsigned char)buffer[this->offset + 1] << 16) |
        ((unsigned char)buffer[this->offset + 2] << 8) |
        ((unsigned char)buffer[this->offset + 3])
    );

    this->offset += 4;

    return result;
}

void BufferReader::Skip(size_t count) {
    if (this->error || !this->CheckRead(count))
    {
        this->error = true;
        return;
    }

    this->offset += count;
}

void BufferReader::ReadUint8Buffer(struct BufferResult &result)
{
    if (this->error)
    {
        return;
    }

    size_t bufferLength = (size_t)this->ReadUint8();

    if (this->error || !this->CheckRead(bufferLength))
    {
        this->error = true;
        return;
    }

    result.length = (size_t)bufferLength;

    if (bufferLength > 0) {
        result.buffer = (char*)&this->buffer[this->offset];
        this->offset += bufferLength;
    } else {
        result.buffer = NULL;
    }
}

void BufferReader::ReadUint16BERange(struct BufferRange &range)
{
    if (this->error)
    {
        return;
    }

    size_t bufferLength = (size_t)this->ReadUint16BE();

    if (this->error || !CheckRead(bufferLength))
    {
        this->error = true;
        return;
    }

    range.start = this->offset;
    range.end = this->offset + bufferLength;
    this->offset += bufferLength;
}

bool BufferReader::CheckRead(size_t read)
{
    return this->offset + read <= this->length;
}

enum FrameType
{
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
    return (uint32_t)(
        ((unsigned char)buffer[4] << 24) |
        ((unsigned char)buffer[5] << 16) |
        ((unsigned char)buffer[6] << 8) |
        ((unsigned char)buffer[7])
    );
}

}

static Persistent<Function> js_return_error;
static Persistent<Function> js_return_call_req;
static Persistent<Function> js_return_call_res;

static void HandleError(Local<Object> global) {
    js_return_error->Call(global, 0, NULL);
}

static inline void readTChannelCallReq(
    char* buffer,
    size_t length
) {
    tchannel::BufferReader reader(buffer, length);
    struct tchannel::BufferResult bufferResult;
    struct tchannel::BufferRange bufferRange;

    // Return values
    Local<Number> js_size;
    Local<Number> js_id;
    Local<Number> js_flags;
    Local<Number> js_ttl;
    Local<Number> js_traceflags;
    Local<String> js_service;
    Local<Array>  js_headers;
    Local<Number> js_csumtype;
    Local<Number> js_csumval;
    Local<Number> js_arg1start;
    Local<Number> js_arg1end;
    Local<Number> js_arg2start;
    Local<Number> js_arg2end;
    Local<Number> js_arg3start;
    Local<Number> js_arg3end;

    Local<Object> js_global = NanGetCurrentContext()->Global();

    // size:2
    uint32_t size = (uint32_t)reader.ReadUint16BE();
    if (reader.Error()) return HandleError(js_global);
    js_size = NanNew<Uint32>(size);

    // type:1 reserved:1
    reader.Skip(2);

    // id:4
    uint32_t id = reader.ReadUint32BE();
    if (reader.Error()) return HandleError(js_global);
    js_id = NanNew<Uint32>(id);

    // reserved:8
    reader.Skip(8);

    // flags:1
    uint32_t flags = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_flags = NanNew<Uint32>(flags);

    // ttl:4
    uint32_t ttl = (uint32_t)reader.ReadUint32BE();
    if (reader.Error()) return HandleError(js_global);
    js_ttl = NanNew<Uint32>(ttl);

    // tracing:: spanid:8 parentid:8 traceid:8 
    reader.Skip(24);
    if (reader.Error()) return HandleError(js_global);

    // tracing:: traceflags:1
    uint32_t traceflags = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_traceflags = NanNew<Uint32>(traceflags);

    // service:~1
    reader.ReadUint8Buffer(bufferResult);
    if (reader.Error()) return HandleError(js_global);
    js_service = NanNew<String>(bufferResult.buffer, bufferResult.length);

    // nh:1 (hk~1 hv~1){nh}
    uint8_t nh = reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_headers = NanNew<Array>(2 * nh);

    Local<String> js_hk;
    Local<String> js_hv;

    for (uint8_t h = 0; h < nh; h++) {
        reader.ReadUint8Buffer(bufferResult);
        if (reader.Error()) return HandleError(js_global);
        js_hk = NanNew<String>(bufferResult.buffer, bufferResult.length);
        js_headers->Set(2 * h, js_hk);

        reader.ReadUint8Buffer(bufferResult);
        if (reader.Error()) return HandleError(js_global);
        js_hv = NanNew<String>(bufferResult.buffer, bufferResult.length);
        js_headers->Set(2 * h + 1, js_hv);
    }

    // csumtype:1 (csum:4){0,1}
    uint32_t csumtype = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_csumtype = NanNew<Uint32>(csumtype);


    uint32_t csumval = 0;

    if (csumtype != 0x00) {
        csumval = reader.ReadUint32BE();
        if (reader.Error()) return HandleError(js_global);
    }

    js_csumval = NanNew<Uint32>(csumval);

    // arg1~1
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg1start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg1end = NanNew<Uint32>((uint32_t)bufferRange.end);

    // arg2~2
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg2start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg2end = NanNew<Uint32>((uint32_t)bufferRange.end);

    // arg3~2
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg3start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg3end = NanNew<Uint32>((uint32_t)bufferRange.end);

    Local<Value> js_callreq[15] = {
        js_size,
        js_id,
        js_flags,
        js_ttl,
        js_traceflags,
        js_service,
        js_headers,
        js_csumtype,
        js_csumval,
        js_arg1start,
        js_arg1end,
        js_arg2start,
        js_arg2end,
        js_arg3start,
        js_arg3end
    };

    js_return_call_req->Call(js_global, 15, js_callreq);
}

static inline void readTChannelCallRes(
    char* buffer,
    size_t length
) {
    tchannel::BufferReader reader(buffer, length);
    struct tchannel::BufferResult bufferResult;
    struct tchannel::BufferRange bufferRange;

    // Return values
    Local<Number> js_size;
    Local<Number> js_id;
    Local<Number> js_flags;
    Local<Number> js_code;
    Local<Number> js_traceflags;
    Local<Array>  js_headers;
    Local<Number> js_csumtype;
    Local<Number> js_csumval;
    Local<Number> js_arg1start;
    Local<Number> js_arg1end;
    Local<Number> js_arg2start;
    Local<Number> js_arg2end;
    Local<Number> js_arg3start;
    Local<Number> js_arg3end;

    Local<Object> js_global = NanGetCurrentContext()->Global();

    // size:2
    uint32_t size = (uint32_t)reader.ReadUint16BE();
    if (reader.Error()) return HandleError(js_global);
    js_size = NanNew<Uint32>(size);

    // type:1 reserved:1
    reader.Skip(2);

    // id:4
    uint32_t id = reader.ReadUint32BE();
    if (reader.Error()) return HandleError(js_global);
    js_id = NanNew<Uint32>(id);

    // reserved:8
    reader.Skip(8);

    // flags:1
    uint32_t flags = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_flags = NanNew<Uint32>(flags);

    // code:1
    uint32_t code = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_code = NanNew<Uint32>(code);

    // tracing:: spanid:8 parentid:8 traceid:8 
    reader.Skip(24);
    if (reader.Error()) return HandleError(js_global);

    // tracing:: traceflags:1
    uint32_t traceflags = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_traceflags = NanNew<Uint32>(traceflags);

    // nh:1 (hk~1 hv~1){nh}
    uint8_t nh = reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_headers = NanNew<Array>(2 * nh);

    Local<String> js_hk;
    Local<String> js_hv;

    for (uint8_t h = 0; h < nh; h++) {
        reader.ReadUint8Buffer(bufferResult);
        if (reader.Error()) return HandleError(js_global);
        js_hk = NanNew<String>(bufferResult.buffer, bufferResult.length);
        js_headers->Set(2 * h, js_hk);

        reader.ReadUint8Buffer(bufferResult);
        if (reader.Error()) return HandleError(js_global);
        js_hv = NanNew<String>(bufferResult.buffer, bufferResult.length);
        js_headers->Set(2 * h + 1, js_hv);
    }

    // csumtype:1 (csum:4){0,1}
    uint32_t csumtype = (uint32_t)reader.ReadUint8();
    if (reader.Error()) return HandleError(js_global);
    js_csumtype = NanNew<Uint32>(csumtype);


    uint32_t csumval = 0;

    if (csumtype != 0x00) {
        csumval = reader.ReadUint32BE();
        if (reader.Error()) return HandleError(js_global);
    }

    js_csumval = NanNew<Uint32>(csumval);

    // arg1~1
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg1start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg1end = NanNew<Uint32>((uint32_t)bufferRange.end);

    // arg2~2
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg2start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg2end = NanNew<Uint32>((uint32_t)bufferRange.end);

    // arg3~2
    reader.ReadUint16BERange(bufferRange);
    if (reader.Error()) return HandleError(js_global);
    js_arg3start = NanNew<Uint32>((uint32_t)bufferRange.start);
    js_arg3end = NanNew<Uint32>((uint32_t)bufferRange.end);

    Local<Value> js_callres[14] = {
        js_size,
        js_id,
        js_flags,
        js_code,
        js_traceflags,
        js_headers,
        js_csumtype,
        js_csumval,
        js_arg1start,
        js_arg1end,
        js_arg2start,
        js_arg2end,
        js_arg3start,
        js_arg3end
    };

    js_return_call_res->Call(js_global, 14, js_callres);
}


NAN_METHOD(Parse) {
    NanScope();

    Local<Object> frame;
    tchannel::FrameType frameType;
    Local<Object> bufferObject = args[0].As<Object>();

    char* bufferData = node::Buffer::Data(bufferObject);
    size_t bufferLength = node::Buffer::Length(bufferObject);

    tchannel::BufferReader reader(bufferData, bufferLength);


    if (bufferLength < 16) {
        // TODO: return frame length error
        NanReturnNull();
    }

    frameType = tchannel::readFrameType(bufferData);

    switch (frameType) {
        case tchannel::TC_CALL_REQ_TYPE:
            readTChannelCallReq(bufferData, bufferLength);
            NanReturnNull();
        case tchannel::TC_CALL_RES_TYPE:
            readTChannelCallRes(bufferData, bufferLength);
            NanReturnNull();
        default:
            // TODO: Implement the rest of the frames
            NanReturnNull();
    }
}

NAN_METHOD(Setup) {
    NanScope();

    Local<Function> returnError = args[0].As<Function>();
    Local<Function> returnCallReq = args[1].As<Function>();
    Local<Function> returnCallRes = args[2].As<Function>();

    assert(returnError->IsFunction());
    assert(returnCallReq->IsFunction());
    assert(returnCallRes->IsFunction());
    // TODO: Add other frame types here

    NanAssignPersistent(js_return_error, returnError);
    NanAssignPersistent(js_return_call_req, returnCallReq);
    NanAssignPersistent(js_return_call_res, returnCallRes);
    // TODO: Add other frame types here

    NanReturnNull();
}

void Init(Handle<Object> exports) {
    exports->Set(
        NanNew("parse"),
        NanNew<FunctionTemplate>(Parse)->GetFunction()
    );

    exports->Set(
        NanNew("setup"),
        NanNew<FunctionTemplate>(Setup)->GetFunction()
    );
}

NODE_MODULE(tchannel_parser, Init);
