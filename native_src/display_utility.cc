#include <napi.h>
#include <iostream>
#include "../headers/display_utility_x11.h"
using namespace remoting;

Napi::String GetOutputName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::String outputName;

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return outputName;
    }

    if (!info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return outputName;
    }

    unsigned int rROutput = info[0].As<Napi::Number>().Int32Value();

    std::unique_ptr<DisplayUtilityX11> desktopInfo = DisplayUtilityX11::Create();

    return Napi::String::New(env, desktopInfo->GetOutputName(rROutput));
}

Napi::Object GetCurrentResolution(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::Object currentResolution;

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return currentResolution;
    }

    if (!info[0].IsNumber())
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return currentResolution;
    }

    unsigned int rROutput = info[0].As<Napi::Number>().Int32Value();
    std::unique_ptr<DisplayUtilityX11> desktopInfo = DisplayUtilityX11::Create();
    std::unique_ptr<OutputResolution> resolution = desktopInfo->GetCurrentResolution(rROutput);
    if (resolution != nullptr)
    {
        std::cout << "current Resolution : " << resolution->width() << "x" << resolution->height() << std::endl;
        currentResolution = Napi::Object::New(env);
        currentResolution.Set(Napi::String::New(env, "width"), Napi::Number::New(env, resolution->width()));
        currentResolution.Set(Napi::String::New(env, "height"), Napi::Number::New(env, resolution->height()));
    }
    else
    {
        Napi::Error::New(env, "Could not get the current resolution of the output. Please Try again.");
    }
    return currentResolution;
}

void SetResolution(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2)
    {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsNumber() || !info[1].IsObject() || !info[1].ToObject().Has("width") || !info[1].ToObject().Has("height"))
    {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return;
    }

    unsigned int rROutput = info[0].As<Napi::Number>().Int32Value();
    std::unique_ptr<DisplayUtilityX11> desktopInfo = DisplayUtilityX11::Create();
    
    std::string outputName = desktopInfo->GetOutputName(rROutput);
    std::string width = info[1].ToObject().Get("width").ToString();
    std::string height = info[1].ToObject().Get("height").ToString();
    std::string resolution = width + "x" + height;

    std::string setResolutionCommnad = "xrandr --output " + outputName + " --mode " + resolution;
    int return_value = system(setResolutionCommnad.c_str());
    std::cout << "The value returned by command " << setResolutionCommnad << " was: " << return_value << std::endl;
    return;
}

Napi::Number GetPrimaryRROutput(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    std::unique_ptr<DisplayUtilityX11> desktopInfo = DisplayUtilityX11::Create();
    RROutput outputIndex = desktopInfo->GetPrimaryRROutput();

    return Napi::Number::New(env, outputIndex);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "getOutputName"), Napi::Function::New(env, GetOutputName));
    exports.Set(Napi::String::New(env, "getCurrentResolution"), Napi::Function::New(env, GetCurrentResolution));
    exports.Set(Napi::String::New(env, "setResolution"), Napi::Function::New(env, SetResolution));
    exports.Set(Napi::String::New(env, "getPrimaryRROutput"), Napi::Function::New(env, GetPrimaryRROutput));
    return exports;
}

NODE_API_MODULE(desktop_info, Init);
