//
// Created by aojoie on 5/8/2023.
//

#ifndef OJOIE_SERIALIZEMETAFLAGS_H
#define OJOIE_SERIALIZEMETAFLAGS_H

namespace AN {

enum CoderMetaFlagBits {
    kCoderMetaFlagNone = 0,
    kHideInEditor      = 1 << 0,
    kFlowMappingStyle = 1 << 1
};


}

#endif//OJOIE_SERIALIZEMETAFLAGS_H
