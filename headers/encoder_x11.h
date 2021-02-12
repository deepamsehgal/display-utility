#ifndef REMOTING_HOST_ENCODER_X11_H_
#define REMOTING_HOST_ENCODER_X11_H_

extern "C"
{
#include <X11/Xutil.h>
#include <X11/extensions/Xdamage.h>
}

#include "base_encoder.h"

namespace remoting
{
class EncoderX11: public BaseEncoder
{
public:
    EncoderX11();
    void Init(bool singleMonitorCapture, RROutput rROutput = 0);
    uint8_t *GetNextFrame(int *frameSize);
    ~EncoderX11();

private:
    // Capture properties
    Display* _display;
    Window _window;

    // XDamage
    void InitXDamage();
    bool _use_xdamage;
    Damage _damage_handle;
    int _damage_event_base;
    int _damage_error_base;
    XserverRegion _damage_region;
    XEvent _event;

    uint8_t *CaptureAndEncode(int *frameSize);
    void CleanUp();
};

} // namespace remoting

#endif // REMOTING_HOST_ENCODER_X11_H_
