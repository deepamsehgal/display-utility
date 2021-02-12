#ifndef REMOTING_HOST_GETNEXTFRAME_WORKER_H_
#define REMOTING_HOST_GETNEXTFRAME_WORKER_H_

#include <napi.h>
#include "../headers/base_encoder.h"

namespace remoting
{
class GetNextFrameWorker : public Napi::AsyncWorker {
    public:
        GetNextFrameWorker(BaseEncoder* encoder, Napi::Function& callback);
        void Execute();
        void OnOK();
    private:
        BaseEncoder* _encoder;
        uint8_t* _next_frame;
        int _frame_size;
};
}

#endif
