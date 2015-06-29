#include <iostream>
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

    /* TODO: maybe write a proper C(++) parser
    struct frame {
        uint16_t size;
        FrameType type;
        uint32_t id;
    };

    struct call_req_frame {
        uint16_t size;
        FrameType type;
        uint32_t id;
        uint8_t flags;
        uint32_t ttl;
        // etc..
    }


    void readCallReqFrame(
        struct call_req_frame* frame,
        char* bufferData,
        size_t length
    ) {
        frame->frameType = FrameType.CALL_REQ;
        memcpy(&frame->size, bufferData, 2);

        // etc ..
    }
    */

    inline uint16_t readFrameSize(char* buffer) {
        // Note: Assuming architecture is little endian
        return ((unsigned char)buffer[1] << 8) | ((unsigned char)buffer[0]);
    }

    inline FrameType readFrameType(char* buffer) {
        return (FrameType)buffer[2];
    }

    inline uint32_t readFrameId(char* buffer) {
        // Note: Assuming architecture is little endian
        return ((unsigned char)buffer[4] << 24) |
            ((unsigned char)buffer[5] << 16) |
            ((unsigned char)buffer[6] << 8) |
            ((unsigned char)buffer[7]);
    }
}

// No intermediate struct parsing, direct js object creation
inline void readTChannelReqFrame(
    Local<Object> frame,
    char* buffer,
    size_t length
) {
    // size:2
    uint32_t frameSize = TChannel::readFrameSize(buffer);
    frame->Set(NanNew<String>("size"), NanNew<Uint32>(frameSize));

    // type:1 (we know it's a call request in this case)
    frame->Set(
        NanNew<String>("type"),
        NanNew<Uint32>((uint32_t)TChannel::TC_CALL_REQ_TYPE)
    );

    // id:4
    uint32_t frameId = TChannel::readFrameId(buffer);
    frame->Set(NanNew<String>("id"), NanNew<Uint32>(frameId));
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
            readTChannelReqFrame(frame, bufferData, bufferLength);
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
}

NODE_MODULE(tchannel_parser, Init);
