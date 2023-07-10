//
// Created by Aleudillonam on 5/2/2023.
//

#include "Serialize/Coder/YamlEncoder.hpp"
#include "Allocator/MemoryDefines.h"
#include "Utility/Log.h"
#include "Utility/String.hpp"

#include <yaml.h>

extern "C" {
#include <yaml_private.h>
}


namespace AN {


YamlEncoder::YamlEncoder() {
    _currentNode = -1;
    _document = (yaml_document_t *)AN_MALLOC(sizeof(yaml_document_t));
    yaml_version_directive_t version;
    version.major = 1;
    version.minor = 1;
    yaml_tag_directive_t tag;
    tag.handle = (yaml_char_t*) "!AN!";
    tag.prefix = (yaml_char_t*) "tag:an.com,2023:";

    if (!yaml_document_initialize(_document, &version, &tag, &tag + 1, 1, 1)) {
        AN_LOG(Error, "yaml_document_initialize failed.");
        _error = true;
    } else {
        _error = false;
    }
}

YamlEncoder::~YamlEncoder() {
    yaml_document_delete(_document);
    ANSafeFree(_document);
}

static int StringOutputHandler(void *data, unsigned char *buffer, size_t size) {
    std::string* theString = reinterpret_cast<std::string *> (data);
    theString->append((char *) buffer, size);
    return 1;
}

static int StreamOutputHandler(void *data, unsigned char *buffer, size_t size) {
    OutputStream* os = reinterpret_cast<OutputStream *> (data);
    os->write(buffer, size);
    return 1;
}

void YamlEncoder::outputToHandler(yaml_write_handler_t *handler, void *data) {
    yaml_node_t *root = yaml_document_get_root_node (_document);
    if (root->type == YAML_MAPPING_NODE && root->data.mapping.pairs.start != root->data.mapping.pairs.top) {
        yaml_emitter_t emitter;
        memset(&emitter, 0, sizeof(emitter));

        if (!yaml_emitter_initialize (&emitter)) {
            AN_LOG(Error, "YamlEncoder: yaml_emitter_initialize failed.");
            return;
        }

        yaml_emitter_set_output(&emitter, handler, data);
        yaml_emitter_dump(&emitter, _document);

        if (emitter.error != YAML_NO_ERROR)
            AN_LOG(Error, "YamlEncoder: Unable to write text file : %s.", emitter.problem);

        yaml_emitter_delete(&emitter);
        memset(_document, 0, sizeof(*_document)); /// when emitted, document got free, zero it to prevent crash in destructor
    }
}

void YamlEncoder::outputToStream(OutputStream &outputStream) {
    if (_error) {
        AN_LOG(Error, "Could not serialize text file because an error occurred - we probably ran out of memory.");
        return;
    }
    outputToHandler(StreamOutputHandler, reinterpret_cast<void *>(&outputStream));
}

void YamlEncoder::outputToString(std::string &str) {
    if (_error) {
        AN_LOG(Error, "Could not serialize text file because an error occurred - we probably ran out of memory.");
        return;
    }
    outputToHandler(StringOutputHandler, reinterpret_cast<void *>(&str));
}



void YamlEncoder::transferStringToCurrentNode(const char *str, size_t size) {
    if (_error)
        return;
    if (size == 0) {
        size = strlen(str);
    }
    int node = yaml_document_add_scalar(_document, nullptr, (yaml_char_t*)str, (int) size, YAML_ANY_SCALAR_STYLE);
    if (node)
        _currentNode = node;
    else
        _error = true;
}

void YamlEncoder::transferTypelessData(void *data, size_t size, int metaFlags) {
    std::string hexStr;
    hexStr.resize(size * 2);
    BytesToHexString(data, size, hexStr.data());
    transfer(hexStr, "_typelessdata");
}


int YamlEncoder::newMapping() {
    int node = yaml_document_add_mapping(_document, nullptr, YAML_ANY_MAPPING_STYLE);
    if (node == 0)
        _error = true;
    return node;
}

int YamlEncoder::newSequence() {
    int node = yaml_document_add_sequence(_document, NULL, YAML_ANY_SEQUENCE_STYLE);
    if (node == 0)
        _error = true;
    return node;
}

int YamlEncoder::getNode() {
    if (_currentNode == -1)
        _currentNode = newMapping();
    return _currentNode;
}

void YamlEncoder::appendToNode(int parentNode, const char *keyStr, int valueNode) {
    yaml_node_t* parent = yaml_document_get_node(_document, parentNode);
    switch (parent->type) {
        case YAML_MAPPING_NODE:
        {
            int keyNode = yaml_document_add_scalar(_document, nullptr, (yaml_char_t*)keyStr, (int) strlen(keyStr), YAML_ANY_SCALAR_STYLE);
            if (keyNode == 0)
                _error = true;
            yaml_document_append_mapping_pair(_document, parentNode, keyNode, valueNode);
        }
        break;

        case YAML_SEQUENCE_NODE:
            yaml_document_append_sequence_item(_document, parentNode, valueNode);
            break;

        default:
            AN_LOG(Error, "YamlEncoder: Unexpected node type.");
    }
}

void YamlEncoder::startSequence() {
    _currentNode = newSequence();
}

void YamlEncoder::beginMetaGroup(const char *name) {
    _metaParents.emplace_back();
    _metaParents.back().node = getNode();
    _metaParents.back().name = name;
    _currentNode = newMapping();
}

void YamlEncoder::endMetaGroup() {
    appendToNode(_metaParents.back().node, _metaParents.back().name.c_str(), _currentNode);
    _currentNode = _metaParents.back().node;
    _metaParents.pop_back();
}

void YamlEncoder::transferStringData(const char *data, size_t size) {
    transferStringToCurrentNode(data, size);
}

}