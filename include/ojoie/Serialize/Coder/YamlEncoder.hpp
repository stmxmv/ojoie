//
// Created by Aleudillonam on 5/2/2023.
//

#ifndef OJOIE_YAMLENCODER_HPP
#define OJOIE_YAMLENCODER_HPP

#include <ojoie/IO/OutputStream.hpp>
#include <ojoie/Serialize/SerializeTraits.hpp>
#include <vector>

#include <yaml.h>

typedef struct yaml_document_s yaml_document_t;
typedef int                    yaml_write_handler_t(void *data, unsigned char *buffer, size_t size);

namespace AN {

class AN_API YamlEncoder {

    bool             _error;
    yaml_document_t *_document;

    int _currentNode;

    struct MetaParent {
        int         node;
        std::string name;
    };

    std::vector<MetaParent> _metaParents;

    int  newMapping();
    int  newSequence();
    int  getNode();
    void appendToNode(int parentNode, const char *keyStr, int valueNode);
    void transferStringToCurrentNode(const char *str, size_t size = 0);

    void outputToHandler(yaml_write_handler_t *handler, void *data);

public:
    YamlEncoder();
    ~YamlEncoder();

    constexpr static bool IsEncoding() { return true; }
    constexpr static bool IsDecoding() { return false; }

    void outputToStream(OutputStream &outputStream);
    void outputToString(std::string &str);

    void startSequence();

    template<typename T>
    void transfer(T &data, const char *name, int metaFlags = 0);

    void transferTypeless(size_t &value, const char *name, int metaFlag = 0) {
        transfer(value, name, metaFlag);
    }

    void transferTypelessData(void *data, size_t size, int metaFlags = 0);

    void beginMetaGroup(const char *name);
    void endMetaGroup();

    /// Internal function. Should only be called from SerializeTraits
    template<typename T>
    void transferPrimitiveData(T &data);

    void transferStringData(const char *data, size_t size);

    template<typename _Array>
    void transferSTLStyleArray(_Array &array);

    template<typename T>
    void transferSTLStyleMap(T &data);

    template<typename T>
    void transferPair(T &data, int parent = -1);
};

template<typename T>
void YamlEncoder::transfer(T &data, const char *name, int metaFlags) {
    if (_error) return;

    int parent   = getNode();
    _currentNode = -1;

    SerializeTraits<T>::Transfer(data, *this);

    if (_currentNode != -1) {
        appendToNode(parent, name, _currentNode);
    }
    _currentNode = parent;
}


template<>
inline void YamlEncoder::transferPrimitiveData<float>(float &data) {
    char valueStr[64];
    snprintf(valueStr, 64, "%f", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<double>(double &data) {
    char valueStr[64];
    snprintf(valueStr, 64, "%lf", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<Int32>(Int32 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%d", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<UInt32>(UInt32 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%u", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<UInt16>(UInt16 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%hu", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<Int16>(Int16 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%hd", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<UInt64>(UInt64 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%llu", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<Int64>(Int64 &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%lld", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<char>(char &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%hhd", data);
    transferStringToCurrentNode(valueStr);
}

template<>
inline void YamlEncoder::transferPrimitiveData<unsigned char>(unsigned char &data) {
    char valueStr[16];
    snprintf(valueStr, 16, "%hhu", data);
    transferStringToCurrentNode(valueStr);
}

template<typename _Array>
void YamlEncoder::transferSTLStyleArray(_Array &array) {
    _currentNode = newSequence();
    auto i       = std::begin(array);
    auto end     = std::end(array);
    while (i != end) {
        transfer(*i, "data");
        ++i;
    }
}

template<typename T>
void YamlEncoder::transferPair(T &data, int parent) {
    typedef typename T::first_type  first_type;
    typedef typename T::second_type second_type;
    if (parent == -1)
        parent = newMapping();

    _currentNode = -1;
    SerializeTraits<first_type>::Transfer(data.first, *this);
    int key = _currentNode;

    _currentNode = -1;
    SerializeTraits<second_type>::Transfer(data.second, *this);

    yaml_document_append_mapping_pair(_document, parent, key, _currentNode);

    _currentNode = parent;
}

template<typename T>
void YamlEncoder::transferSTLStyleMap(T &data) {
    _currentNode = newMapping();

    typename T::iterator i   = data.begin();
    typename T::iterator end = data.end();

    // maps value_type is: pair<const First, Second>
    // So we have to write to maps non-const value type
    typedef typename NonConstContainerValueType<T>::value_type non_const_value_type;
    typedef typename non_const_value_type::first_type          first_type;
    while (i != end) {
        non_const_value_type &p = (non_const_value_type &) (*i);
        if (SerializeTraits<first_type>::IsPrimitiveType())
            transferPair(p, _currentNode);
        else
            transfer(p, "data");
        ++i;
    }
}

}// namespace AN

#endif//OJOIE_YAMLENCODER_HPP
