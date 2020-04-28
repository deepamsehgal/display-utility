#include <iostream>
#include "../headers/encoder.h"
#include <unistd.h>

namespace remoting
{
Encoder::Encoder()
{
    _isInitialised = false;
    _use_xdamage = false;
}
void Encoder::Init(bool singleMonitorCapture, RROutput rROutput)
{
    XInitThreads();
    if (_isInitialised)
    {
        std::cout << "Deleting stuff for reinitialising..";
        this->CleanUp();
    }
    else
    {
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

    _width = _screenCapturer->GetWidth();
    _height = _screenCapturer->GetHeight();

    _i_frame_counter = 0;

    // RGB input information
    _rgbData = _screenCapturer->GetDataPointer();
    // _rgbPlanes[0] = _rgbData;
    // _rgbPlanes[1] = NULL;
    // _rgbPlanes[2] = NULL;
    // _rgbStride[0] = 4 * _width;
    // _rgbStride[1] = 0;
    // _rgbStride[2] = 0;

    // YUV output information
    _yuvData = new uint8_t[3 * _width * _height / 2];
    // _yuvPlanes[0] = _yuvData;
    // _yuvPlanes[1] = _yuvData + _width * _height;
    // _yuvPlanes[2] = _yuvData + _width * _height + _width * _height / 4;
    // _yuvStride[0] = _width;
    // _yuvStride[1] = _width / 2;
    // _yuvStride[2] = _width / 2;

    try
    {
        // Initialise x265 encoder
        _x265Encoder = OpenEncoder(_width, _height);
    }
    catch (const char *msg)
    {
        throw "ERROR: x264 Encoder initialisation failed. " + std::string(msg);
    }
    // InitializeConverter(_width, _height);

    InitXDamage();

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
uint8_t *Encoder::GetNextFrame(int *frame_size)
{
    if (!_isInitialised)
    {
        throw "ERROR: ScreenCaptureUtility not initialised before use.";
    }
    while (1)
    {
        int pendingEvents = 0;
        if (!_use_xdamage || _force_next_frame)
        {
            // if (_use_xdamage) {
            //     XDamageSubtract(this->_screenCapturer->GetDisplay(), _damage_handle, None, None);
            // }
            _force_next_frame = false;
            return CaptureAndEncode(frame_size);
        }
        else
        {
            pendingEvents = XPending(this->_display);
            bool damage_event_flag = false;
            for (int i = 0; i < pendingEvents; i++)
            {
                XNextEvent(this->_display, &_event);
                if (_event.type == _damage_event_base + XDamageNotify)
                {
                    damage_event_flag = true;
                }
            }
            if (damage_event_flag)
            {
                XDamageSubtract(this->_display, _damage_handle, None, _damage_region);
                int rects_num = 0;
                XRectangle bounds;
                XRectangle *rects = XFixesFetchRegionAndBounds(this->_display, _damage_region,
                                                               &rects_num, &bounds);
                XFree(rects);
                return CaptureAndEncode(frame_size);
            }
            else
            {
                usleep(30 * 1000);
            }
        }
    }
}

uint8_t *Encoder::CaptureAndEncode(int *frame_size)
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

    _inputPic.planes[0] = _yuvData;
    _inputPic.planes[1] = _yuvData + luma_size;
    _inputPic.planes[2] = _yuvData + luma_size + chroma_size;
    _inputPic.stride[0] = _width;
    _inputPic.stride[1] = _width/2;
    _inputPic.stride[2] = _width/2;
    _inputPic.pts = _i_frame_counter;
    if (_next_frame_as_iframe)
    {
        // Set to force an iFrame
        std::cout << "Sending an iFrame from encoder";
        _inputPic.sliceType = X265_TYPE_IDR;
        _next_frame_as_iframe = false;
    }
    else
    {
        // Set to get back to normal encodings
        _inputPic.sliceType = X265_TYPE_AUTO;
    }
    _i_frame_counter++;

    int encodeReturnValue = 0;
    uint8_t *encodedBuffer = nullptr;
    try
    {
        encodeReturnValue = x265_encoder_encode(_x265Encoder, &_nal, &_noOfNal, &_inputPic, &_outputPic);

        std::cout << _noOfNal << std::endl;
        int size = 0;
        for (uint32_t i = 0; i < _noOfNal; i++)
        {
            size += _nal[i].sizeBytes;
        }

        *frame_size = size;
        if(size > 0)
            encodedBuffer = new uint8_t[size];
        for (uint32_t i = 0, k = 0; i < _noOfNal; i++)
        {
            memcpy(encodedBuffer + k, _nal[i].payload, _nal[i].sizeBytes);
            k += _nal[i].sizeBytes;
        }
        // std::cout<<i_frame_size;

        if (encodeReturnValue <= 0)
        {
            throw "No NAL is produced out of encoder.";
        }
    }
    catch (const char *msg)
    {
        throw "ERROR : Encoding failed. " + std::string(msg);
    }

    return encodedBuffer;
}

x265_encoder *Encoder::OpenEncoder(int width, int height)
{
    x265_param x265Params;
    x265_encoder *h;

    x265_param_default(&x265Params);

    int returnValue = x265_param_default_preset(&x265Params, x265_preset_names[2], x265_tune_names[3]);
    if (returnValue == 0)
    {
        std::cout << x265_preset_names[2] << " preset is applied and " << x265_tune_names[3] << " tune is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply default preset.";
    }

    /* Configure non-default params */
    x265Params.internalCsp = X265_CSP_I420;
    // Width and height should be even for encoder. Setting to next even number
    x265Params.sourceWidth = width + (width % 2);
    x265Params.sourceHeight = height + (height % 2);
    x265Params.bRepeatHeaders = 1;
    x265Params.bAnnexB = 1;
    x265Params.fpsNum = 25;
    x265Params.fpsDenom = 1;

    x265Params.keyframeMax = 5000;

    int crfValue = 25;
    x265Params.rc.rfConstant = crfValue;
    std::cout << "CRF set as " << crfValue << std::endl;

    returnValue = x265_param_apply_profile(&x265Params, x265_profile_names[0]);
    if (returnValue == 0)
    {
        std::cout << x265_profile_names[0] << " profile is applied." << std::endl;
    }
    else
    {
        throw "Failed to apply profile.";
    }

    x265_picture_init(&x265Params, &_inputPic);

    h = x265_encoder_open(&x265Params);
    if (h == NULL)
    {
        printf("x265_encoder_open err\n");
    }

    return h;
}

void Encoder::CleanUp()
{
    if (_damage_handle)
        XDamageDestroy(this->_display, _damage_handle);
    std::cout << "Cleanup invoked";
    delete this->_screenCapturer;
    // delete[] this->_yuvData;  // Not necessary as x264_picture_clean clears yuvData
    x265_picture_free(&this->_inputPic);
    x265_encoder_close(this->_x265Encoder);
    // sws_freeContext(_swsConverter);
}

void Encoder::SetForceNextFrame()
{
    this->_force_next_frame = true;
}

void Encoder::SendNextFrameAsIFrame()
{
    this->_next_frame_as_iframe = true;
    this->_force_next_frame = true;
}

void Encoder::InitXDamage()
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

Encoder::~Encoder()
{
    this->CleanUp();
    XCloseDisplay(this->_display);
}
} // namespace remoting
