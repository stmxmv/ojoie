//
// Created by aojoie on 6/18/2023.
//

#pragma once

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Template/RC.hpp>
#include <yaml.h>
#include <string>
#include <map>
#include <vector>

namespace AN {

class YAMLNode : public RefCounted<YAMLNode> {
public:

    virtual ~YAMLNode() = default;

};

template<typename T>
YAMLNode* MakeYAMLNode(const T& value);

class AN_API YAMLScalar : public YAMLNode {
    int intValue;
    bool intValid;

    float floatValue;
    bool floatValid;

    std::string stringValue;
    bool stringValid;

public:

    YAMLScalar() : intValid(false), floatValid(false), stringValid(false) {}
    YAMLScalar(Int32 i) : intValid(true), floatValid(false), stringValid(false), intValue(i) {} 
    YAMLScalar(UInt32 i) : intValid(true), floatValid(false), stringValid(false), intValue(i) {} 
    YAMLScalar(UInt64 i) : intValid(false), floatValid(false), stringValid(false) {} 
    YAMLScalar(float f) : intValid(false), floatValid(true), stringValid(false), floatValue(f) {}
	 
    YAMLScalar(std::string s) : intValid(false), floatValid(false), stringValid(true), stringValue(std::move(s)) {}
    YAMLScalar(const char* s) : intValid(false), floatValid(false), stringValid(true), stringValue(s) {}
	
    bool isValid() { return intValid || floatValid || stringValid; }
	
    int	getIntValue();
    float getFloatValue();
    const std::string & getStringValue();
    
    operator UInt64() { return 0; }
    operator Int32() { return getIntValue(); }
    operator UInt32() { return getIntValue(); }
    operator Int16() { return getIntValue(); }
    operator UInt16() { return getIntValue(); }
    operator Int8() { return getIntValue(); }
    operator UInt8() { return getIntValue(); }
    operator char() { return getIntValue(); }
    operator float() { return getFloatValue(); }
    operator const std::string& () { return getStringValue(); }
};

class AN_API YAMLMapping : public YAMLNode {
    std::vector<std::pair<YAMLScalar*, YAMLNode*>> content;
    std::multimap<std::string, int> index;

public:

    YAMLMapping() {}
    explicit YAMLMapping(bool inlineStyle) {}

    void reserve(size_t size) {
        content.reserve (size);
    }

    void append(YAMLScalar* key, YAMLNode* value) {
        content.emplace_back(key, value);
        index.insert(std::make_pair(key->getStringValue(), (int) content.size() - 1));
    }

    template<typename T>
    void append(YAMLScalar* key, const T& value) {
        append( key, MakeYAMLNode(value) );
    }
    template<typename T1,typename T2>
    void append(const T1& key, const T2& value) {
        append( new YAMLScalar(key), MakeYAMLNode(value) );
    }

    YAMLNode* get(const std::string &key)  const {
        auto found = index.find(key);
        if( found != index.end() )
            return content[found->second].second;
        return nullptr;
    }

    void remove(const std::string& key);

    int size() const { return content.size(); }

    typedef std::vector<std::pair<YAMLScalar*,YAMLNode*> >::const_iterator const_iterator;
    typedef std::vector<std::pair<YAMLScalar*,YAMLNode*> >::iterator iterator;

    const_iterator begin() const {
        return content.begin();
    }

    const_iterator end() const {
        return content.end();
    }

    iterator begin() { return content.begin(); }
    iterator end() { return content.end(); }

    void clear () {
        for (auto i = content.begin(); i != content.end(); i++) {
            i->first->release();
            i->second->release();
        }
        content.clear();
        index.clear();
    }

    ~YAMLMapping() {
        clear();
    }

};

class AN_API YAMLSequence : public YAMLNode {
    std::vector<YAMLNode *> content;
public:

    YAMLSequence() = default;

    void reserve(size_t size) {
        content.reserve (size);
    }

    void append(YAMLNode* value) {
        content.push_back(value);
    }

    template<typename T>
    void append(const T& value) {
        append(MakeYAMLNode(value));
    }

    YAMLNode* get(int key)  const {
        if (key < 0 || key >= content.size()) return NULL;
        return content[key];
    }

    int size() { return content.size(); }

    typedef std::vector<YAMLNode*>::const_iterator const_iterator;
    const_iterator begin() {
        return content.begin();
    }
    const_iterator end() {
        return content.end();
    }

    ~YAMLSequence() {
        for (auto i = content.begin(); i != content.end(); i++) {
            (*i)->release();
        }
    }
};

template<typename T>
YAMLNode* MakeYAMLNode(const T& value) {
    return (YAMLNode*) new YAMLScalar(value);
}

AN_API YAMLNode* YAMLDocNodeToNode(yaml_document_t* doc, yaml_node_t*);
AN_API YAMLScalar* YAMLDocNodeToScalar(yaml_document_t* doc, yaml_node_t*);
AN_API YAMLMapping* YAMLDocNodeToMapping(yaml_document_t* doc, yaml_node_t*, bool useInlineStyle = false);
AN_API YAMLSequence* YAMLDocNodeToSequence(yaml_document_t* doc, yaml_node_t*);

}
