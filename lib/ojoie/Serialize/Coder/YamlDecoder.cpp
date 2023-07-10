//
// Created by Aleudillonam on 5/2/2023.
//

#include "Serialize/Coder/YamlDecoder.hpp"
#include <ojoie/Allocator/MemoryDefines.h>
#include "Utility/Log.h"
#include "Utility/String.hpp"
#include <yaml.h>

namespace AN {

int YamlDecoder::ReadStringHandler(void *data, unsigned char *buffer, size_t size, size_t *size_read) {
    YamlDecoder *read = (YamlDecoder *)data;

    if (read->_readOffset + size > read->_endOffset)
        size = read->_endOffset - read->_readOffset;

    const char* readData = reinterpret_cast<const char*>(read->_readData);

    memcpy(buffer, readData + read->_readOffset, size);
    read->_readOffset += size;
    *size_read = size;
    return true;
}

int YamlDecoder::ReadStreamHandler(void *data, unsigned char *buffer, size_t size, size_t *size_read) {
    InputStream *is = ((YamlDecoder *)data)->_inputStream;
    *size_read = is->read(buffer, size);
    return true;
}

void YamlDecoder::init_internal(yaml_read_handler_t *handler) {
    _document = (yaml_document_t *)AN_CALLOC(1, sizeof(yaml_document_t));
    yaml_parser_t parser{};

    if (!yaml_parser_initialize(&parser)) {
        AN_LOG(Error, "Could not initialize yaml parser");
        return;
    }

    yaml_parser_set_input(&parser, handler, this);
    yaml_parser_load(&parser, _document);

    if (parser.error != YAML_NO_ERROR) {
       AN_LOG(Error, "Unable to parse YAML file: [%s] at line %d", parser.problem, (int)parser.problem_mark.line);
    }

    yaml_parser_delete(&parser);
    _currentNode = yaml_document_get_root_node(_document);


}

YamlDecoder::YamlDecoder(const char *strBuffer, int size)
    : bDidReadLastProperty() {
    _readOffset = 0;
    _endOffset  = size;
    _readData   = const_cast<char *>(strBuffer);
    init_internal(ReadStringHandler);
}

YamlDecoder::YamlDecoder(InputStream &inputStream)
    : bDidReadLastProperty() {
    _readOffset = 0;

    _inputStream = &inputStream;
    init_internal(ReadStreamHandler);
}

YamlDecoder::~YamlDecoder() {
    yaml_document_delete(_document);
    ANSafeFree(_document);
}

yaml_node_t *YamlDecoder::getValueForKey(yaml_node_t *parentNode, const char *keystr) {
    if (parentNode && parentNode->type == YAML_MAPPING_NODE) {
       // The code below does not handle empty yaml arrays.
       if (parentNode->data.mapping.pairs.top == parentNode->data.mapping.pairs.start)
           return nullptr;

       yaml_node_pair_t *start;
       if (_cachedIndex < parentNode->data.mapping.pairs.top && _cachedIndex >= parentNode->data.mapping.pairs.start)
           start = _cachedIndex;
       else
           start = parentNode->data.mapping.pairs.start;

       yaml_node_pair_t *top = parentNode->data.mapping.pairs.top;
       yaml_node_pair_t *i   = start;

       do {
           yaml_node_pair_t *next = i + 1;
           if (next == top)
               next = parentNode->data.mapping.pairs.start;

           yaml_node_t *key = yaml_document_get_node(_document, i->key);
           if (key == nullptr) {
               // I've seen a crash bug report with no repro, indicating that this is happening.
               // If you ever get this error and can repro it, let me know! jonas.
               AN_LOG(Error, "YAML Node is nullptr!");
           } else {
               ANAssert(key->type == YAML_SCALAR_NODE);
               if (strcmp((char *) key->data.scalar.value, keystr) == 0) {
                   _cachedIndex = next;
                   return yaml_document_get_node(_document, i->value);
               }
           }
           i = next;
       } while (i != start);
    }
    return nullptr;
}

void YamlDecoder::beginMetaGroup(const char *name) {
    _metaParents.push_back(_currentNode);
    _currentNode = getValueForKey(_currentNode, name);
}

void YamlDecoder::endMetaGroup() {
    _currentNode = _metaParents.back();
    _metaParents.pop_back();
}

YAMLNode *YamlDecoder::getCurrentNode() {
    return YAMLDocNodeToNode(_document, _currentNode);
}


template<>
AN_API void YamlDecoder::transferPrimitiveData<float>(float &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%f", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<double>(double &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%lf", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<Int32>(Int32 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%d", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<UInt32>(UInt32 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%u", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<Int16>(Int16 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%hd", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<UInt16>(UInt16 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%hu", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<UInt64>(UInt64 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%llu", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<Int64>(Int64 &data) {
    sscanf((char*)_currentNode->data.scalar.value, "%lld", &data);
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<char>(char &data) {
    int i;
    sscanf ((char*)_currentNode->data.scalar.value, "%d", &i);
    data = (char) i;
}

template<>
AN_API void YamlDecoder::transferPrimitiveData<unsigned char>(unsigned char &data) {
    int i;
    sscanf ((char*)_currentNode->data.scalar.value, "%d", &i);
    data = (char) i;
}

size_t YamlDecoder::transferStringData(char *data, size_t size) {
    if (size == 0 || data == nullptr) {
        return strlen((char*)_currentNode->data.scalar.value);
    }

    memcpy(data, (char*)_currentNode->data.scalar.value, size);
    return size;
}

void YamlDecoder::transferTypelessData(void *data, size_t size, int metaFlags) {
    std::string dataString;
    transfer(dataString, "_typelessdata", metaFlags);
    dataString.resize(size * 2);
    HexStringToBytes (dataString.data(), size, data);
}

}