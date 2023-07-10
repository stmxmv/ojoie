//
// Created by aojoie on 5/6/2023.
//

#ifndef OJOIE_SERIALIZETRAITS_HPP
#define OJOIE_SERIALIZETRAITS_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Core/Name.hpp>

#include <ojoie/Template/SmallVector.hpp>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>

namespace AN {

template<typename T>
struct SerializeTraitsBase {
    typedef T     value_type;
    static int    GetByteSize() { return sizeof(value_type); }
    static size_t GetAlignOf() { return alignof(value_type); }
    static bool   IsPrimitiveType() { return false; }
};

template<typename T>
struct SerializeTraitsBaseForPrimitiveType : public SerializeTraitsBase<T> {
    typedef T value_type;

    static bool IsPrimitiveType() { return true; }

    template<typename Coder>
    inline static void Transfer(value_type &data, Coder& transfer) {
        transfer.transferPrimitiveData(data);
    }
};

template<typename T>
struct SerializeTraits : public SerializeTraitsBase<T> {

    typedef T     value_type;

    constexpr static const char* GetTypeString() { return value_type::GetTypeString(); }
    constexpr static bool MightContainIDPtr() { return value_type::MightContainIDPtr(); }
    template<typename Coder>
    inline static void Transfer(value_type &data, Coder& transfer) {
        data.transfer(transfer);
    }
};

template<>
struct SerializeTraits<std::string> : public SerializeTraitsBase<std::string> {

    constexpr static const char* GetTypeString() { return "string"; }
    constexpr static bool MightContainIDPtr() { return false; }

    template<typename Coder>
    inline static void Transfer(value_type &data, Coder& transfer) {
        if constexpr (Coder::IsDecoding()) {
            data.resize(transfer.transferStringData(nullptr, 0));
        }
        transfer.transferStringData(data.data(), data.length());
    }
};

template<>
struct SerializeTraits<Name> : public SerializeTraitsBase<Name> {

    constexpr static const char* GetTypeString() { return "Name"; }
    constexpr static bool MightContainIDPtr() { return false; }
    template<typename Coder>
    inline static void Transfer(value_type &data, Coder& transfer) {
        if constexpr (Coder::IsDecoding()) {
            std::string tempString;
            tempString.resize(transfer.transferStringData(nullptr, 0));
            transfer.transferStringData(tempString.data(), tempString.length());
            data = tempString;
        } else {
            transfer.transferStringData(data.c_str(), data.string_view().length());
        }
    }
};

template<typename T, typename Allocator>
struct SerializeTraits<std::vector<T, Allocator>> : public SerializeTraitsBase<std::vector<T, Allocator> > {
    typedef std::vector<T,Allocator>	value_type;

    constexpr static const char* GetTypeString() { return "vector"; }
    constexpr static bool MightContainIDPtr() { return SerializeTraits<T>::MightContainIDPtr(); }
    template<class Coder>
    inline static void Transfer(value_type& data, Coder& transfer) {
        transfer.transferSTLStyleArray(data);
    }

};

template<typename T, size_t N>
struct SerializeTraits<SmallVector<T, N>> : public SerializeTraitsBase<SmallVector<T, N>> {
    typedef SmallVector<T, N> value_type;

