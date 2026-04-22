#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Mirael::NfdShim
{

void Init();
void Quit();

enum Error { None, Cancel, Other };

struct Results {
    std::filesystem::path filepath;
    Error error;
    std::string errorMessage;
    bool good() const { return error == Error::None; }
    bool bad() const { return error != Error::None && error != Error::Cancel; }
};

struct Filter {
    std::string name;
    std::string extensions;
};

struct OpenArgs {
    std::vector<Filter> filters;
};

struct SaveArgs {
    std::vector<Filter> filters;
    std::string defaultName;
};

Results getOpenFilePath(const OpenArgs &args);
Results getSaveAsFilePath(const SaveArgs &args);

}; // namespace Mirael::NfdShim
