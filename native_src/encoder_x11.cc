#include "../headers/encoder_x11.h"
#include <iostream>
#include <unistd.h>

namespace remoting
{
EncoderX11::EncoderX11()
{
    _use_xdamage = false;
}

void EncoderX11::Init(bool singleMonitorCapture, RROutput rROutput)
{
    XInitThreads();
    if (_isInitialised)
    {
        std::cout << "Deleting stuff for reinitialising..";
        this->CleanUp();
    } else {
        // Initialise only in the first time
        _display = XOpenDisplay(NULL);
        _window = DefaultRootWindow(_display);
    }
    
    try
    {
        if (singleMonitorCapture)
        {
            _screenCapturer = new SingleScreenCapturer(_display, _window, rROutput);
        }
        else
        {
            _screenCapturer = new MultiScreenCapturer(_display, _window);
        }
    }
    catch (std::string msg)
    {
        throw "ERROR: x264 Encoder initialisation failed." + msg;
    }

    BaseEncoder::Init(singleMonitorCapture, rROutput);
    InitXDamage();
}

uint8_t *EncoderX11::GetNextFrame(int *frame_size)
{
    if (!_isInitialised)
    {
        throw "ERROR: ScreenCaptureUtility not initialised before use.";
    }
    while (1) {
        int pendingEvents = 0;
        if (!_use_xdamage || _force_next_frame) {
            // if (_use_xdamage) {
            //     XDamageSubtract(this->_screenCapturer->GetDisplay(), _damage_handle, None, None);
            // }
            _force_next_frame = false;
            return CaptureAndEncode(frame_size);
        } else {
            pendingEvents = XPending(this->_display);
            bool damage_event_flag = false;
            for(int i = 0; i < pendingEvents; i++) {
                XNextEvent(this->_display, &_event);
                if (_event.type == _damage_event_base + XDamageNotify)
                {
                    damage_event_flag = true;
                }
            }
            if (damage_event_flag) {
                XDamageSubtract(this->_display, _damage_handle, None, _damage_region);
                int rects_num = 0;
                XRectangle bounds;
                XRectangle* rects = XFixesFetchRegionAndBounds(this->_display, _damage_region,
                                                            &rects_num, &bounds);
                XFree(rects);
                return CaptureAndEncode(frame_size);    
            } else {
                usleep(30 * 1000);
            }
        }
    }
}

uint8_t *EncoderX11::CaptureAndEncode(int *frame_size)
{
    try
    {
        _screenCapturer->CaptureScreen();
    }
    catch (const char *msg)
    {
        throw "ERROR: Screen capture of next frame failed. " + std::string(msg);
    }
    
    try
    {
        Bitmap2Yuv420p_calc2(_yuvData, _rgbData, _width, _height);
    }
    catch (const char *msg)
    {
        throw "ERROR: RGB to YUV conversion failed. " + std::string(msg);
    }

    int luma_size = _width * _height;
    int chroma_size = luma_size / 4;

    _inputPic.img.plane[0] = _yuvData;
    _inputPic.img.plane[1] = _yuvData + luma_size;
    _inputPic.img.plane[2] = _yuvData + luma_size + chroma_size;
    _inputPic.i_pts = _i_frame_counter;
    if (_next_frame_as_iframe) {
        // Set to force an iFrame
        std::cout<<"Sending an iFrame from encoder";
        _inputPic.i_type = X264_TYPE_IDR;
        _next_frame_as_iframe = false;
    } else {
        // Set to get back to normal encodings
        _inputPic.i_type = X264_TYPE_AUTO;
    }
    _i_frame_counter++;

    int i_frame_size = 0;
    try
    {
        i_frame_size = x264_encoder_encode(_x264Encoder, &_nal, &_noOfNal, &_inputPic, &_outputPic);
        *frame_size = i_frame_size;

        if (i_frame_size <= 0)
        {
            throw "No NAL is produced out of encoder.";
        }
    }
    catch (const char *msg)
    {
        throw "ERROR : Encoding failed. " + std::string(msg);
    }

    return _nal->p_payload;
}

void EncoderX11::CleanUp()
{
    if (_damage_handle)
        XDamageDestroy(this->_display, _damage_handle);
    BaseEncoder::CleanUp();
}

void EncoderX11::InitXDamage()
{
    // Check for XDamage extension.
    if (!XDamageQueryExtension(this->_display, &_damage_event_base,
                                &_damage_error_base))
    {
        std::cout << "X server does not support XDamage." << std::endl;
        return;
    }
    // TODO(lambroslambrou): Disable DAMAGE in situations where it is known
    // to fail, such as when Desktop Effects are enabled, with graphics
    // drivers (nVidia, ATI) that fail to report DAMAGE notifications
    // properly.
    // Request notifications every time the screen becomes damaged.
    _damage_handle = XDamageCreate(this->_display, this->_window,
                                    XDamageReportNonEmpty);
    if (!_damage_handle)
    {
        std::cout << "Unable to initialize XDamage." << std::endl;
        return;
    }

    // Create an XFixes server-side region to collate damage into.
    _damage_region = XFixesCreateRegion(this->_display, 0, 0);
    if (!_damage_region)
    {
        XDamageDestroy(this->_display, _damage_handle);
        std::cout << "Unable to create XFixes region." << std::endl;
        return;
    }

    _use_xdamage = true;
    std::cout << "Using XDamage extension." << std::endl;
}

EncoderX11::~EncoderX11()
{
    this->CleanUp();
    XCloseDisplay(this->_display);
}
} // namespace remoting
