#include "NfdShim.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "nfd.h"
#include "nfd_glfw3.h"

namespace Mirael::NfdShim
{

void Init()
{
    if (NFD_Init() != NFD_OKAY) {
        throw std::runtime_error("Failed to initialize NFD library.");
    }
    NFD_SetDisplayPropertiesFromGLFW();
}

void Quit() { NFD_Quit(); }

namespace // internal linkage for helpers
{

void setNativeParentWindow(nfdwindowhandle_t *parentWindow)
{
    auto *viewport = ImGui::GetWindowViewport();
    if (viewport && viewport->PlatformHandle) {
        GLFWwindow *window = static_cast<GLFWwindow *>(viewport->PlatformHandle);
        NFD_GetNativeWindowFromGLFWWindow(window, parentWindow);
    }
}

std::vector<nfdu8filteritem_t> getNfdFilters(const std::vector<Filter> &shimFilters)
{
    std::vector<nfdu8filteritem_t> nfdFilters;
    nfdFilters.reserve(shimFilters.size());
    for (const auto &filter : shimFilters) {
        nfdFilters.emplace_back(filter.name.c_str(), filter.extensions.c_str());
    }
    return nfdFilters;
}

}; // namespace

Results getOpenFilePath(const OpenArgs &args)
{
    Results results{};
    nfdu8char_t *outpath;
    nfdopendialogu8args_t nfdArgs = {0};
    auto nfdFilters               = getNfdFilters(args.filters);
    nfdArgs.filterList            = nfdFilters.data();
    nfdArgs.filterCount           = static_cast<nfdfiltersize_t>(nfdFilters.size());
    setNativeParentWindow(&nfdArgs.parentWindow);
    nfdresult_t resultCode = NFD_OpenDialogU8_With(&outpath, &nfdArgs);
    switch (resultCode) {
    case NFD_OKAY:
        results.filepath = outpath;
        NFD_FreePathU8(outpath);
        results.error = Error::None;
        break;
    case NFD_CANCEL:
        results.error = Error::Cancel;
        break;
    default:
        results.error        = Error::Other;
        results.errorMessage = NFD_GetError();
        break;
    }
    return results;
}

Results getSaveAsFilePath(const SaveArgs &args)
{
    Results results{};
    nfdu8char_t *outpath;
    nfdsavedialogu8args_t nfdArgs = {0};
    auto nfdFilters               = getNfdFilters(args.filters);
    nfdArgs.filterList            = nfdFilters.data();
    nfdArgs.filterCount           = static_cast<nfdfiltersize_t>(nfdFilters.size());
    nfdArgs.defaultName           = args.defaultName.c_str();
    setNativeParentWindow(&nfdArgs.parentWindow);
    nfdresult_t resultCode = NFD_SaveDialogU8_With(&outpath, &nfdArgs);
    switch (resultCode) {
    case NFD_OKAY:
        results.filepath = outpath;
        NFD_FreePathU8(outpath);
        results.error = Error::None;
        break;
    case NFD_CANCEL:
        results.error = Error::Cancel;
        break;
    default:
        results.error        = Error::Other;
        results.errorMessage = NFD_GetError();
        break;
    }
    return results;
}

}; // namespace Mirael::NfdShim
