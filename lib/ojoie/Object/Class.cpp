//
// Created by aojoie on 4/1/2023.
//

#include "Object/Class.hpp"
#include "Allocator/MemoryDefines.h"
#include "Object/Object.hpp"
#include "Template/Access.hpp"
#include "Core/Exception.hpp"
#include "Utility/Log.h"

#include <format>
#include <map>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>

namespace AN {

AN_API std::atomic_int gClassId = 0;

}

extern "C" AN_API int __an_newClassId_Internal(void) {
    return AN::gClassId++;
}

AN_API std::atomic_int gClassId = 0;

namespace AN {


namespace {

struct ObjectISATag : Access::TagBase<ObjectISATag> {};
struct ObjectInstanceIDTag : Access::TagBase<ObjectInstanceIDTag> {};

}// namespace

template struct Access::Accessor<ObjectISATag, &Object::isa>;
template struct Access::Accessor<ObjectInstanceIDTag, &Object::_instanceID>;

ANHashSet<Name> gDisabledMessageNames;

class ClassManager {

    /// use map instead of unordered_map because the Class pointer will remain valid after inserting more classes
    typedef std::map<int, Class>                          ClassMap;
    typedef std::unordered_map<std::string_view, Class *> ClassNameMap;

    ClassMap          classMap;
    ClassNameMap      classNameMap;
    std::shared_mutex sm;

public:
    ~ClassManager() {
        unloadAllClasses();
    }

    void loadClass(const Class &context) {
        int superId = context.getSuperClassId();
        {
            std::shared_lock lock(sm);
            if (classMap.contains(context.getClassId())) {
                ANLog("Class id %d name %s already registered",
                      context.getClassId(),
                      classMap.at(context.getClassId()).getClassName());
                return;
            }
            if (!classMap.contains(context.getSuperClassId())) {
                superId = context.getClassId();
            }
        }

        Class const cls(context.getClassId(), superId,
                        context.ctor, context.dtor, context.className, context.size,
                        context.bIsAbstract, context.initializeClass, context.deallocClass);

        {
            std::unique_lock lock(sm);
            auto             it = classMap.insert({context.classID, cls});
            if (classNameMap.contains(context.className)) {
                AN_LOG(Warning, "Class with the same name %s already exist, create which class is undefined at runtime",
                       it.first->second.getClassName());
            }
            classNameMap[it.first->second.className] = &it.first->second;
        }

        /// initialize class, initializeClass fun may call GetClass, make sure no dead lock
        if (cls.initializeClass) {
            cls.initializeClass();
        }
    }

    void unloadAllClasses() {
        std::unique_lock lock(sm);
        classNameMap.clear();
        for (auto &[id, cls] : classMap) {
            if (cls.deallocClass) {
                cls.deallocClass();
            }
        }
        classMap.clear();
    }

    Class *getClass(int classId) {
        std::shared_lock lock(sm);
        if (auto it = classMap.find(classId); it != classMap.end()) {
            return &it->second;
        }
        return nullptr;
    }

