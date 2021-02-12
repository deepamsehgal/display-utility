#ifndef REMOTING_HOST_ENCODER_H_
#define REMOTING_HOST_ENCODER_H_

extern "C"
{
#include <stdint.h>
#include "../x264/headers/x264.h"
}

#include "base_screen_capturer.h"
#include "single_screen_capturer.h"
#include "multi_screen_capturer.h"
#include "tty_console_capturer.h"

namespace remoting
{
class BaseEncoder
{
public:
    BaseEncoder();
    void Init(bool dummy1, RROutput dummy2 = 0);
    virtual uint8_t *GetNextFrame(int *frameSize) = 0;
    void SetForceNextFrame();
    void SetCRFValue(int crfValue);
    void SendNextFrameAsIFrame();
    virtual ~BaseEncoder();

protected:
    // Capture properties
    BaseScreenCapturer *_screenCapturer;
    uint8_t *_rgbData;

    // Encoder properties
    x264_t *_x264Encoder;

    uint8_t *_yuvData;
    bool _isInitialised;
    x264_picture_t _inputPic;
    x264_picture_t _outputPic;
    x264_nal_t *_nal;
    int _noOfNal;
    int _width;
    int _height;
    int64_t _i_frame_counter;
    bool _force_next_frame;
    bool _next_frame_as_iframe;
    int _currentCRFValue;

    void Bitmap2Yuv420p_calc2(uint8_t *destination, uint8_t *rgb, size_t width, size_t height);
    x264_t *OpenEncoder(int width, int height);
    void CleanUp();
};

} // namespace remoting

#endif // REMOTING_HOST_ENCODER_H_
