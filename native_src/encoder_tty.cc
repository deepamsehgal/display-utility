#include "../headers/encoder_tty.h"
#include <iostream>
#include <unistd.h>

namespace remoting
{
EncoderTty::EncoderTty() {}

void EncoderTty::Init(bool singleMonitorCapture, RROutput rROutput)
{
    try
    {
        _screenCapturer = new TtyConsoleCapturer();
    }
    catch (std::string msg)
    {
        throw "ERROR: x264 Encoder initialisation failed." + msg;
    }

    BaseEncoder::Init(singleMonitorCapture, rROutput);
}

// pixel format is bgra32, skipped 4th byte for alpha

uint8_t *EncoderTty::GetNextFrame(int *frame_size)
{
    if (!_isInitialised)
    {
        throw "ERROR: ScreenCaptureUtility not initialised before use.";
    }
    return CaptureAndEncode(frame_size);
}

uint8_t *EncoderTty::CaptureAndEncode(int *frame_size)
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
    	_rgbData = _screenCapturer->GetDataPointer();
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

EncoderTty::~EncoderTty()
{
    this->CleanUp();
}
} // namespace remoting