    constexpr static const char* GetTypeString() { return "SmallVector"; }
    constexpr static bool MightContainIDPtr() { return SerializeTraits<T>::MightContainIDPtr(); }
    template<class Coder>
    inline static void Transfer(value_type& data, Coder& transfer) {
        transfer.transferSTLStyleArray(data);
    }

};

template<typename FirstClass, typename SecondClass>
struct SerializeTraits<std::pair<FirstClass, SecondClass>> : public SerializeTraitsBase<std::pair<FirstClass, SecondClass> > {
    typedef std::pair<FirstClass, SecondClass>	value_type;
    constexpr static const char* GetTypeString() { return "pair"; }
    constexpr static bool MightContainIDPtr() {
        return SerializeTraits<FirstClass>::MightContainIDPtr() ||
               SerializeTraits<SecondClass>::MightContainIDPtr();
    }
    template<typename Coder>
    inline static void Transfer (value_type& data, Coder& transfer) {
        transfer.transfer(data.first, "first");
        transfer.transfer(data.second, "second");
    }
};

template<typename FirstClass, typename SecondClass, typename Compare, typename Allocator>
struct SerializeTraits<std::map<FirstClass, SecondClass, Compare, Allocator>>
        : public SerializeTraitsBase<std::map<FirstClass, SecondClass, Compare, Allocator> > {

    typedef std::map<FirstClass, SecondClass, Compare, Allocator>	value_type;

    constexpr static const char* GetTypeString() { return "map"; }
    constexpr static bool MightContainIDPtr() {
        return SerializeTraits<FirstClass>::MightContainIDPtr() ||
               SerializeTraits<SecondClass>::MightContainIDPtr();
    }
    template<typename Coder>
    inline static void Transfer (value_type& data, Coder& transfer) {
        transfer.transferSTLStyleMap(data);
    }
};

template<typename FirstClass, typename SecondClass, typename Hasher, typename Equal, typename Allocator>
struct SerializeTraits<std::unordered_map<FirstClass, SecondClass, Hasher, Equal, Allocator>>
    : public SerializeTraitsBase<std::unordered_map<FirstClass, SecondClass, Hasher, Equal, Allocator>> {

    typedef std::unordered_map<FirstClass, SecondClass, Hasher, Equal, Allocator> value_type;

    constexpr static const char* GetTypeString() { return "map"; }
    constexpr static bool MightContainIDPtr() {
        return SerializeTraits<FirstClass>::MightContainIDPtr() ||
               SerializeTraits<SecondClass>::MightContainIDPtr();
    }
    template<typename Coder>
    inline static void Transfer (value_type& data, Coder& transfer) {
        transfer.transferSTLStyleMap(data);
    }
};

template<typename T>
struct NonConstContainerValueType {
    typedef typename T::value_type value_type;
};

template<typename T>
struct NonConstContainerValueType<std::set<T>> {
    typedef T value_type;
};

template<class T0, class T1, class Compare, class Allocator>
struct NonConstContainerValueType<std::map<T0, T1, Compare, Allocator>> {
    typedef std::pair<T0, T1> value_type;
};

template<typename FirstClass, typename SecondClass, typename Hasher, typename Equal, typename Allocator>
struct NonConstContainerValueType<std::unordered_map<FirstClass, SecondClass, Hasher, Equal, Allocator>> {
    typedef std::pair<FirstClass, SecondClass> value_type;
};

#define DEFINE_Primitive_TYPE_TRAITS(x)		\
	constexpr static const char* GetTypeString() { return #x; } \
    constexpr static bool MightContainIDPtr() { return false; }


template<typename T>
requires std::is_enum_v<T>
struct SerializeTraits<T> : public SerializeTraits<std::underlying_type_t<T>> {

    typedef T value_type;

    template<typename Coder>
    inline static void Transfer(value_type &data, Coder& transfer) {
        transfer.transferPrimitiveData(reinterpret_cast<std::underlying_type_t<T> &>(data));
    }
};

template<>
struct SerializeTraits<float> : public SerializeTraitsBaseForPrimitiveType<float> {
    DEFINE_Primitive_TYPE_TRAITS(float)
};

template<>
struct SerializeTraits<double> : public SerializeTraitsBaseForPrimitiveType<double> {
    DEFINE_Primitive_TYPE_TRAITS(double)
};

template<>
struct SerializeTraits<Int32> : public SerializeTraitsBaseForPrimitiveType<Int32> {
    DEFINE_Primitive_TYPE_TRAITS(Int32)
};

template<>
struct SerializeTraits<UInt32> : public SerializeTraitsBaseForPrimitiveType<UInt32> {
    DEFINE_Primitive_TYPE_TRAITS(<UInt32)
};

template<>
struct SerializeTraits<Int64> : public SerializeTraitsBaseForPrimitiveType<Int64> {
    DEFINE_Primitive_TYPE_TRAITS(Int64)
};

template<>
struct SerializeTraits<UInt64> : public SerializeTraitsBaseForPrimitiveType<UInt64> {
    DEFINE_Primitive_TYPE_TRAITS(UInt64)
};

template<>
struct SerializeTraits<UInt8> : public SerializeTraitsBaseForPrimitiveType<UInt8> {
    DEFINE_Primitive_TYPE_TRAITS(UInt8)
};

template<>
struct SerializeTraits<Int8> : public SerializeTraitsBaseForPrimitiveType<Int8> {
    DEFINE_Primitive_TYPE_TRAITS(Int8)
};

template<>
struct SerializeTraits<UInt16> : public SerializeTraitsBaseForPrimitiveType<UInt16> {
    DEFINE_Primitive_TYPE_TRAITS(UInt16)
};

template<>
struct SerializeTraits<Int16> : public SerializeTraitsBaseForPrimitiveType<Int16> {
    DEFINE_Primitive_TYPE_TRAITS(Int16)
};

template<>
struct SerializeTraits<bool> : public SerializeTraitsBase<bool> {
    typedef bool value_type;

    inline static const char* GetTypeString() { return "bool"; }

    template<typename Coder>
    inline static void Transfer (value_type& data, Coder& transfer) {
        ANAssert(sizeof(bool) == 1);
        UInt8& temp = reinterpret_cast<UInt8&>(data);
        transfer.transferPrimitiveData(temp);

        // You constructor or Reset function is not setting the bool value to a defined value!
        ANAssert((transfer.IsDecoding() || transfer.IsEncoding()) && (temp == 0 || temp == 1));
    }
};

}

#endif//OJOIE_SERIALIZETRAITS_HPP
