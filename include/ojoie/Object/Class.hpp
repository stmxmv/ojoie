//
// Created by aojoie on 4/1/2023.
//

#ifndef OJOIE_CLASS_HPP
#define OJOIE_CLASS_HPP


#include <ojoie/Configuration/typedef.h>
#include <ojoie/Template/RC.hpp>
#include <ojoie/Utility/Log.h>
#include <ojoie/Core/Name.hpp>

#include <string>
#include <unordered_map>
#include <atomic>

namespace AN {


typedef Name MessageName;

struct Message {
    void       *sender;
    MessageName name;
    intptr_t    data;

    template<typename T>
    T getData() {
        return *reinterpret_cast<T *>(&data);
    }
};

typedef void (*MessageCallback)(void *receiver, Message &message);

typedef ANHashMap<MessageName, MessageCallback> MessageMap;

class Object;

enum ObjectCreationMode {
    kCreateObjectDefault = 0,
    kCreateObjectFromNonMainThread = 1,
    kCreateObjectDefaultNoLock = 2
};


class AN_API Class {
public:
    typedef void Ctor(void *, ObjectCreationMode mode);

    typedef void Dtor(void *);

private:
    int classID;// the class ID of the class
    int superClassId;
    Ctor *ctor;
    Dtor *dtor;
    std::string className;// the name of the class
    int size;             // sizeof (Class)
    bool bIsAbstract;     // is the class Abstract?

    void (*initializeClass)();
    void (*deallocClass)();

    MessageMap _supportedMessages; // message callback of this class
    MessageMap _cachedMessages; // may contain super class message callback

    friend class ClassManager;

public:
    Class(int classId, int superClassId,
          Ctor *ctor, Dtor *dtor,
          std::string_view className,
          int size, bool bIsAbstract,
          void (*initializeClass)(),
          void (*deallocClass)())
        : classID(classId), superClassId(superClassId),
          ctor(ctor), dtor(dtor),
          className(className), size(size),
          bIsAbstract(bIsAbstract),
          initializeClass(initializeClass), deallocClass(deallocClass) {}

    Class *getSuperClass() const;

    Ctor *getFactoryFunction() const { return ctor; }

    int getSuperClassId() const { return superClassId; }
    int getClassId() const { return classID; }

    const char *getClassName() const { return className.c_str(); }

    int getSize() const { return size; }

    bool isAbstract() const { return bIsAbstract; }

    template<typename Obj>
    bool isDerivedFrom() const { return isDerivedFrom(Obj::GetClassIDStatic()); }

    bool isDerivedFrom(int derivedFromClassID) const;

    std::string debugDescription();

    Object *createInstance(ObjectCreationMode mode);

    void destroyInstance(Object *obj);

    bool respondToMessage(MessageName name) { return respondToMessageInternal(name) != nullptr; }

    void sendMessage(void *receiver, Message &message) { sendMessageInternal(receiver, message); }

    static void SetMessageEnabled(MessageName name, bool enable);

    MessageCallback respondToMessageInternal(MessageName name);

    MessageCallback sendMessageInternal(void *receiver, Message &message);

    /// will replace callback if exist
    void registerMessageCallback(MessageName name, MessageCallback callback) {
        _supportedMessages[name] = callback;
    }

    template<typename Obj>
    static Obj *CreateInstance(ObjectCreationMode mode) {
        Class *cls = GetClass<Obj>();
        if (cls == nullptr) { return nullptr; }
        return (Obj *) cls->createInstance(mode);
    }

    static Object *CreateInstance(ObjectCreationMode mode, std::string_view name) {
        Class *cls = GetClass(name);
        if (cls == nullptr) { return nullptr; }
        return cls->createInstance(mode);
    }

    static void DestroyInstance(Object *obj);

    template<typename T>
    static Class *GetClass() {
        return GetClass(T::GetClassIDStatic());
    }

    static Class *GetClass(int id);

    static Class *GetClass(std::string_view name);

    static std::vector<Class *> FindAllSubClasses(int id);

    template<typename T>
    static std::vector<Class *> FindAllSubClasses() {
        return FindAllSubClasses(T::GetClassIDStatic());
    }

    static std::vector<Class *> FindAllSubClasses(std::string_view name);

    /// \brief load aClass into the internal class DB
    ///        note that it will not remain the reference to aClass,
    ///        implementation will copy aClass's data to the internal
    ///        managed class
    ///        to get the registered class, use GetClass method
    static void LoadClass(const Class &aClass);
};

template<typename Obj>
inline Obj *NewObject() {
    return Class::CreateInstance<Obj>(kCreateObjectDefault);
}

inline Object *NewObject(std::string_view name) {
    return Class::CreateInstance(kCreateObjectDefault, name);
}

inline Object *NewObject(int id) {
    Class *cls = Class::GetClass(id);
    if (!cls) {
        ANLog("Class id %d not exist", id);
        return nullptr;
    }
    return cls->createInstance(kCreateObjectDefault);
}

template<typename Obj>
inline void DestroyObject(Obj *&obj) {
    if (obj) {
        Class::DestroyInstance((Object *) obj);
        obj = nullptr;
    }
}

template<typename Obj>
inline void DestroyObject(Obj * &&obj) {
    if (obj) {
        Class::DestroyInstance((Object *) obj);
    }
}

template<typename T>
inline int ClassID() {
    return T::GetClassIDStatic();
}

struct ObjectDeleter {
    void operator()(Object *obj) const {
        DestroyObject(obj);
    }
};

}// namespace AN

#endif//OJOIE_CLASS_HPP
