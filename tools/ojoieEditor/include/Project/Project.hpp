//
// Created by Aleudillonam on 7/4/2023.
//

#pragma once

#include <ojoie/Utility/Path.hpp>
#include <string>

namespace AN::Editor {


const char *GetProjectRoot();

void SetProjectRoot(const char *path);

inline std::string GetAssetFolder() {
    Path path(GetProjectRoot());
    return path.string();
}

}
