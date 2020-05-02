#include "../headers/decoder.h"

namespace remoting
{
    Decoder::Decoder()
    {
        codec = nullptr;
        codecCtx = nullptr;
        frame = nullptr;
        pkt = nullptr;
        try
        {
            /* find the video decoder */
            codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
            if (!codec)
            {
                throw("Codec not found.\n");
            }
            codecCtx = avcodec_alloc_context3(codec);
            if (!codecCtx)
            {
                throw("Could not allocate video codec context.\n");
            }
            /* open codec */
            if (avcodec_open2(codecCtx, codec, NULL) < 0)
            {
                throw("Could not open codec.\n");
            }
            frame = av_frame_alloc();
            if (!frame)
            {
                throw("Could not allocate video frame.\n");
            }
            pkt = av_packet_alloc();
            if (!pkt)
            {
                throw("Could not allocate video packet.\n");
            }
        }
        catch (const char *msg)
        {
            std::cout << "ERROR : Decoder initialization failed. " + std::string(msg);
        }
    }
    Decoder::~Decoder()
    {
        avcodec_free_context(&codecCtx);
        av_frame_free(&frame);
        av_packet_free(&pkt);
    }

    AVFrame *Decoder::decode(uint8_t *data, int size)
    {
        pkt->data = data;
        pkt->size = size;
        if (pkt->size)
        {
            try
            {
                decode();
            }
            catch (const char *msg)
            {
                std::cout<< "ERROR : Decoding failed. " + std::string(msg);
                return nullptr;
            }
        }
        return frame;
    }
    void Decoder::decode()
    {
        int ret;

        ret = avcodec_send_packet(codecCtx, pkt);
        if (ret < 0)
        {
            throw("Error sending a packet for decoding.\n");
        }

        if (ret >= 0)
        {
            ret = avcodec_receive_frame(codecCtx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                throw("No new decoded frame is available.\n");
            else if (ret < 0)
            {
                throw("Error during decoding.\n");
            }
        }
    }
} // namespace remoting