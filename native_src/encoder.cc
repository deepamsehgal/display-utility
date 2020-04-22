#include <iostream>
#include "../headers/encoder.h"

namespace remoting
{
Encoder::Encoder()
{
    _isInitialised = false;
}
void Encoder::Init(bool singleMonitorCapture, RROutput rROutput)
{
    if (_isInitialised)
    {
        std::cout << "Deleting stuff for reinitialising..";
        this->CleanUp();
    }
    try
    {
        if (singleMonitorCapture)
        {
            _screenCapturer = new SingleScreenCapturer(rROutput);
        }
        else
        {
            _screenCapturer = new MultiScreenCapturer();
        }
    }
    catch (std::string msg)
    {
        throw "ERROR: x264 Encoder initialisation failed." + msg;
    }

    _width = _screenCapturer->GetWidth();
    _height = _screenCapturer->GetHeight();

    _i_frame_counter = 0;

    // RGB input information
    _rgbData = _screenCapturer->GetDataPointer();
    _rgbPlanes[0] = _rgbData;
    _rgbPlanes[1] = NULL;
    _rgbPlanes[2] = NULL;
    _rgbStride[0] = 4 * _width;
    _rgbStride[1] = 0;
    _rgbStride[2] = 0;

    // YUV output information
    _yuvData = new uint8_t[3 * _width * _height / 2];
    _yuvPlanes[0] = _yuvData;
    _yuvPlanes[1] = _yuvData + _width * _height;
    _yuvPlanes[2] = _yuvData + _width * _height + _width * _height / 4;
    _yuvStride[0] = _width;
    _yuvStride[1] = _width / 2;
    _yuvStride[2] = _width / 2;

    try
    {
        // Initialise x264 encoder
        _x264Encoder = OpenEncoder(_width, _height);
    }
    catch (const char *msg)
    {
        throw "ERROR: x264 Encoder initialisation failed. " + std::string(msg);
    }
    // InitializeConverter(_width, _height);

    _isInitialised = true;
}

void Bitmap2Yuv420p_calc2(uint8_t *destination, uint8_t *rgb, size_t width, size_t height)
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

/*void Encoder::InitializeConverter(int W, int H)
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
uint8_t *Encoder::GetNextFrame(int *frame_size, bool getIFrame)
{
    if (!_isInitialised)
    {
        throw "ERROR: ScreenCaptureUtility not initialised before use.";
    }

    int W = _width;
    int H = _height;

    try
    {
        _screenCapturer->CaptureScreen();
    }
    catch (const char *msg)
    {
        throw "ERROR: Screen capture of next frame failed. " + std::string(msg);
    }

    // int returnValue = sws_scale(_swsConverter, _rgbPlanes, _rgbStride, 0, H, _yuvPlanes, _yuvStride);
    // if (returnValue == H)
    // {
    //     std::cout << "Converted the color space of the image." << std::endl;
    // }
    // else
    // {
    //     throw("Failed to convert the color space of the image.");
    // }

    try
    {
        Bitmap2Yuv420p_calc2(_yuvData, _rgbData, W, H);
    }
    catch (const char *msg)
    {
        throw "ERROR: RGB to YUV conversion failed. " + std::string(msg);
    }

    int luma_size = W * H;
    int chroma_size = luma_size / 4;

    // _inputPic.colorSpace = X265_CSP_I420;
    _inputPic.planes[0] = _yuvData;
    _inputPic.planes[1] = _yuvData + luma_size;
    _inputPic.planes[2] = _yuvData + luma_size + chroma_size;
    _inputPic.stride[0] = W;
    _inputPic.stride[1] = W/2;
    _inputPic.stride[2] = W/2;
    _inputPic.pts = _i_frame_counter;
    if (getIFrame) {
        // Set to force an iFrame
        _inputPic.sliceType = X265_TYPE_IDR;
    } else {
        // Set to get back to normal encodings
        _inputPic.sliceType = X265_TYPE_AUTO;
    }
    _i_frame_counter++;

    int returnValue = 0;
    uint8_t* returnBuffer = nullptr;
    try
    {
        returnValue = x265_encoder_encode(_x264Encoder, &_nal, &_noOfNal, &_inputPic, &_outputPic);
        // std::cout<<i_frame_size;
        int size = 0;
        std::cout<< _noOfNal << std::endl;
        for(int i = 0; i < _noOfNal; i++) {
            size += _nal[i].sizeBytes;
//            std::cout<< _nal[i].sizeBytes << std::endl;
        }
        *frame_size = size;
        
        returnBuffer = new uint8_t[size];
        for(int i = 0, k = 0; i < _noOfNal; i++) {
            memcpy(returnBuffer + k, _nal[i].payload, _nal[i].sizeBytes);
            k += _nal[i].sizeBytes;
            // for(int j = 0; j < _nal[i].sizeBytes; j++, k++) {
            //     returnBuffer[k] = _nal[i].payload[j];
            // }
        }


        if (returnValue <= 0)
        {
            throw "No NAL is produced out of encoder.";
        }
    }
    catch (const char *msg)
    {
        throw "ERROR : Encoding failed. " + std::string(msg);
    }

    return returnBuffer;
}

x265_encoder *Encoder::OpenEncoder(int width, int height)
{
    x265_param x264Params;
    x265_encoder *h;

    x265_param_default(&x264Params);
    int returnValue = x265_param_default_preset(&x264Params, x265_preset_names[2], x265_tune_names[3]);
    if (returnValue == 0)
    {
        std::cout << x265_preset_names[2] << " preset is applied and " << x265_tune_names[3] << " tune is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply default preset.";
    }

    /* Configure non-default params */
    // x264Params.i_bitdepth = 8;
    x264Params.internalCsp = X265_CSP_I420;
    
    // Width and height should be even for encoder. Setting to next even number
    x264Params.sourceWidth = width + (width % 2);
    x264Params.sourceHeight = height + (height % 2);
    // x264Params.b_vfr_input = 0;
    x264Params.bRepeatHeaders = 1;
    x264Params.bAnnexB = 1;

    x264Params.fpsNum = 25;
    x264Params.fpsDenom = 1;

    // x264_param_apply_fastfirstpass(&x264Params);

    returnValue = x265_param_apply_profile(&x264Params, x265_profile_names[0]);
    if (returnValue == 0)
    {
        std::cout << x265_profile_names[0] << " profile is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply profile.";
    }

    x265_picture_init(&x264Params, &_inputPic);
    std::cout << "color space of input picture " << _inputPic.colorSpace << std::endl;
    // _inputPic.colorSpace = X265_CSP_I420;
    // std::cout << "color space of input picture" << _inputPic.colorSpace;
    // if (_inputPic != NULL)
    // {
    //     std::cout << "x264_picture_alloc succeeded." << std::endl;
    // }
    // else
    // {
    //     throw "x264_picture_alloc Failed.";
    // }

    h = x265_encoder_open(&x264Params);
    if(h==NULL){
		printf("x265_encoder_open err\n");
	}

    return h;
}

void Encoder::CleanUp()
{
    std::cout << "Cleanup invoked";
    delete this->_screenCapturer;
    // delete[] this->_yuvData;  // Not necessary as x264_picture_clean clears yuvData
    x265_picture_free(&this->_inputPic);
    x265_encoder_close(this->_x264Encoder);
    // sws_freeContext(_swsConverter);
}

Encoder::~Encoder()
{
    this->CleanUp();
}
} // namespace remoting