    Class *getClass(std::string_view name) {
        std::shared_lock lock(sm);
        if (auto it = classNameMap.find(name); it != classNameMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void debugPrintClassList() {
        for (auto &[id, cls] : classMap) {
            puts(cls.debugDescription().c_str());
        }
    }

    std::vector<Class *> findAllSubClasses(int id) {
        std::vector<Class *> result;
        for (auto &[name, cls] : classMap) {
            if (cls.isDerivedFrom(id)) {
                result.push_back(&cls);
            }
        }
        return result;
    }
};

ClassManager &GetClassManager() {
    static ClassManager classManager;
    return classManager;
}

extern "C" void __an_debugPrintClassList_internal(void) {
    GetClassManager().debugPrintClassList();
}

Class *Class::GetClass(int id) {
    return GetClassManager().getClass(id);
}

Class *Class::GetClass(std::string_view name) {
    return GetClassManager().getClass(name);
}

std::vector<Class *> Class::FindAllSubClasses(int id) {
    return GetClassManager().findAllSubClasses(id);
}

std::vector<Class *> Class::FindAllSubClasses(std::string_view name) {
    return GetClassManager().findAllSubClasses(GetClassManager().getClass(name)->getClassId());
}

void Class::LoadClass(const Class &context) {
    GetClassManager().loadClass(context);
}

Class *Class::getSuperClass() const {
    return GetClassManager().getClass(superClassId);
}

bool Class::isDerivedFrom(int derivedFromClassID) const {
    Class *superClass = getSuperClass();
    for (;;) {

        if (superClass->classID == derivedFromClassID) {
            return true;
        }

        if (superClass->classID == Object::GetClassIDStatic()) {
            break;
        }

        superClass = superClass->getSuperClass();
    }

    return false;
}

Object *Class::createInstance(ObjectCreationMode mode) {

    if (bIsAbstract) {
        ANLog("Cannot create abstract class %s", className.c_str());
        return nullptr;
    }

    Object *object = (Object *) AN_CALLOC(1, size);

    /// set up isa pointer
    Access::set<ObjectISATag>(*object, this);

    /// call c++ constructor
    ctor(object, mode);

    return object;
}

void Class::destroyInstance(Object *obj) {
    if (obj == nullptr) { return; }

    /// call dealloc first
    obj->deallocInternal();

    /// call c++ destructor
    obj->getClass()->dtor(obj);
    AN_FREE(obj);
}

void Class::DestroyInstance(Object *obj) {
    if (obj) {
        obj->getClass()->destroyInstance(obj);
    }
}

std::string Class::debugDescription() {
    return std::format("id: {}, name: {}, super class id: {} name: {}, is abstract: {}",
                       classID, className, getSuperClass()->getClassId(), getSuperClass()->className, isAbstract());
}

MessageCallback Class::respondToMessageInternal(MessageName name) {

    if (gDisabledMessageNames.contains(name)) { return nullptr; }

    if (auto it = _cachedMessages.find(name);
        it != _cachedMessages.end()) {
        if (it->second == nullptr) {
            return nullptr;
        }
        return it->second;
    }

    if (auto it = _supportedMessages.find(name);
        it != _supportedMessages.end()) {
        _cachedMessages[name] = it->second;
        return it->second;
    }

    if (getClassId() == Object::GetClassIDStatic()) {
        /// not found
        return nullptr;
    }

    /// look up super
    /// and assign cache
    MessageCallback callback = getSuperClass()->respondToMessageInternal(name);
    if (callback)
    {
        _cachedMessages[name] = callback;
    }
    return callback;
}

MessageCallback Class::sendMessageInternal(void *receiver, Message &message) {

    if (gDisabledMessageNames.contains(message.name)) { return nullptr; }

    // look up cache first
    if (auto it = _cachedMessages.find(message.name);
        it != _cachedMessages.end()) {
        if (it->second) {
            it->second(receiver, message);
        }
        return it->second;
    }

    if (auto it = _supportedMessages.find(message.name);
            it != _supportedMessages.end()) {
        it->second(receiver, message);
        /// add cache
        _cachedMessages[message.name] = it->second;
        return it->second;
    }

    if (getClassId() == Object::GetClassIDStatic()) {
        /// no method found, this is not an error
#if AN_DEBUG
        AN_LOG(Error, "%s", std::format("unknown message send to object {}", receiver).c_str());
#endif
        return nullptr;
    }

    /// send to super class
    MessageCallback callback = getSuperClass()->sendMessageInternal(receiver, message);
    /// add to cache
    if (callback)
    {
        _cachedMessages[message.name] = callback;
    }
    return callback;
}

void Class::SetMessageEnabled(MessageName name, bool enable) {
    if (enable) {
        auto it = gDisabledMessageNames.find(name);
        if (it != gDisabledMessageNames.end()) {
            gDisabledMessageNames.erase(it);
        }
    } else {
        gDisabledMessageNames.insert(name);
    }
}

}// namespace AN