//
// Created by aojoie on 5/15/2023.
//

#ifndef OJOIE_INPUTCONTROL_HPP
#define OJOIE_INPUTCONTROL_HPP

#include <ojoie/Configuration/typedef.h>
#include <ojoie/Core/Exception.hpp>
#include <ojoie/Math/Math.hpp>

namespace AN {

enum InputControlType {
    kInputControlButton,
    kInputControlAxis, // float
    kInputControlVector2
};

enum InputStateFormat {
    kInputStateFormatInvalid = 0,
    kInputStateFormatFloat,
    kInputStateFormatByte,
    kInputStateFormatBit,
    kInputStateFormatVector2
};

struct InputStateBlockInfo {
    InputStateFormat format;
    UInt32           byteOffset;
    UInt32           bitOffset;

    InputStateBlockInfo() : format(), byteOffset(), bitOffset() {}
};

class IInputDevice;

class AN_API IInputControl {
protected:

    IInputDevice *_device;
    InputStateBlockInfo _stateBlockInfo;

public:

    void initInternal(IInputDevice    *device,
                      InputStateFormat format,
                      UInt32           byteOffset,
                      UInt32           bitOffset) {
        _device = device;
        _stateBlockInfo.format = format;
        _stateBlockInfo.byteOffset = byteOffset;
        _stateBlockInfo.bitOffset = bitOffset;
    }

    virtual void finishSetup() {}

    void *getCurrentFrameStatePtr();
    void *getPreviousFrameStatePtr();
};

template<typename T>
class InputControl : public IInputControl {

public:

    T getValue() { return readUnprocessedValueFromState(getCurrentFrameStatePtr()); }
    T getPreviousFrameValue() { return readUnprocessedValueFromState(getPreviousFrameStatePtr()); }

    bool isChanged() { return getValue() != getPreviousFrameValue(); }

    virtual T readUnprocessedValueFromState(void* statePtr) = 0;

};

class AN_API AxisControl : public InputControl<float> {
public:

    virtual float readUnprocessedValueFromState(void* statePtr) override {
        switch (_stateBlockInfo.format) {
            case kInputStateFormatFloat:
                return *(float *)((UInt8 *)statePtr + _stateBlockInfo.byteOffset);
            case kInputStateFormatByte:
                return *(UInt8 *)((UInt8 *)statePtr + _stateBlockInfo.byteOffset) == 0 ? 0.f : 1.f;
            case kInputStateFormatBit:
                return (*((UInt8 *)statePtr + _stateBlockInfo.byteOffset) & (1 << _stateBlockInfo.bitOffset)) != 0 ? 1.f : 0.f;
            default:
                throw AN::Exception("Unsupported input state format");
        }
    }
};

class AN_API ButtonControl : public AxisControl {

    static bool isValueConsideredPressed(float value) {
        return value > 0.1f;
    }

public:

    bool isPress() { return isValueConsideredPressed(getValue()); }

    bool wasPressedThisFrame() { return isPress() && !isValueConsideredPressed(getPreviousFrameValue()); }

    bool wasReleasedThisFrame() { return !isPress() && isValueConsideredPressed(getPreviousFrameValue()); }

};

class AN_API Vector2Control : public InputControl<Vector2f> {
    AxisControl x;
    AxisControl y;
public:

    virtual void finishSetup() override {
        x.initInternal(_device, kInputStateFormatFloat, _stateBlockInfo.byteOffset, 0);
        y.initInternal(_device, kInputStateFormatFloat, _stateBlockInfo.byteOffset + sizeof(float), 0);
        x.finishSetup();
        y.finishSetup();
    }

    AxisControl &getX() { return x; }
    AxisControl &getY() { return y; }

    virtual Vector2f readUnprocessedValueFromState(void* statePtr) override {
        return Vector2f(x.getValue(), y.getValue());
    }
};


}

#endif//OJOIE_INPUTCONTROL_HPP
