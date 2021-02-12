#ifndef REMOTING_HOST_TTY_CONSOLE_CAPTURER_H_
#define REMOTING_HOST_TTY_CONSOLE_CAPTURER_H_

#include "base_screen_capturer.h"

namespace remoting
{
class TtyConsoleCapturer : public BaseScreenCapturer
{
public:
    TtyConsoleCapturer();
    uint8_t *GetDataPointer();
    void CaptureScreen();
    int GetWidth();
    int GetHeight();
    ~TtyConsoleCapturer();
    void InitializeMonitorProperties();
private:
    int _width;
    int _height;
    int _bytesPerPixel;
    int _frameBufferDescriptor;
    uint8_t* _data_ptr;
    void resetResources();
};
} // namespace remoting

#endif // REMOTING_HOST_TTY_CONSOLE_CAPTURER_H_
