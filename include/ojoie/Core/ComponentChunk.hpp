//
// Created by aojoie on 3/31/2023.
//

#ifndef OJOIE_COMPONENTCHUNK_HPP
#define OJOIE_COMPONENTCHUNK_HPP

#include "ojoie/Configuration/typedef.h"

namespace AN {

// Cache-Lines size is (typically) 64 bytes
// it has to be an exponent of 2 and >= 16
constexpr static size_t ComponentChunkAlignment = 128;

// 16384 bytes : 16 KB
// it has to be a multiple of ChunkAlignment and an exponent of 2
constexpr static size_t ComponentChunkSize = 16384;

class alignas(ComponentChunkAlignment) ComponentChunk {
    uint8_t data[ComponentChunkSize];
public:

    struct ChunkInfo {
        ComponentChunk *nextChunk;
        uint64_t capacity;
        uint64_t componentNum;
        uint64_t freeIndex;
    };





};



}

#endif//OJOIE_COMPONENTCHUNK_HPP
