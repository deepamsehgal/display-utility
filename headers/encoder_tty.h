#ifndef REMOTING_HOST_ENCODER_TTY_H_
#define REMOTING_HOST_ENCODER_TTY_H_

#include "base_encoder.h"

namespace remoting
{
class EncoderTty: public BaseEncoder
{
public:
    EncoderTty();
    void Init(bool singleMonitorCapture, RROutput rROutput = 0);
    uint8_t *GetNextFrame(int *frameSize);
    ~EncoderTty();

private:
    uint8_t *CaptureAndEncode(int *frameSize);
};

} // namespace remoting

#endif // REMOTING_HOST_ENCODER_TTY_H_
