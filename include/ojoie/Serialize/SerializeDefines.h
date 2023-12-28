//
// Created by aojoie on 5/7/2023.
//

#pragma once

#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/Serialize/Coder/IDPtrRemapper.hpp>

#define AN_SERIALIZE(x) \
	constexpr static const char* GetTypeString()	{ return #x; } \
    constexpr static bool MightContainIDPtr() { return true; }\
	template<typename Coder> \
	void transfer(Coder& coder);

#define AN_SERIALIZE_NO_IDPTR(x) \
	constexpr static const char* GetTypeString()	{ return #x; } \
    constexpr static bool MightContainIDPtr() { return false; }\
	template<typename Coder> \
	void transfer(Coder& coder);


#define TRANSFER(x) coder.transfer(x, #x)

#define INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL(x, decl)	\
template decl void x::transfer(YamlEncoder& coder); \
template decl void x::transfer(YamlDecoder& coder); \
template decl void x::transfer(IDPtrRemapper<int>& coder);

#define INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL_NO_IDPTR(x, decl)	\
template decl void x::transfer(YamlEncoder& coder); \
template decl void x::transfer(YamlDecoder& coder);

#define INSTANTIATE_TEMPLATE_TRANSFER(x) INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL(x, )
#define INSTANTIATE_TEMPLATE_TRANSFER_NO_IDPTR(x) INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL_NO_IDPTR(x, )
