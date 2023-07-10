//
// Created by Aleudillonam on 7/4/2023.
//

#include "Project/Project.hpp"

#include <string>

namespace AN::Editor {

static std::string s_ProjectRoot;

const char *GetProjectRoot() {
    return s_ProjectRoot.c_str();
}

void SetProjectRoot(const char *path) {
    s_ProjectRoot = path;
}

}