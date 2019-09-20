#include "../headers/screen_resources.h"

namespace remoting
{
ScreenResources::ScreenResources() : resources_(nullptr)
{
}

ScreenResources::~ScreenResources()
{
    Release();
}

bool ScreenResources::Refresh(Display *display, Window window)
{
    Release();
    resources_ = XRRGetScreenResources(display, window);
    return resources_ != nullptr;
}

void ScreenResources::Release()
{
    if (resources_)
    {
        XRRFreeScreenResources(resources_);
        resources_ = nullptr;
    }
}

bool ScreenResources::TryGetOutput(const unsigned int outputIndex, RROutput *output)
{
    if (resources_ == nullptr)
        return false;
    *output = resources_->outputs[outputIndex];
    return true;
}

XRROutputInfo *ScreenResources::GetOutputInfo(Display *display, const RROutput output_id)
{
    if (resources_ == nullptr)
        return nullptr;
    return XRRGetOutputInfo(display, resources_, output_id);
}

XRRScreenResources *ScreenResources::get() { return resources_; }
} // namespace remoting