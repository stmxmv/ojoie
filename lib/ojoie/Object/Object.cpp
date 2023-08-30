//
// Created by aojoie on 4/1/2023.
//

#include "Object/Object.hpp"
#include "Template/Constructor.hpp"

#include <format>
#include <shared_mutex>

namespace AN {


void (*s_ScriptHandleCleanup)(intptr_t) = nullptr;

extern "C" int __an_newClassId_Internal(void);

static std::atomic_int gInstanceID = -1;
static std::atomic_int gSerializedInstanceID = 1;

/// PersistentManager will handle this
void setNextSerializedInstanceIDInternal(int id) {
    gSerializedInstanceID = id;
}

typedef std::unordered_map<int, Object *> ObjectMap;

static ObjectMap gObjectMap;
static std::shared_mutex sm;

static void insertObjectInMap(Object *object) {
    std::unique_lock lock(sm);
    ANAssert(object->getInstanceID() != 0);
    ANAssert(gObjectMap.find(object->getInstanceID()) == gObjectMap.end());
    gObjectMap.insert({ object->getInstanceID(), object });
}

static void removeObjectInMap(Object *object) {
    std::unique_lock lock(sm);
    ANAssert(gObjectMap.find(object->getInstanceID()) != gObjectMap.end());
    gObjectMap.erase(object->getInstanceID());
}

static Object *getInstance(int instanceID) {
    std::shared_lock lock(sm);
    if (auto it = gObjectMap.find(instanceID); it != gObjectMap.end()) {
        return it->second;
    }
    return nullptr;
}

int Object::GetClassIDStatic() {
    static int id = __an_newClassId_Internal();
    return id;
}

Class *Object::GetClassStatic() {
    return Class::GetClass(GetClassIDStatic());
}

void Object::LoadClass() {
    Class objectClass(
            GetClassIDStatic(), GetClassIDStatic(),
            Constructor<Object>::ConstructStatic<ObjectCreationMode>,
            [](void *obj) {
                ((Object *) obj)->~Object();
            },
            "Object",
            sizeof(Object),
            false,
            InitializeClass,
            DeallocClass);
    Class::LoadClass(objectClass);
}

void Object::InitializeClass() {
    (void)0;
}

void Object::DeallocClass() {}

void Object::DestroyAllObjects() {
    for (;;) {
        Object *object = nullptr;
        {
            std::shared_lock lock(sm);

            if (gObjectMap.empty()) {
                break;
            }

            object = gObjectMap.begin()->second;
        }

        DestroyObject(object);
    }
}

/// constructor should not initialize isa and instanceID, since it is already inited by Class
Object::Object(ObjectCreationMode mode) : _initCalled(), m_ScriptHandle() {}

bool Object::init() {
    ANAssert(_initCalled == false);
    _initCalled = true;

    if (isa != nullptr) {
        /// assign instance id
        _instanceID = gInstanceID--;
        insertObjectInMap(this);
        return true;
    }
    return false;
}

bool Object::initAfterDecode() {
    ANAssert(_initCalled == false);
    _initCalled = true;

    if (isa != nullptr) {
        _instanceID = gSerializedInstanceID++;
        insertObjectInMap(this);
        return true;
    }
    return false;
}

void Object::dealloc() {
    ANAssert(_initCalled == true);

    if (m_ScriptHandle && s_ScriptHandleCleanup) {
        s_ScriptHandleCleanup(m_ScriptHandle);
        m_ScriptHandle = 0;
    }
}

void Object::deallocInternal() {
    if (!_initCalled) {
        ANLog("Object [%d:%s] allocate but not inited", isa->getClassId(), isa->getClassName());
    } else {
        removeObjectInMap(this);
        dealloc();
    }
}

void Object::sendMessage(Message &message) {
    getClass()->sendMessage(this, message);
}

void Object::sendMessageSuper(Message &message) {
    getSuperClass()->sendMessage(this, message);
}

bool Object::respondToMessage(MessageName name) {
    return getClass()->respondToMessage(name);
}

bool Object::respondToMessageSuper(MessageName name) {
    return getSuperClass()->respondToMessage(name);
}

std::vector<Object *> Object::FindObjectsOfType(int type) {
    std::vector<Object *> result;
    for (auto [id, obj] : gObjectMap) {
        if (obj->getClassID() == type || obj->getClass()->isDerivedFrom(type)) {
            result.push_back(obj);
        }
    }
    return result;
}

std::string Object::debugDescription() {
    return std::format("Object: 0x{} class {}", (void *)this, getClass()->debugDescription());
}

void Object::SetScriptHandleCleanupFunc(void (*cleanup)(intptr_t)) {
    s_ScriptHandleCleanup = cleanup;
}

template<typename _Coder>
void Object::transfer(_Coder &coder) {

    std::string className;
    if constexpr (_Coder::IsEncoding()) {
        ANAssert(_initCalled == true);
        /// when serialize object get a new serialized instance id as global unique
        removeObjectInMap(this);
        _instanceID = gSerializedInstanceID++;
        insertObjectInMap(this);

        className = getClassName();
    }

    coder.transfer(className, "classname");

    if constexpr (_Coder::IsDecoding()) {
        ANAssert(className == getClassName());
    }
}

IMPLEMENT_OBJECT_SERIALIZE(Object)
INSTANTIATE_TEMPLATE_TRANSFER(Object)

}// namespace AN