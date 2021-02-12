#include "../headers/tty_console_capturer.h"
#include <iostream>
#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

namespace remoting
{
TtyConsoleCapturer::TtyConsoleCapturer()
{   
    this->InitializeMonitorProperties();
}

void TtyConsoleCapturer::InitializeMonitorProperties()
{
    this->_frameBufferDescriptor = open("/dev/fb0", O_RDONLY);
    if (this->_frameBufferDescriptor < 0) {
        throw "cannot open framebuffer file";
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    ioctl(this->_frameBufferDescriptor, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(this->_frameBufferDescriptor, FBIOGET_FSCREENINFO, &finfo);
    this->_bytesPerPixel = vinfo.bits_per_pixel / 8;
    this->_width = finfo.line_length;
    this->_height = vinfo.yres;
    this->_data_ptr = NULL;

    std::cout << "line_length "     << finfo.line_length        << std::endl;
    std::cout << "xres "            << vinfo.xres               << std::endl;
    std::cout << "yres "            << vinfo.yres               << std::endl;
    std::cout << "xres_virtual "    << vinfo.xres_virtual       << std::endl;
    std::cout << "yres_virtual "    << vinfo.yres_virtual       << std::endl;
    std::cout << "xoffset "         << vinfo.xoffset            << std::endl;
    std::cout << "yoffset "         << vinfo.yoffset            << std::endl;
    std::cout << "bits/pixel "      << vinfo.bits_per_pixel     << std::endl;
}

uint8_t *TtyConsoleCapturer::GetDataPointer()
{
    return this->_data_ptr;
}

// just open file, mmap it, close file
void TtyConsoleCapturer::CaptureScreen()
{
    this->resetResources();
    this->_frameBufferDescriptor = open("/dev/fb0", O_RDONLY);
    if (this->_frameBufferDescriptor < 0) {
        throw "cannot open framebuffer file";
    }

    this->_data_ptr = (uint8_t*)mmap(NULL, this->_width * this->_height, PROT_READ, MAP_PRIVATE, this->_frameBufferDescriptor, 0);
}

int TtyConsoleCapturer::GetWidth()
{
    return this->_width / this->_bytesPerPixel;
}

int TtyConsoleCapturer::GetHeight()
{
    return this->_height;
}

TtyConsoleCapturer::~TtyConsoleCapturer()
{
    this->resetResources();
}

void TtyConsoleCapturer::resetResources()
{
    if (this->_data_ptr && munmap(this->_data_ptr, this->_width * this->_height) != 0) {
        std::cout << "last framebuffer memory could not be freed\n";
    }
    if (close(this->_frameBufferDescriptor) < 0) {
        std::cout << "cannot close framebuffer file\n";
    }
    this->_data_ptr = NULL;
}
} // namespace remoting
