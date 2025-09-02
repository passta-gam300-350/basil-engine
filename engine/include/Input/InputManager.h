/*!************************************************************************
\file:      InputManager.h
\author:    Saminathan Aaron Nicholas
\email:     s.aaronnicholas@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file declares the functions used by the input manager class.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
**************************************************************************/
#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <string>

// #include "Graphics/Graphics_Helper.h"
// #include "MessagingSystem/Messaging_System.h"

// #include "imgui.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h"

/**
 * @brief Manages input events and states.
 *
 * The class handles keyboard, mouse and gamepad input, providing an interface for checking the states of various keys and buttons. 
 * 
 * NOTE: GLFWwindow* window parameter is included in the callback function because of the way GLFW is designed.
 * GLFW defines the signature for input callback functions, and this specific parameter is always passed in by GLFW even if not used.
 * scancode and mods are also unused currently but may be needed in future.
 */
class InputManager
{
    public:
        /**
        * @brief Gets a singleton instance.
        *
        * If the instance doesn't exist, it creates one.
        * This provides a global access point to the class, ensuring only 1 instance exists.
        */
        static InputManager* Get_Instance()
        {
            if (!instance)
            {
                instance = new InputManager();
            }
            return instance;
        }

        /**
        * @brief Destroys the singleton instance.
        *
        * Releases the memory of the singleton instance if it exists.
        */
        static void Destroy_Instance()
        {
            if (instance)
            {
                delete instance;
                instance = nullptr;
            }
        }

        /**
        * @brief Enables or disables debug mode.
        *
        * When debug mode is enabled, key and mouse events will be printed onto the console,
        * providing detailed information about user interactions for debugging purposes.
        *
        * @param[in] debug Boolean flag to true (enabled) or false (disabled) debug mode.
        */
        void Set_DebugMode(bool debug)
        {
            debugMode = debug;
        }

        /**
        * @brief Tests if a key is currently pressed.
        *
        * This function checks if a specified key is currently pressed.
        * If debug mode is enabled, it will print a message onto the console.
        *
        * @param[in] key The GLFW key code to test for a press event.
        */
        void Test_KeyPressed(int key) const
        {
            if (debugMode)
            {
                if (Is_KeyPressed(key))
                {
                    std::cout << "Key " << Key_ToString(key) << " pressed." << std::endl;
                }
            }
        }

        /**
        * @brief Tests if a key has been released.
        *
        * This function checks whether the specified key has been released.
        * If debug mode is enabled, it will print a message to the console.
        *
        * @param[in] key The GLFW key code to test for a release event.
        */
        void Test_KeyReleased(int key) const
        {
            if (debugMode)
            {
                if (Is_KeyReleased(key))
                {
                    std::cout << "Key " << Key_ToString(key) << " released." << std::endl;
                }
            }
        }

        /**
        * @brief Tests if a mouse button is currently pressed.
        *
        * This function checks whether the specified mouse button is currently pressed.
        * If debug mode is enabled, it will print a message to the console indicating which button is pressed.
        *
        * @param[in] button The GLFW button code to test for a press event.
        */
        void Test_MousePressed(int button) const
        {
            std::string buttonName;
            if (debugMode)
            {
                switch (button)
                {
                    case GLFW_MOUSE_BUTTON_LEFT:
                        buttonName = "Left mouse button";
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        buttonName = "Right mouse button";
                        break;
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        buttonName = "Middle mouse button";
                        break;
                    default:
                        buttonName = "Unknown button";
                        break;
                }
                if (Is_MousePressed(button))
                {
                    std::cout << buttonName << " pressed." << std::endl;
                }
            }
        }

        /**
        * @brief Tests if a mouse button has been released.
        *
        * This function checks whether the specified mouse button has been released.
        * If debug mode is enabled, it will print a message to the console indicating which button was released.
        *
        * @param[in] button The GLFW button code to test for a release event.
        */
        void Test_MouseReleased(int button) const
        {
            std::string buttonName;
            if (debugMode)
            {
                switch (button)
                {
                    case GLFW_MOUSE_BUTTON_LEFT:
                        buttonName = "Left mouse button";
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        buttonName = "Right mouse button";
                        break;
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        buttonName = "Middle mouse button";
                        break;
                    default:
                        buttonName = "Unknown button";
                        break;
                }
                if (Is_MouseReleased(button))
                {
                    std::cout << buttonName << " released." << std::endl;
                }
            }
        }

