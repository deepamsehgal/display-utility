#ifndef REMOTING_HOST_MULTI_SCREEN_CAPTURER_H_
#define REMOTING_HOST_MULTI_SCREEN_CAPTURER_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "display_utility_x11.h"
#include "base_screen_capturer.h"
#include <stdint.h>

namespace remoting
{
class MultiScreenCapturer : public BaseScreenCapturer
{
public:
    MultiScreenCapturer();
    void InitializeMonitorProperties();
    uint8_t *GetDataPointer();
    void CaptureScreen();
    int GetWidth();
    int GetHeight();
    ~MultiScreenCapturer();

private:
    Display *_display;
    Window _window;
    int _width;
    int _height;
    XImage *_xImage;
    std::set<OutputResolutionWithOffset> _currentResolutions;
};
} // namespace remoting

#endif // REMOTING_HOST_MULTI_SCREEN_CAPTURER_H_