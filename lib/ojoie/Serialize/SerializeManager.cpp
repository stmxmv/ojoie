//
// Created by Aleudillonam on 8/2/2023.
//

#include "Serialize/SerializeManager.hpp"

#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/IO/FileOutputStream.hpp>

namespace AN {


bool SerializeManager::serializeObjectAtPath(Object *object, const char *path) {
    YamlEncoder yamlEncoder;
    File file;
    file.open(path, AN::kFilePermissionWrite);
    FileOutputStream fileOutputStream(file);
    object->redirectTransferVirtual(yamlEncoder);
    yamlEncoder.outputToStream(fileOutputStream);
    return true;
}


SerializeManager &GetSerializeManager() {
    static SerializeManager serializeManager;
    return serializeManager;
}

}