        /**
        * @brief Sets up input callbacks for GLFW.
        *
        * This function registers the necessary input callbacks with GLFW, such as key, character, mouse, cursor position, scroll, and framebuffer size events.
        * It is essential for capturing and responding to user input in the application.
        */
        static void Setup_Callbacks();

        /**
        * @brief Handles key press and release events.
        *
        * This callback function is triggered when a key event occurs, such as when a key is pressed or released.
        * It updates the internal key state and invokes any debug outputs if debug mode is enabled.
        *
        * @param[in] window The GLFW window receiving the key event.
        * @param[in] key The GLFW key code that was pressed or released.
        * @param[in] scancode The system-specific scan code of the key.
        * @param[in] action GLFW_PRESS / GLFW_RELEASE / GLFW_REPEAT.
        * @param[in] mods Bit field describing which modifier keys were held down.
        */
        static void Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods);

        /**
        * @brief Handles character input events for text typing.
        *
        * This callback is triggered when the input is a character.
        * It appends the typed character to the internal text buffer used for managing text input.
        *
        * @param[in] window The GLFW window receiving the character event.
        * @param[in] codepoint The Unicode point of the character input.
        */
        static void Char_Callback(GLFWwindow* window, unsigned int codepoint);

        /**
        * @brief Handles mouse button press and release events.
        *
        * This callback function is triggered when a mouse button is pressed or released.
        * It updates the internal mouse button state and invokes any debug outputs if debug mode is enabled.
        *
        * @param[in] window The GLFW window receiving the mouse event.
        * @param[in] button The GLFW button code that was pressed or released.
        * @param[in] action GLFW_PRESS / GLFW_RELEASE.
        * @param[in] mods Bit field describing which modifier keys were held down.
        */
        static void Mouse_Callback(GLFWwindow* window, int button, int action, int mods);

        /**
        * @brief Handles mouse cursor movement events.
        *
        * This callback function is triggered when the mouse cursor is moved within the GLFW window.
        * It updates the internal cursor position state.
        *
        * @param[in] window The GLFW window receiving the cursor position event.
        * @param[in] xpos The new x-coordinate of the cursor.
        * @param[in] ypos The new y-coordinate of the cursor.
        */
        static void CursorPosition_Callback(GLFWwindow* window, double xpos, double ypos);

        /**
        * @brief Handles mouse scroll events.
        *
        * This callback function is triggered when the mouse scroll wheel is used.
        * It updates the internal scroll offset state.
        *
        * @param[in] window The GLFW window receiving the scroll event.
        * @param[in] xoffset The scroll offset along the x-axis.
        * @param[in] yoffset The scroll offset along the y-axis.
        */
        static void Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset);

        /**
        * @brief Handles framebuffer resize events.
        *
        * This callback function is triggered when the framebuffer is resized.
        * It updates the viewport dimensions to match the new framebuffer size.
        *
        * @param[in] window The GLFW window whose framebuffer was resized.
        * @param[in] widthFb The new width of the framebuffer in pixels.
        * @param[in] heightFb The new height of the framebuffer in pixels.
        */
        static void FramebufferSize_Callback(GLFWwindow* window, int widthFb, int heightFb);

        /**
        * @brief Handles GLFW error events.
        *
        * This callback function is triggered when an error occurs in GLFW.
        * It prints the error code and description to the console.
        *
        * @param[in] error The error code.
        * @param[in] description A human-readable description of the error.
        */
        static void Error_Callback(int error, char const* description);

        /**
        * @brief Updates the state of the input manager.
        *
        * This function processes any events and updates the internal state, including key and mouse button states, cursor position, scroll offsets, and typed text.
        * It should be called every frame to ensure input is up-to-date.
        */
        void Update();

        /**
        * @brief Converts a key code to a string representation.
        *
        * This function returns a string that represents the name of the specified key based on its GLFW key code.
        * It can be used to display key names.
        *
        * @param[in] key The GLFW key code to convert to a string.
        */
        std::string Key_ToString(int key) const;

        /**
        * @brief Converts a mouse button code to a string representation.
        *
        * This function returns a string that represents the name of the specified mouse button based on its GLFW button code.
        * It can be used to display button names.
        *
        * @param[in] button The GLFW button code to convert to a string.
        */
        std::string Mouse_ToString(int button) const;

        /**
        * @brief Checks if a specific key is currently pressed.
        *
        * This function returns true if the specified key is currently being pressed, otherwise false.
        * It can be used to detect real-time input for key presses.
        *
        * @param[in] key The GLFW key code to check.
        */
        bool Is_KeyPressed(int key) const;

        /**
        * @brief Checks if a specific key has been released.
        *
        * This function returns true if the specified key has just been released.
        * It is useful for detecting key release events and one-time key actions.
        *
        * @param[in] key The GLFW key code to check.
        */
        bool Is_KeyReleased(int key) const;

        /**
        * @brief Checks if a specific mouse button is currently pressed.
        *
        * This function returns true if the specified mouse button is currently being pressed, otherwise false.
        * It can be used to detect real-time input for mouse button presses.
        *
        * @param[in] button The GLFW button code to check.
        */
        bool Is_MousePressed(int button) const;

        /**
        * @brief Checks if a specific mouse button has been released.
        *
        * This function returns true if the specified mouse button has just been released.
        * It is useful for detecting mouse button release events and one-time actions.
        *
        * @param[in] button The GLFW button code to check.
        */
        bool Is_MouseReleased(int button) const;

        /**
        * @brief Retrieves the current mouse cursor position.
        *
        * This function retrieves the current position of the mouse cursor in the window.
        * The x and y coordinates are returned by reference in screen space, where the origin is at the top-left corner of the window.
        *
        * @param[out] xpos A reference to a double representing the x-coordinate.
        * @param[out] ypos A reference to a double representing the y-coordinate.
        */
        void Get_MousePosition(double& xpos, double& ypos) const;

        /**
        * @brief Retrieves the current mouse cursor position.
        *
        * This function retrieves the current position of the mouse cursor in the window.
        * The x and y coordinates are returned by reference in screen space, where the origin is at the top-left corner of the window.
        *
        * @param[out] xpos A reference to a float representing the x-coordinate.
        * @param[out] ypos A reference to a float representing the y-coordinate.
        */
        void Get_MousePosition(float& xpos, float& ypos) const;

        /**
        * @brief Retrieves the current scroll offset.
        *
        * This function retrieves the amount of scroll input, typically from the mouse scroll wheel.
        * The x and y offset values are returned by reference, where positive values indicate scrolling up or right,
        * and negative values indicate scrolling down or left.
        *
        * @param[out] xoffset A reference to a double representing the x-axis scroll offset.
        * @param[out] yoffset A reference to a double representing the y-axis scroll offset.
        */
        void Get_ScrollOffset(double& xoffset, double& yoffset) const;

        /**
        * @brief Retrieves the text typed by the user.
        *
        * This function returns a string containing the characters typed by the user since the last update.
        * It can be used for text input handling, such as typing into UI elements or text boxes.
        */
        std::string Get_TypedText() const;

        /**
        * Helper functions to support the usage of a gamepad
        */
        void Poll_GamepadInput();
        void Initialise_GamePad_Input();
        void GamePadUpdate();
    private:
        InputManager() : mouseButton(false), mouseXPosition(0.0), mouseYPosition(0.0), scrollXOffset(0.0), scrollYOffset(0.0), typedText(""), debugMode(false) {}
        std::unordered_map<int, bool> keyPressedStates;
        std::unordered_map<int, bool> keyReleasedStates;
        std::unordered_map<int, bool> mousePressedStates;
        std::unordered_map<int, bool> mouseReleasedStates;
        GLboolean mouseButton;
        double mouseXPosition;
        double mouseYPosition;
        double scrollXOffset;
        double scrollYOffset;
        std::string typedText;

        // For gamepad usage
        struct AxisMapping
        {
            int negativeKey;  // Key for negative axis movement (e.g., A for left)
            int positiveKey;  // Key for positive axis movement (e.g., D for right)
        };

        std::unordered_map<int, int> gamepadButtonToKeyMap; // Maps gamepad buttons to keyboard keys
        std::unordered_map<int, AxisMapping> gamepadAxisToKeyMap;
        std::unordered_map<int, bool> gamepadPressedStates; // Tracks button states
        std::unordered_map<int, bool> gamepadReleasedStates; // Tracks released states

        // Static variables for callbacks
        static InputManager* instance;
        bool debugMode = false;
};

#endif
