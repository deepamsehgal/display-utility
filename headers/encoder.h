#ifndef REMOTING_HOST_ENCODER_H_
#define REMOTING_HOST_ENCODER_H_

extern "C"
{
#include <stdint.h>
#include <X11/Xutil.h>
#include "../x265/headers/x265.h"
#include <X11/extensions/Xdamage.h>
}
#include "base_screen_capturer.h"
#include "single_screen_capturer.h"
#include "multi_screen_capturer.h"
#include <cstring>

namespace remoting
{
class Encoder
{
public:
    Encoder();
    void Init(bool singleMonitorCapture, RROutput rROutput = 0);
    uint8_t *GetNextFrame(int *frameSize);
    void SetForceNextFrame();
    void SendNextFrameAsIFrame();
    ~Encoder();

private:
    // Capture properties
    Display* _display;
    Window _window;
    BaseScreenCapturer *_screenCapturer;
    uint8_t *_rgbData;

    // Encoder properties
    x265_encoder *_x265Encoder;
    // SwsContext *_swsConverter;
    uint8_t *_yuvData;
    bool _isInitialised;
    x265_picture _inputPic;
    x265_picture _outputPic;
    x265_nal *_nal;
    uint32_t _noOfNal;
    int _width;
    int _height;
    int64_t _i_frame_counter;
    bool _force_next_frame;
    bool _next_frame_as_iframe;
    // void InitializeConverter(int width, int height);
    x265_encoder *OpenEncoder(int width, int height);
    uint8_t *CaptureAndEncode(int *frameSize);
    void CleanUp();

    // XDamage
    void InitXDamage();
    bool _use_xdamage;
    Damage _damage_handle;
    int _damage_event_base;
    int _damage_error_base;
    XserverRegion _damage_region;
    XEvent _event;
};

} // namespace remoting

#endif // REMOTING_HOST_ENCODER_H_