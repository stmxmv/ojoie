//
// Created by aojoie on 5/7/2023.
//

#ifndef OJOIE_SERIALIZEDEFINES_H
#define OJOIE_SERIALIZEDEFINES_H

#include <ojoie/Serialize/Coder/YamlEncoder.hpp>
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>

#define DECLARE_SERIALIZE(x) \
	constexpr static const char* GetTypeString()	{ return #x; } \
    constexpr static bool MightContainIDPtr() { return true; }\
	template<typename Coder> \
	void transfer(Coder& coder);

#define DECLARE_SERIALIZE_NO_IDPTR(x) \
	constexpr static const char* GetTypeString()	{ return #x; } \
    constexpr static bool MightContainIDPtr() { return false; }\
	template<typename Coder> \
	void transfer(Coder& coder);


#define TRANSFER(x) coder.transfer(x, #x)

#define INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL(x, decl)	\
template decl void x::transfer(YamlEncoder& coder); \
template decl void x::transfer(YamlDecoder& coder);

#define INSTANTIATE_TEMPLATE_TRANSFER(x) INSTANTIATE_TEMPLATE_TRANSFER_WITH_DECL(x, )

#endif//OJOIE_SERIALIZEDEFINES_H
