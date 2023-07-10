//
// Created by aojoie on 4/1/2023.
//

#ifndef OJOIE_OBJECTDEFINES_HPP
#define OJOIE_OBJECTDEFINES_HPP

#include <ojoie/Utility/Assert.h>
#include <ojoie/Template/Constructor.hpp>
#include <ojoie/Serialize/SerializeDefines.h>
#include <ojoie/Serialize/Coder/YamlDecoder.hpp>
#include <ojoie/Serialize/Coder/YamlEncoder.hpp>

// Every non-abstract class that is derived from object has to place this inside the class Declaration
// (REGISTER_DERIVED_CLASS (Foo, Object))
#define DECLARE_DERIVED_AN_CLASS(x, d) \
public: \
	static int GetClassIDStatic ();       \
    static AN::Class *GetClassStatic(); \
	constexpr static const char* GetClassNameStatic(){ return #x; }			\
	static bool IsAbstract ()						{ return false; }\
	typedef d Super; \
	static void LoadClass (); \
protected: \
    virtual ~x() override; \
	public:


// Every abstract class that is derived from object has to place this inside the class Declaration
// (REGISTER_DERIVED_ABSTRACT_CLASS (Foo, Object))
#define	DECLARE_DERIVED_ABSTRACT_AN_CLASS(x, d) \
public: \
	static int GetClassIDStatic();                 \
    static AN::Class *GetClassStatic();\
	static bool IsAbstract ()						{ return true; }\
	constexpr static const char* GetClassNameStatic() { return #x; }				\
	typedef d Super; \
	static void LoadClass (); \
protected: \
	virtual ~x() override; \
	public:


#define IMPLEMENT_AN_CLASS_FULL(x, INIT, DEALLOC) \
extern "C" int __an_newClassId_Internal(void); \
int x::GetClassIDStatic() {   \
    int superId [[maybe_unused]] = Super::GetClassIDStatic(); \
    static int id = __an_newClassId_Internal(); \
    return id;\
}                             \
AN::Class *x::GetClassStatic() {     \
    return AN::Class::GetClass(x::GetClassIDStatic());\
}\
void x::LoadClass() { \
	ANAssert(!Super::IsSealedClass ());           \
	if (AN::Class::GetClass(Super::GetClassIDStatic()) == nullptr)	\
		Super::LoadClass();     \
    if (AN::Class::GetClass(x::GetClassIDStatic()) != nullptr) return; \
    AN::Class x##Class(x::GetClassIDStatic(), Super::GetClassIDStatic(), \
                       AN::Constructor<x>::ConstructStatic<AN::ObjectCreationMode>, \
                       [](void *obj) { \
                            ((x *)obj)->~x(); \
                       },     \
                       #x,    \
                       sizeof(x),              \
                       IsAbstract(),           \
                       INIT,     \
                       DEALLOC);\
	AN::Class::LoadClass(x##Class); \
}

#define IMPLEMENT_AN_CLASS(x) IMPLEMENT_AN_CLASS_FULL(x, nullptr, nullptr)
#define IMPLEMENT_AN_CLASS_HAS_INIT(x) IMPLEMENT_AN_CLASS_FULL(x, x::InitializeClass, x::DeallocClass)
#define IMPLEMENT_AN_CLASS_HAS_INIT_ONLY(x) IMPLEMENT_AN_CLASS_FULL(x, x::InitializeClass, nullptr)

#define LOAD_AN_CLASS(x) \
    struct x##_class_loader { \
        x##_class_loader() {  \
            x::LoadClass();\
        }\
    } g##x##_class_loader;

#define DECLARE_OBJECT_SERIALIZE(x) \
	constexpr static const char* GetTypeString()	 { return GetClassNameStatic(); } \
    constexpr static bool MightContainIDPtr() { return true; } \
	template<typename _Coder> void transfer(_Coder &coder); \
	virtual void redirectTransferVirtual(AN::YamlEncoder& coder) override; \
	virtual void redirectTransferVirtual(AN::YamlDecoder& coder) override;

#define IMPLEMENT_OBJECT_SERIALIZE(x)	\
void x::redirectTransferVirtual(AN::YamlEncoder& coder) { coder.transfer(*this, GetTypeString()); }	\
void x::redirectTransferVirtual(AN::YamlDecoder& coder) { coder.transfer(*this, GetTypeString()); }	\

#endif//OJOIE_OBJECTDEFINES_HPP
