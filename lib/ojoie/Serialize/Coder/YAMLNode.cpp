//
// Created by aojoie on 6/18/2023.
//

#include "Serialize/Coder/YAMLNode.hpp"

#include <sstream>

namespace AN {


int YAMLScalar::getIntValue() {
    if (!intValid) {
        if (floatValid) {
            intValue = (int) floatValue;
            intValid = true;
        } else if (stringValid) {
            std::istringstream convert(stringValue.c_str());
            convert.imbue(std::locale("C"));// Ensure there are no std::locale issues when converting numbers to strings and back again
            convert >> intValue;
            intValid = true;
        } else {
            return 0;
        }
    }
    return intValue;
}

float YAMLScalar::getFloatValue() {
    if (!floatValid) {
        if (stringValid) {
            std::istringstream convert(stringValue.c_str());
            convert.imbue(std::locale("C"));// Ensure there are no std::locale issues when converting numbers to strings and back again
            convert >> floatValue;
            floatValid = true;
        } else if (intValid) {
            floatValue = (float) intValue;
            floatValid = true;
        } else {
            return 0.0;
        }
    }
    return floatValue;
}

const std::string &YAMLScalar::getStringValue() {
    if (!stringValid) {
        std::ostringstream convert;
        convert.imbue(std::locale("C"));// Ensure there are no std::locale issues when converting numbers to strings and back again
        if (floatValid) {
            convert.precision(7);// 7 digits of precision is about what a float can handle
            convert << floatValue;
            stringValue = std::string(convert.str().c_str());

            stringValid = true;
        } else if (intValid) {
            convert << intValue;
            stringValue = std::string(convert.str().c_str());

            stringValid = true;
        }
    }

    return stringValue;
}

void YAMLMapping::remove(const std::string &key) {
    auto found = index.find(key);
    if (found != index.end()) {
        int idx = found->second;

        // reindex map: update indices that are >found->second
        index.erase(found);
        for (auto it = index.begin(), end = index.end(); it != end; ++it) {
            if (it->second > idx)
                it->second--;
        }

        content[idx].first->release();
        content[idx].second->release();
        content.erase(content.begin() + idx);
    }
}

YAMLScalar *YAMLDocNodeToScalar(yaml_document_t *doc, yaml_node_t *node) {
    return new YAMLScalar((const char*)node->data.scalar.value);
}

YAMLMapping *YAMLDocNodeToMapping(yaml_document_t *doc, yaml_node_t *node, bool useInlineStyle) {
    yaml_node_pair_t* start = node->data.mapping.pairs.start;
    yaml_node_pair_t* top   = node->data.mapping.pairs.top;

    YAMLMapping* result = new YAMLMapping(useInlineStyle);
    result->reserve (top - start);
    for(yaml_node_pair_t* i = start; i != top; i++)
    {
        YAMLScalar* key=dynamic_cast<YAMLScalar*> (YAMLDocNodeToNode(doc, yaml_document_get_node(doc, i->key)));
        // TODO : fix potential memory leak - key is not released if dynamic_cast fails
        if( ! key )
        {
            // TODO log error
            continue;
        }
        result->append(key, YAMLDocNodeToNode(doc, yaml_document_get_node(doc, i->value)));
    }

    return result;
}
YAMLSequence *YAMLDocNodeToSequence(yaml_document_t *doc, yaml_node_t *node) {
    yaml_node_item_t* start = node->data.sequence.items.start;
    yaml_node_item_t* top   = node->data.sequence.items.top;

    YAMLSequence* result = new YAMLSequence();
    result->reserve(top - start);
    for(yaml_node_item_t* i = start; i != top; i++) {
        result->append(YAMLDocNodeToNode(doc, yaml_document_get_node(doc, *i)));
    }

    return result;
}

YAMLNode *YAMLDocNodeToNode(yaml_document_t *doc, yaml_node_t *node) {
    if (!node) return nullptr;
    switch (node->type) {
        case YAML_NO_NODE:
            return nullptr;
        case YAML_SCALAR_NODE:
            return YAMLDocNodeToScalar(doc, node);
        case YAML_MAPPING_NODE: {
            bool useInlineStyle = node->data.mapping.style == YAML_FLOW_MAPPING_STYLE;
            return YAMLDocNodeToMapping(doc, node, useInlineStyle);
        }
        case YAML_SEQUENCE_NODE:
            return YAMLDocNodeToSequence(doc, node);
    }
    return nullptr;
}

}// namespace AN