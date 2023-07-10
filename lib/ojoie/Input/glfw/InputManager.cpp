////
//// Created by Aleudillonam on 8/6/2022.
////
//
//
//namespace AN {
//
//InputKey glfwKeyToANInputKey(int glfwKey) {
//    switch (glfwKey) {
//        case GLFW_KEY_0:
//            return InputKey::_0;
//        case GLFW_KEY_1:
//            return InputKey::_1;
//        case GLFW_KEY_2:
//            return InputKey::_2;
//        case GLFW_KEY_3:
//            return InputKey::_3;
//        case GLFW_KEY_4:
//            return InputKey::_4;
//        case GLFW_KEY_5:
//            return InputKey::_5;
//        case GLFW_KEY_6:
//            return InputKey::_6;
//        case GLFW_KEY_7:
//            return InputKey::_7;
//        case GLFW_KEY_8:
//            return InputKey::_8;
//        case GLFW_KEY_9:
//            return InputKey::_9;
//        case GLFW_KEY_A:
//            return InputKey::A;
//        case GLFW_KEY_B:
//            return InputKey::B;
//        case GLFW_KEY_C:
//            return InputKey::C;
//        case GLFW_KEY_D:
//            return InputKey::D;
//        case GLFW_KEY_E:
//            return InputKey::E;
//        case GLFW_KEY_F:
//            return InputKey::F;
//        case GLFW_KEY_G:
//            return InputKey::G;
//        case GLFW_KEY_H:
//            return InputKey::H;
//        case GLFW_KEY_I:
//            return InputKey::I;
//        case GLFW_KEY_J:
//            return InputKey::J;
//        case GLFW_KEY_K:
//            return InputKey::K;
//        case GLFW_KEY_L:
//            return InputKey::L;
//        case GLFW_KEY_M:
//            return InputKey::M;
//        case GLFW_KEY_N:
//            return InputKey::N;
//        case GLFW_KEY_O:
//            return InputKey::O;
//        case GLFW_KEY_P:
//            return InputKey::P;
//        case GLFW_KEY_Q:
//            return InputKey::Q;
//        case GLFW_KEY_R:
//            return InputKey::R;
//        case GLFW_KEY_S:
//            return InputKey::S;
//        case GLFW_KEY_T:
//            return InputKey::T;
//        case GLFW_KEY_U:
//            return InputKey::U;
//        case GLFW_KEY_V:
//            return InputKey::V;
//        case GLFW_KEY_W:
//            return InputKey::W;
//        case GLFW_KEY_X:
//            return InputKey::X;
//        case GLFW_KEY_Y:
//            return InputKey::Y;
//        case GLFW_KEY_Z:
//            return InputKey::Z;
//        case GLFW_KEY_ESCAPE:
//            return InputKey::Esc;
//        case GLFW_KEY_LEFT_SHIFT:
//            return InputKey::LeftShift;
//        case GLFW_KEY_LEFT_CONTROL:
//            return InputKey::LeftControl;
//        case GLFW_KEY_LEFT_ALT:
//            return InputKey::LeftAlt;
//        case GLFW_KEY_LEFT_SUPER:
//            return InputKey::LeftSuper;
//        case GLFW_KEY_RIGHT_SHIFT:
//            return InputKey::RightShift;
//        case GLFW_KEY_RIGHT_CONTROL:
//            return InputKey::RightControl;
//        case GLFW_KEY_RIGHT_ALT:
//            return InputKey::RightAlt;
//        case GLFW_KEY_RIGHT_SUPER:
//            return InputKey::RightSuper;
//        case GLFW_KEY_CAPS_LOCK:
//            return InputKey::Caps;
//        case GLFW_KEY_NUM_LOCK:
//            return InputKey::NumLock;
//        default:
//            return InputKey::None;
//    }
//}
//
//InputKey glfwMouseButtonToANInputKey(int glfwKey) {
//    switch (glfwKey) {
//        case GLFW_MOUSE_BUTTON_LEFT:
//            return InputKey::MouseLeftButton;
//        case GLFW_MOUSE_BUTTON_RIGHT:
//            return InputKey::MouseRightButton;
//        case GLFW_MOUSE_BUTTON_MIDDLE:
//            return InputKey::MouseMiddleButton;
//        default:
//            return InputKey::None;
//    }
//}
//
//InputEvent glfwKeyActionToANInputEvent(int action) {
//    switch (action) {
//        case GLFW_PRESS:
//            return InputEvent::Pressed;
//        case GLFW_RELEASE:
//            return InputEvent::Released;
//        case GLFW_REPEAT:
//            return InputEvent::Repeat;
//        default:
//            return InputEvent::None;
//    }
//}
//
//InputModifierFlags glfwModsToANInputModifierFlags(int mods) {
//    InputModifierFlags ret = InputModifierFlags::None;
//
//    if (mods & GLFW_MOD_SHIFT) {
//        ret |= InputModifierFlags::Shift;
//    }
//    if (mods & GLFW_MOD_CONTROL) {
//        ret |= InputModifierFlags::Control;
//    }
//    if (mods & GLFW_MOD_ALT) {
//        ret |= InputModifierFlags::Alt;
//    }
//    if (mods & GLFW_MOD_SUPER) {
//        ret |= InputModifierFlags::Super;
//    }
//    if (mods & GLFW_MOD_CAPS_LOCK) {
//        ret |= InputModifierFlags::Caps;
//    }
//    if (mods & GLFW_MOD_NUM_LOCK) {
//        ret |= InputModifierFlags::Caps;
//    }
//    return ret;
//}
//
//void restoreInput(GLFWwindow *window, PlatformImpl &platformImpl) {
//
//    glfwSetCursorPosCallback(window, platformImpl.preCursorPosFun);
//    glfwSetKeyCallback(window, platformImpl.preKeyFun);
//    glfwSetMouseButtonCallback(window, platformImpl.preMouseButtonFun);
//    glfwSetScrollCallback(window, platformImpl.preScrollFun);
//    glfwSetWindowFocusCallback(window, platformImpl.preWindowFocusFun);
//    glfwSetCursorEnterCallback(window, platformImpl.preCursorEnterFun);
//    glfwSetCharCallback(window, platformImpl.preCharFun);
//    glfwSetMonitorCallback(platformImpl.preMonitorFun);
//
//}
//
//void InputManager::setCurrentWindow(Window *window) {
//    if (currentWindow != nullptr) {
////        restoreInput((GLFWwindow *)currentWindow->getUnderlyingWindow(), impl->platformImpl);
//    }
//
//    currentWindow = window;
//
//    if (window == nullptr) {
//        return;
//    }
//
//    GLFWwindow *glfWwindow/* = (GLFWwindow *)window->getUnderlyingWindow()*/;
//
//    double mouseX, mouseY;
//    glfwGetCursorPos(glfWwindow, &mouseX, &mouseY);
//    impl->platformImpl.mouseX = (float)mouseX;
//    impl->platformImpl.mouseY = (float)mouseY;
//
//    impl->platformImpl.preCursorPosFun = glfwSetCursorPosCallback(glfWwindow, [](GLFWwindow* window, double xpos, double ypos) {
//        if (GetInputManager().impl->platformImpl.preCursorPosFun) {
//            GetInputManager().impl->platformImpl.preCursorPosFun(window, xpos, ypos);
//        }
//
//        PlatformImpl &platformImpl = GetInputManager().impl->platformImpl;
//
//        float xDelta = xpos - platformImpl.mouseX;
//        float yDelta = ypos - platformImpl.mouseY; // reversed since y-coordinates range from bottom to top
//
//        platformImpl.mouseX = (float)xpos;
//        platformImpl.mouseY = (float)ypos;
//
////        ANLog("xDelta: %f, yDelta: %f", xDelta, yDelta);
//
//        GetInputManager().mousePositionX.store((float)xpos, std::memory_order_relaxed);
//        GetInputManager().mousePositionY.store((float)ypos, std::memory_order_relaxed);
//        GetInputManager().impl->mouseQueue.enqueue({ xDelta, yDelta });
//    });
//
//    impl->platformImpl.preKeyFun = glfwSetKeyCallback(glfWwindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
//
//        if (GetInputManager().impl->platformImpl.preKeyFun) {
//            GetInputManager().impl->platformImpl.preKeyFun(window, key, scancode, action, mods);
//        }
//
//        GetInputManager().impl->keyEventQueue.enqueue({
//                glfwKeyToANInputKey(key),
//                glfwKeyActionToANInputEvent(action),
//                glfwModsToANInputModifierFlags(mods)
//        });
//
//    });
//
//    impl->platformImpl.preMouseButtonFun = glfwSetMouseButtonCallback(glfWwindow,
//                                                                      [](GLFWwindow* window, int button, int action, int mods) {
//        if (GetInputManager().impl->platformImpl.preMouseButtonFun) {
//            GetInputManager().impl->platformImpl.preMouseButtonFun(window, button, action, mods);
//        }
//
//        GetInputManager().impl->keyEventQueue.enqueue({
//                glfwMouseButtonToANInputKey(button),
//                glfwKeyActionToANInputEvent(action),
//                glfwModsToANInputModifierFlags(mods)
//        });
//
//    });
//}
//
//}