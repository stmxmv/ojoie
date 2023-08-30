//
// Created by aojoie on 3/31/2023.
//

#ifndef OJOIE_OBJECT_HPP
#define OJOIE_OBJECT_HPP

#include <ojoie/Utility/Assert.h>
#include <ojoie/Object/Class.hpp>
#include <ojoie/Object/ObjectDefines.hpp>

namespace AN {

#ifdef _MSC_VER
#pragma warning(disable : 4291) // ignore no matching delete operator warning
#endif

class AN_API Object {
    class Class *isa;
    int _instanceID;
    bool _initCalled:1;
    intptr_t m_ScriptHandle;

protected:

    virtual ~Object() {}

public:

    explicit Object(ObjectCreationMode mode);

    /// Returns the classID of the class
    static int GetClassIDStatic();

    static Class *GetClassStatic();

    constexpr static const char* GetClassNameStatic() { return "Object"; }

    static bool IsSealedClass() { return false; }

    /// Returns true if the class is abstract
    static bool IsAbstract() { return true; }

    /// \brief load the class into the AN runtime
    ///        override this method to perform you own load class logic
    static void LoadClass();

    /// \brief called when the class is loaded
    static void InitializeClass();

    /// \brief called when the class is unloaded
    static void DeallocClass();

    static void DestroyAllObjects();

    static std::vector<Object *> FindObjectsOfType(int type);

    template<typename T>
    static std::vector<T *> FindObjectsOfType() {
        std::vector<Object *> results = FindObjectsOfType(T::GetClassIDStatic());
        return *reinterpret_cast<std::vector<T *> *>(&results);
    }

    template<typename T>
    static T *FindObjectOfType() {
        std::vector<Object *> results = FindObjectsOfType(T::GetClassIDStatic());
        return (T *)results.front();
    }

    Class *getClass() const { return isa; }
    Class *getSuperClass() const { ANAssert(isa != nullptr); return isa->getSuperClass(); }
    const char *getClassName() const { ANAssert(isa != nullptr); return isa->getClassName(); }

    int getClassID() const { ANAssert(isa != nullptr); return isa->getClassId(); }

    int getInstanceID() const { return _instanceID; }

    /// return -1 if the super class not exist
    int getSuperClassID() const {
        ANAssert(isa != nullptr);
        Class *superClass = isa->getSuperClass();
        if (superClass) {
            return superClass->getClassId();
        }
        return -1;
    }

    template<typename T>
    bool isDerivedFrom() const { return isDerivedFrom(T::GetClassIDStatic()); }

    bool isDerivedFrom(int compareClassID) const {
        return isa->isDerivedFrom(compareClassID);
    }

    template<typename T>
    T *asUnsafe() {
        return (T *)this;
    }

    template<typename T>
    T *as() {
        if (T::GetClassIDStatic() == isa->getClassId() || isDerivedFrom<T>()) {
            return (T *)this;
        }
        return nullptr;
    }

    template<typename T>
    bool is() {
        return getClassID() == T::GetClassIDStatic();
    }

    template<typename T>
    bool isKindOf() {
        return is<T>() || isDerivedFrom<T>();
    }

    /// init can fail, but caller can still ignore this return value
    /// subclass need to properly handle fail case
    virtual bool init();

    /// init after decoding
    virtual bool initAfterDecode();

    /// dealloc is called only the object is inited
    virtual void dealloc();

    /// sending unknown message will not throw but will increase performance overhead
    /// override this method for good reason
    virtual void sendMessage(Message &message);

    virtual void sendMessageSuper(Message &message);

    /// override this method for good reason
    virtual bool respondToMessage(MessageName name);

    virtual bool respondToMessageSuper(MessageName name);

    /// Serialize
    DECLARE_SERIALIZE(Object)

    virtual void redirectTransferVirtual(AN::YamlEncoder& coder);
    virtual void redirectTransferVirtual(AN::YamlDecoder& coder);

    void deallocInternal();

    std::string debugDescription();

    intptr_t getScriptHandle() const { return m_ScriptHandle; }
    void setScriptHandle(intptr_t mScriptHandle) { m_ScriptHandle = mScriptHandle; }

    static void SetScriptHandleCleanupFunc(void (*cleanup)(intptr_t));
};



}

#endif//OJOIE_OBJECT_HPP
