/*!************************************************************************
\file:      InputManager.cpp
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3451 - Software Engineering Project 6
\brief:     This file implements the class, which provides a wrapper for input handling in the application.
            The class manages input events like key presses, mouse movements, scroll wheel activity, and typed text.
            It captures input using GLFW callbacks and publishes input events to the messaging system for further processing.

            Methods are also offered to query key and mouse states, fetching mouse positions, and retrieving scroll offsets.
            It also resets input states at the end of each frame and handles input state persistence between frames.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/
#include <Core/Window.h>
#include "./Input/InputManager.h"
#include "./Messaging/Message.h"
#include "./Messaging/Messaging_System.h"
#include "./Messaging/Subscriber.h"
#include "Engine.hpp"
#include "Profiler/profiler.hpp"

InputManager* InputManager::instance = nullptr;
const float DEAD_ZONE = 0.25f; //  Adjustable

void InputManager::Setup_Callbacks()
{
    if (!instance)
    {
        instance = new InputManager();
    }

    glfwSetKeyCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::Key_Callback);
    glfwSetCharCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::Char_Callback);
    glfwSetMouseButtonCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::Mouse_Callback);
    glfwSetCursorPosCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::CursorPosition_Callback);
    glfwSetScrollCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::Scroll_Callback);
    glfwSetFramebufferSizeCallback(Engine::GetWindowInstance().GetNativeWindow(), InputManager::FramebufferSize_Callback);
}

void InputManager::Key_Callback(GLFWwindow*, int key, int , int action, int)
{
    if (instance)
    {
        std::unique_ptr<InputMessage> keyMessage = std::make_unique<InputMessage>();

        if (action == GLFW_PRESS)
        {
            instance->keyPressedStates[key] = true;
            instance->keyReleasedStates[key] = false;
            instance->Test_KeyPressed(key);
            keyMessage->keyPressed.push_back(instance->Key_ToString(key)[0]);
        }
        else if (action == GLFW_RELEASE)
        {
            instance->keyPressedStates[key] = false;
            instance->keyReleasedStates[key] = true;
            instance->Test_KeyReleased(key);
            keyMessage->keyReleased.push_back(instance->Key_ToString(key)[0]);
        }

         if (!keyMessage->keyPressed.empty() || !keyMessage->keyReleased.empty())
         {
             messagingSystem.Publish(INPUT_KEY, std::move(keyMessage));
         }
    }
}

void InputManager::Char_Callback(GLFWwindow*, unsigned int codepoint)
{
    if (instance)
    {
        instance->typedText += static_cast<char>(codepoint); // Append the character to typedText
    }
}

void InputManager::Mouse_Callback(GLFWwindow*, int button, int action, int )
{
    std::string buttonName;
    if (instance)
    {
        std::unique_ptr<InputMessage> mouseMessage = std::make_unique<InputMessage>();

        if (action == GLFW_PRESS)
        {
            instance->mousePressedStates[button] = true;
            instance->mouseReleasedStates[button] = false;
            instance->Test_MousePressed(button);
            mouseMessage->mousePressed.push_back(instance->Mouse_ToString(button));
        }
        else if (action == GLFW_RELEASE)
        {
            instance->mousePressedStates[button] = false;
            instance->mouseReleasedStates[button] = true;
            instance->Test_MouseReleased(button);
            mouseMessage->mouseReleased.push_back(instance->Mouse_ToString(button));
        }

         if (!mouseMessage->mousePressed.empty() || !mouseMessage->mouseReleased.empty())
         {
             messagingSystem.Publish(INPUT_MOUSE_, std::move(mouseMessage));
         }
    }
}

void InputManager::CursorPosition_Callback(GLFWwindow* , double xpos, double ypos)
{
    if (instance)
    {
        instance->mouseXPosition = xpos;
        instance->mouseYPosition = ypos;
    }
}

void InputManager::Scroll_Callback(GLFWwindow* , double xoffset, double yoffset)
{
    if (instance)
    {
        instance->scrollXOffset = xoffset;
        instance->scrollYOffset = yoffset;
    }

}

void InputManager::FramebufferSize_Callback(GLFWwindow* , int widthFb, int heightFb)
{
    GLHelper::width = widthFb;
    GLHelper::height = heightFb;
}

void InputManager::Error_Callback(int , char const* description)
{
    std::cerr << "GLFW error: " << description << std::endl;
}

void InputManager::Update()
{
    PF_SYSTEM("Input System");
    typedText.clear();
    scrollXOffset = 0.0;
    scrollYOffset = 0.0;

    mouseConsumed = false;

    //for (auto& keyState : keyReleasedStates)
    //    keyState.second = false;

    for (auto& mouseState : mouseReleasedStates)
        mouseState.second = false;

    keyPressedStatesLastFrame = keyPressedStates;
}

void InputManager::GamePadUpdate()
{
    Poll_GamepadInput();

    for (auto& buttonState : gamepadReleasedStates)
    {
        buttonState.second = false;
    }
}

std::string InputManager::Key_ToString(int key) const
{
    // Create a mapping from key codes to characters
    static std::unordered_map<int, std::string> keyMap =
    {
        // Alphabet keys
        {GLFW_KEY_A, "A"}, {GLFW_KEY_B, "B"}, {GLFW_KEY_C, "C"},
        {GLFW_KEY_D, "D"}, {GLFW_KEY_E, "E"}, {GLFW_KEY_F, "F"},
        {GLFW_KEY_G, "G"}, {GLFW_KEY_H, "H"}, {GLFW_KEY_I, "I"},
        {GLFW_KEY_J, "J"}, {GLFW_KEY_K, "K"}, {GLFW_KEY_L, "L"},
        {GLFW_KEY_M, "M"}, {GLFW_KEY_N, "N"}, {GLFW_KEY_O, "O"},
        {GLFW_KEY_P, "P"}, {GLFW_KEY_Q, "Q"}, {GLFW_KEY_R, "R"},
        {GLFW_KEY_S, "S"}, {GLFW_KEY_T, "T"}, {GLFW_KEY_U, "U"},
        {GLFW_KEY_V, "V"}, {GLFW_KEY_W, "W"}, {GLFW_KEY_X, "X"},
        {GLFW_KEY_Y, "Y"}, {GLFW_KEY_Z, "Z"},

        // Number keys
        {GLFW_KEY_0, "0"}, {GLFW_KEY_1, "1"}, {GLFW_KEY_2, "2"},
        {GLFW_KEY_3, "3"}, {GLFW_KEY_4, "4"}, {GLFW_KEY_5, "5"},
        {GLFW_KEY_6, "6"}, {GLFW_KEY_7, "7"}, {GLFW_KEY_8, "8"},
        {GLFW_KEY_9, "9"},

        // Function keys
        {GLFW_KEY_F1, "F1"}, {GLFW_KEY_F2, "F2"}, {GLFW_KEY_F3, "F3"},
        {GLFW_KEY_F4, "F4"}, {GLFW_KEY_F5, "F5"}, {GLFW_KEY_F6, "F6"},
        {GLFW_KEY_F7, "F7"}, {GLFW_KEY_F8, "F8"}, {GLFW_KEY_F9, "F9"},
        {GLFW_KEY_F10, "F10"}, {GLFW_KEY_F11, "F11"}, {GLFW_KEY_F12, "F12"},

        // Special keys
        {GLFW_KEY_BACKSPACE, "BACKSPACE"}, {GLFW_KEY_SPACE, "SPACE"},
        {GLFW_KEY_LEFT_SHIFT, "LEFT SHIFT"}, {GLFW_KEY_RIGHT_SHIFT, "RIGHT SHIFT"},
        {GLFW_KEY_TAB, "TAB"}, {GLFW_KEY_CAPS_LOCK, "CAPS LOCK"},
        {GLFW_KEY_LEFT_CONTROL, "LEFT CONTROL"}, {GLFW_KEY_RIGHT_CONTROL, "RIGHT CONTROL"},
        {GLFW_KEY_ENTER, "ENTER"}
    };

    auto it = keyMap.find(key);
    return (it != keyMap.end()) ? it->second : "?";
}

std::string InputManager::Mouse_ToString(int button) const
{
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
            return "Left Mouse Button";
        case GLFW_MOUSE_BUTTON_RIGHT:
            return "Right Mouse Button";
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return "Middle Mouse Button";
        default:
            return "Unknown Mouse Button";
    }
}

bool InputManager::Is_KeyPressed(int key) const
{    
    auto it = keyPressedStates.find(key);
    if (it != keyPressedStates.end() && it->second)
    {
        return true;
    }

    // Check if any gamepad button is mapped to this key and is pressed
    for (const auto& [gamepadButton, mappedKey] : gamepadButtonToKeyMap)
    {
        auto gamepadIt = gamepadReleasedStates.find(gamepadButton);
        if (mappedKey == key && gamepadIt != gamepadReleasedStates.end() && gamepadIt->second)
        {
            return true;
        }
    }

    return false;
}

bool InputManager::Is_KeyReleased(int key) const
{
    auto it = keyReleasedStates.find(key);
    if (it != keyReleasedStates.end() && it->second)
    {
        return true;
    }

    for (const auto& [gamepadButton, mappedKey] : gamepadButtonToKeyMap)
    {
        if (mappedKey == key && gamepadReleasedStates.at(gamepadButton))
        {
            return true;
        }
    }

    return false;
}

bool InputManager::Is_KeyTriggered(int key) const
{
    auto current = keyPressedStates.find(key);
    auto previous = keyPressedStatesLastFrame.find(key);

    bool curr = (current != keyPressedStates.end()) ? current->second : false;
    bool prev = (previous != keyPressedStatesLastFrame.end()) ? previous->second : false;

    return curr && !prev;
}


bool InputManager::Is_MousePressed(int button) const
{
    auto it = mousePressedStates.find(button);
    return it != mousePressedStates.end() && it->second;
}

bool InputManager::Is_MouseReleased(int button) const
{
    auto it = mouseReleasedStates.find(button);
    return it != mouseReleasedStates.end() && it->second;
}

void InputManager::Get_MousePosition(double& xpos, double& ypos) const
{
    xpos = mouseXPosition;
    ypos = mouseYPosition;
    #ifdef _DEBUG
        //std::cout << "Mouse cursor position: (" << xpos << ", " << ypos << ")" << std::endl;
    #endif
}

void InputManager::Get_MousePosition(float& xpos, float& ypos) const
{
    xpos = static_cast<float>(mouseXPosition);
    ypos = static_cast<float>(mouseYPosition);
    #ifdef _DEBUG
        //std::cout << "Mouse cursor position: (" << xpos << ", " << ypos << ")" << std::endl;
    #endif
}

void InputManager::Get_ScrollOffset(double& xoffset, double& yoffset) const
{
    xoffset = scrollXOffset;
    yoffset = scrollYOffset;
}

std::string InputManager::Get_TypedText() const
{
    return typedText;
}

void InputManager::Initialise_GamePad_Input()
{
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_A] = GLFW_KEY_S;
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_B] = GLFW_KEY_E;
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_X] = GLFW_KEY_F;
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_Y] = GLFW_KEY_SPACE;
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_START] = GLFW_KEY_ESCAPE;
    instance->gamepadButtonToKeyMap[GLFW_GAMEPAD_BUTTON_BACK] = GLFW_KEY_R;

    instance->gamepadAxisToKeyMap[GLFW_GAMEPAD_AXIS_LEFT_X] = {  GLFW_KEY_A, GLFW_KEY_D };          // Left = A, Right = D
    instance->gamepadAxisToKeyMap[GLFW_GAMEPAD_AXIS_LEFT_Y] = {  GLFW_KEY_W, GLFW_KEY_H };          // Up = W, Down = S
    instance->gamepadAxisToKeyMap[GLFW_GAMEPAD_AXIS_RIGHT_X] = { GLFW_KEY_LEFT, GLFW_KEY_RIGHT };   // Left, Right 
    instance->gamepadAxisToKeyMap[GLFW_GAMEPAD_AXIS_RIGHT_Y] = { GLFW_KEY_UP, GLFW_KEY_DOWN };      // Up, Down  
}

void InputManager::Poll_GamepadInput()
{
    if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) return;

    GLFWgamepadstate state;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) return;

    std::unique_ptr<InputMessage> keyMessage = std::make_unique<InputMessage>();

    for (const auto& [gamepadButton, mappedKey] : gamepadButtonToKeyMap)
    {
        if (state.buttons[gamepadButton] == GLFW_PRESS)
        {
            if (!gamepadPressedStates[gamepadButton])
            {
                gamepadPressedStates[gamepadButton] = true;
                gamepadReleasedStates[gamepadButton] = false;

                //  Only update if the key is NOT already pressed by the keyboard
                if (!instance->keyPressedStates[mappedKey])
                {
                    instance->keyPressedStates[mappedKey] = true;
                    instance->keyReleasedStates[mappedKey] = false;
                    instance->Test_KeyPressed(mappedKey);
                    keyMessage->keyPressed.push_back(instance->Key_ToString(mappedKey)[0]);
                }
            }
        }
        else
        {
            if (gamepadPressedStates[gamepadButton])
            {
                gamepadPressedStates[gamepadButton] = false;
                gamepadReleasedStates[gamepadButton] = true;

                //  Only update if the key is NOT being held down by the keyboard
                if (!glfwGetKey(GLHelper::ptr_window, mappedKey))
                {
                    instance->keyPressedStates[mappedKey] = false;
                    instance->keyReleasedStates[mappedKey] = true;
                    instance->Test_KeyReleased(mappedKey);
                    keyMessage->keyReleased.push_back(instance->Key_ToString(mappedKey)[0]);
                }
            }
        }
    }

    // Send key messages if any key events occurred
     if (!keyMessage->keyPressed.empty() || !keyMessage->keyReleased.empty())
     {
         messagingSystem.Publish(INPUT_KEY, std::move(keyMessage));
     }

    // Handle gamepad axis movement
    std::unique_ptr<InputMessage> gamepadMessage = std::make_unique<InputMessage>();

    for (const auto& [gamepadAxis, mappedKey] : gamepadAxisToKeyMap)
    {
        float axisValue = state.axes[gamepadAxis];

        //  Ignore tiny movements
        if (fabs(axisValue) < DEAD_ZONE)
        {
            axisValue = 0.0f;
        }

        //  Ensure joystick input doesn't override keyboard input
        bool isNegativeKeyPressed = glfwGetKey(GLHelper::ptr_window, mappedKey.negativeKey) == GLFW_PRESS;
        bool isPositiveKeyPressed = glfwGetKey(GLHelper::ptr_window, mappedKey.positiveKey) == GLFW_PRESS;

        if (gamepadAxis == GLFW_GAMEPAD_AXIS_LEFT_X)
        {
            if (axisValue < -0.5f) // Left movement
            {
                if (!isNegativeKeyPressed) // Only move if keyboard is NOT pressing A
                {
                    if (!instance->keyPressedStates[GLFW_KEY_A])
                    {
                        instance->keyPressedStates[GLFW_KEY_A] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_A)[0]);
                        std::cout << "Gamepad moving left\n";
                    }
                }
            }
            else if (axisValue > 0.5f) // Right movement
            {
                if (!isPositiveKeyPressed) // Only move if keyboard is NOT pressing D
                {
                    if (!instance->keyPressedStates[GLFW_KEY_D])
                    {
                        instance->keyPressedStates[GLFW_KEY_D] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_D)[0]);
                        std::cout << "Gamepad moving right\n";
                    }
                }
            }
            else // Stick is centered, reset states
            {
                if (!isNegativeKeyPressed && !isPositiveKeyPressed)
                {
                    instance->keyPressedStates[GLFW_KEY_A] = false;
                    instance->keyPressedStates[GLFW_KEY_D] = false;
                    gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_A)[0]);
                    gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_D)[0]);
                }
            }
        }

        if (gamepadAxis == GLFW_GAMEPAD_AXIS_LEFT_Y)
        {
            if (axisValue < -0.5f) // Up movement
            {
                if (!isNegativeKeyPressed) // Only move if keyboard is NOT pressing W
                {
                    if (!instance->keyPressedStates[GLFW_KEY_W])
                    {
                        instance->keyPressedStates[GLFW_KEY_W] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_W)[0]);
                        std::cout << "Gamepad moving up\n";
                    }
                }
            }
            else if (axisValue > 0.5f) // Down movement
            {
                if (!isPositiveKeyPressed) // Only move if keyboard is NOT pressing S
                {
                    if (!instance->keyPressedStates[GLFW_KEY_H])
                    {
                        instance->keyPressedStates[GLFW_KEY_H] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_H)[0]);
                        std::cout << "Gamepad moving down\n";
                    }
                }
            }
            else // Stick is centered, reset states
            {
                instance->keyPressedStates[GLFW_KEY_W] = false;
                instance->keyPressedStates[GLFW_KEY_H] = false;
                gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_W)[0]);
                gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_H)[0]);
            }
        }

        if (gamepadAxis == GLFW_GAMEPAD_AXIS_RIGHT_X)
        {
            if (axisValue < -0.5f) // Left movement
            {
                if (!isNegativeKeyPressed) // Only move if keyboard is NOT pressing A
                {
                    if (!instance->keyPressedStates[GLFW_KEY_LEFT])
                    {
                        instance->keyPressedStates[GLFW_KEY_LEFT] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_LEFT)[0]);
                        std::cout << "Gamepad moving left\n";
                    }
                }
            }
            else if (axisValue > 0.5f) // Right movement
            {
                if (!isPositiveKeyPressed) // Only move if keyboard is NOT pressing D
                {
                    if (!instance->keyPressedStates[GLFW_KEY_RIGHT])
                    {
                        instance->keyPressedStates[GLFW_KEY_RIGHT] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_RIGHT)[0]);
                        std::cout << "Gamepad moving right\n";
                    }
                }
            }
            else // Stick is centered, reset states
            {
                if (!isNegativeKeyPressed && !isPositiveKeyPressed)
                {
                    instance->keyPressedStates[GLFW_KEY_LEFT] = false;
                    instance->keyPressedStates[GLFW_KEY_RIGHT] = false;
                    gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_LEFT)[0]);
                    gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_RIGHT)[0]);
                }
            }
        }

        if (gamepadAxis == GLFW_GAMEPAD_AXIS_RIGHT_Y)
        {
            if (axisValue < -0.5f) // Up movement
            {
                if (!isNegativeKeyPressed) // Only move if keyboard is NOT pressing W
                {
                    if (!instance->keyPressedStates[GLFW_KEY_UP])
                    {
                        instance->keyPressedStates[GLFW_KEY_UP] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_UP)[0]);
                        std::cout << "Gamepad moving up\n";
                    }
                }
            }
            else if (axisValue > 0.5f) // Down movement
            {
                if (!isPositiveKeyPressed) // Only move if keyboard is NOT pressing S
                {
                    if (!instance->keyPressedStates[GLFW_KEY_DOWN])
                    {
                        instance->keyPressedStates[GLFW_KEY_DOWN] = true;
                        gamepadMessage->keyPressed.push_back(Key_ToString(GLFW_KEY_DOWN)[0]);
                        std::cout << "Gamepad moving down\n";
                    }
                }
            }
            else // Stick is centered, reset states
            {
                instance->keyPressedStates[GLFW_KEY_UP] = false;
                instance->keyPressedStates[GLFW_KEY_DOWN] = false;
                gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_UP)[0]);
                gamepadMessage->keyReleased.push_back(Key_ToString(GLFW_KEY_DOWN)[0]);
            }
        }
    }

    // Send axis movement messages if any occurred
    if (!gamepadMessage->keyPressed.empty() || !gamepadMessage->keyReleased.empty())
    {
        messagingSystem.Publish(INPUT_KEY, std::move(gamepadMessage));
    }
}

void InputManager::SetInputContext(InputContext context)
{
    currentContext = context;
}

InputManager::InputContext InputManager::GetInputContext() const
{
    return currentContext;
}

bool InputManager::IsGameplayInputEnabled() const
{
    return currentContext == InputContext::Gameplay;
}

void InputManager::Consume_Mouse()
{
    mouseConsumed = true;
}

bool InputManager::Is_MouseConsumed() const
{
    return mouseConsumed;
}
