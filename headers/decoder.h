#ifndef REMOTING_HOST_DECODER_H_
#define REMOTING_HOST_DECODER_H_

extern "C"
{
#include "../ffmpeg/include/libavcodec/avcodec.h"
}
#include <string>
#include <iostream>

namespace remoting
{
    class Decoder
    {
    public:
        Decoder();
        AVFrame *decode(uint8_t *frame, int size);
        ~Decoder();

    private:
        AVCodec *codec;
        AVCodecContext *codecCtx;
        AVFrame *frame;
        AVPacket *pkt;
        void decode();
    };
} // namespace remoting

#endif // REMOTING_HOST_DECODER_H_