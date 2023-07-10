//
// Created by Aleudillonam on 5/2/2023.
//

#ifndef OJOIE_YAMLDECODER_HPP
#define OJOIE_YAMLDECODER_HPP

#include <ojoie/Serialize/SerializeTraits.hpp>
#include <ojoie/Serialize/Coder/YAMLNode.hpp>
#include <ojoie/IO/InputStream.hpp>
#include <vector>

#include <yaml.h>

typedef struct yaml_document_s yaml_document_t;
typedef struct yaml_node_s yaml_node_t;
typedef struct yaml_node_pair_s yaml_node_pair_t;
typedef int yaml_read_handler_t(void *data, unsigned char *buffer, size_t size, size_t *size_read);

namespace AN {

class AN_API YamlDecoder {

    yaml_document_t  *_document;
    yaml_node_t      *_currentNode;
    yaml_node_pair_t *_cachedIndex;

    std::vector<yaml_node_t *> _metaParents;

    std::string _currentType;
    bool bDidReadLastProperty;

    void* _readData;
    size_t _readOffset;
    size_t _endOffset;

    InputStream *_inputStream;

    static int ReadStringHandler(void *data, unsigned char *buffer, size_t size, size_t *size_read);
    static int ReadStreamHandler(void *data, unsigned char *buffer, size_t size, size_t *size_read);
    void init_internal(yaml_read_handler_t *handler);

    yaml_node_t *getValueForKey(yaml_node_t* parentNode, const char* keystr);

public:

    YamlDecoder(const char* strBuffer, int size);
    explicit YamlDecoder(InputStream &inputStream);
    ~YamlDecoder();

    constexpr static bool IsEncoding() { return false; }
    constexpr static bool IsDecoding() { return true; }

    template<typename T>
    void transfer(T& data, const char* name, int metaFlags = 0);

    void transferTypeless(size_t &value, const char* name, int metaFlag = 0) {
        transfer(value, name, metaFlag);
    }

    void transferTypelessData(void* data, size_t size, int metaFlags = 0);

    void beginMetaGroup(const char *name);
    void endMetaGroup();

    /// caller must release after used
    YAMLNode *getCurrentNode();

    /// Internal function. Should only be called from SerializeTraits
    template<typename T>
    void transferPrimitiveData(T &data);

    size_t transferStringData(char *data, size_t size);

    template<typename _Array>
    void transferSTLStyleArray(_Array &array);

    template<typename T>
    void transferSTLStyleMap(T& data);

    template<typename T>
    void transferPair(T& data, yaml_node_pair_t* pair);
};

template<typename T>
void YamlDecoder::transfer(T &data, const char *name, int metaFlags) {
    bDidReadLastProperty = false;

    yaml_node_t *parentNode = _currentNode;
    _currentNode = getValueForKey(parentNode, name);

    std::string parentType = _currentType;
    _currentType = SerializeTraits<T>::GetTypeString();

    if (_currentNode != nullptr) {
        SerializeTraits<T>::Transfer(data, *this);
        bDidReadLastProperty = true;
    }
    _currentNode = parentNode;
    _currentType = parentType;
}

template<typename _Array>
void YamlDecoder::transferSTLStyleArray(_Array &array) {
    yaml_node_t *parentNode = _currentNode;
    typedef std::decay_t<typename _Array::value_type> value_type;

    yaml_node_item_t* start = _currentNode->data.sequence.items.start;
    yaml_node_item_t* top   = _currentNode->data.sequence.items.top;

    array.resize(top - start);
    auto dataIterator = std::begin(array);

    for(yaml_node_item_t* i = start; i != top; i++) {
        _currentNode = yaml_document_get_node(_document, *i);
        SerializeTraits<value_type>::Transfer(*dataIterator, *this);
        ++dataIterator;
    }

}

template<typename T>
void YamlDecoder::transferSTLStyleMap(T &data) {
    if (_currentNode->type == YAML_MAPPING_NODE) {
        yaml_node_pair_t *start = _currentNode->data.mapping.pairs.start;
        yaml_node_pair_t *top   = _currentNode->data.mapping.pairs.top;

        data.clear();

        yaml_node_t *parentNode = _currentNode;

        for (yaml_node_pair_t *i = start; i != top; i++) {
            typedef typename NonConstContainerValueType<T>::value_type non_const_value_type;
            typedef typename non_const_value_type::first_type first_type;
            non_const_value_type                              p;

            if (!SerializeTraits<first_type>::IsPrimitiveType()) {
                _currentNode = yaml_document_get_node(_document, i->value);
                SerializeTraits<non_const_value_type>::Transfer(p, *this);
            } else
                transferPair(p, i);

            data.insert(p);
        }
        _currentNode = parentNode;
    }
}
template<typename T>
void YamlDecoder::transferPair(T &data, yaml_node_pair_t* pair) {
    typedef typename T::first_type  first_type;
    typedef typename T::second_type second_type;

    if (pair == nullptr) {
        yaml_node_pair_t *start = _currentNode->data.mapping.pairs.start;
        yaml_node_pair_t *top   = _currentNode->data.mapping.pairs.top;
        if (start == top)
            return;
        pair = start;
    }

    yaml_node_t *parent = _currentNode;
    _currentNode        = yaml_document_get_node(_document, pair->key);
    SerializeTraits<first_type>::Transfer(data.first, *this);
    _currentNode = yaml_document_get_node(_document, pair->value);
    SerializeTraits<second_type>::Transfer(data.second, *this);
    _currentNode = parent;
}

}

#endif//OJOIE_YAMLDECODER_HPP
