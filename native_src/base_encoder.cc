#include <iostream>
#include "../headers/base_encoder.h"
#include <unistd.h>

namespace remoting
{
BaseEncoder::BaseEncoder()
{
    _isInitialised = false;
    _currentCRFValue = -1;
}

void BaseEncoder::Init(bool dummy1, RROutput dummy2)
{
    _width = _screenCapturer->GetWidth();
    _height = _screenCapturer->GetHeight();
    _i_frame_counter = 0;
    _rgbData = _screenCapturer->GetDataPointer();
    _yuvData = new uint8_t[3 * _width * _height / 2]; 

    try 
    {   
        _x264Encoder = OpenEncoder(_width, _height);
    }   
    catch (const char *msg)
    {   
        throw "ERROR: x264 Encoder initialisation failed. " + std::string(msg);
    }   

    _isInitialised = true;
}

// pixel format is bgra32, skipped 4th byte for alpha
void BaseEncoder::Bitmap2Yuv420p_calc2(uint8_t *destination, uint8_t *rgb, size_t width, size_t height)
{
    size_t image_size = width * height;
    size_t upos = image_size;
    size_t vpos = upos + upos / 4;
    size_t i = 0;
    for (size_t line = 0; line < height; ++line)
    {

        if (!(line % 2))
        {
            for (size_t x = 0; x < width; x += 2)
            {
                uint8_t b = rgb[4 * i];
                uint8_t g = rgb[4 * i + 1];
                uint8_t r = rgb[4 * i + 2];

                destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

                destination[upos++] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
                destination[vpos++] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;

                b = rgb[4 * i];
                g = rgb[4 * i + 1];
                r = rgb[4 * i + 2];

                destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
            }
        }
        else
        {
            for (size_t x = 0; x < width; x += 1)
            {
                uint8_t b = rgb[4 * i];
                uint8_t g = rgb[4 * i + 1];
                uint8_t r = rgb[4 * i + 2];

                destination[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
            }
        }
    }
}

/*void BaseEncoder::InitializeConverter(int W, int H)
{
    // Initialise swscale converter
    _swsConverter = sws_getContext(W, H, AV_PIX_FMT_BGRA, W, H, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (_swsConverter == NULL)
    {
        throw "Could not create scaling context.";
    }
    else
    {
        std::cout << "created scaling context succeeded." << std::endl;
    }
}
 */

x264_t *BaseEncoder::OpenEncoder(int width, int height)
{
    x264_param_t x264Params;
    x264_t *h;

    int returnValue = x264_param_default_preset(&x264Params, x264_preset_names[0], x264_tune_names[7]);
    if (returnValue == 0)
    {
        std::cout << x264_preset_names[0] << " preset is applied and " << x264_tune_names[7] << " tune is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply default preset.";
    }

    /* Configure non-default params */
    // x264Params.i_bitdepth = 8;
    x264Params.i_csp = X264_CSP_I420;
    // Width and height should be even for encoder. Setting to next even number
    x264Params.i_width = width + (width % 2);
    x264Params.i_height = height + (height % 2);
    // x264Params.b_vfr_input = 0;
    x264Params.b_repeat_headers = 1;
    x264Params.b_annexb = 1;
    
    x264Params.i_keyint_max = 5000;
    // x264Params.i_keyint_min = INT32_MAX;
    // x264Params.i_avcintra_class
    if (this->_currentCRFValue == -1) {
        this->_currentCRFValue = 34; // Default CRF value
    }
    x264Params.rc.f_rf_constant = this->_currentCRFValue;
    std::cout<<"CRF set as "<<this->_currentCRFValue;

    x264_param_apply_fastfirstpass(&x264Params);

    returnValue = x264_param_apply_profile(&x264Params, x264_profile_names[0]);
    if (returnValue == 0)
    {
        std::cout << x264_profile_names[0] << " profile is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply profile.";
    }

    returnValue = x264_picture_alloc(&_inputPic, x264Params.i_csp, x264Params.i_width, x264Params.i_height);
    if (returnValue == 0)
    {
        std::cout << "x264_picture_alloc succeeded." << std::endl;
    }
    else
    {
        throw "x264_picture_alloc Failed.";
    }

    h = x264_encoder_open(&x264Params);

    return h;
}

void BaseEncoder::CleanUp()
{
    std::cout << "Cleanup invoked";
    delete this->_screenCapturer;
    // delete[] this->_yuvData;  // Not necessary as x264_picture_clean clears yuvData
    x264_picture_clean(&this->_inputPic);
    x264_encoder_close(this->_x264Encoder);
    // sws_freeContext(_swsConverter);
}

void BaseEncoder::SetForceNextFrame()
{
    this->_force_next_frame = true;
}

void BaseEncoder::SendNextFrameAsIFrame()
{
    this->_next_frame_as_iframe = true;
    this->_force_next_frame = true;
}

void BaseEncoder::SetCRFValue(int crfValue)
{
    x264_param_t x264Params;
    x264_encoder_parameters(_x264Encoder, &x264Params);
    x264Params.rc.f_rf_constant = crfValue;
    this->_currentCRFValue = crfValue;
    int returnValue = x264_encoder_reconfig(_x264Encoder, &x264Params);
    if (returnValue == 0)
    {
        std::cout<<"CRF value set as "<<crfValue;
    } 
    else 
    {
        throw "Reconfigure encoder failed";
    }
}

char* BaseEncoder::GetCurrentResolution()
{
    char resolution[12];
    sprintf(resolution, "%d*%d", _width, _height);
    return resolution;
}

BaseEncoder::~BaseEncoder() {}
} // namespace remoting
