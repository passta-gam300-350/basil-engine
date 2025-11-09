/*!************************************************************************
\file:		Message.h
\author:	Saminathan Aaron Nicholas
\email:		s.aaronnicholas@digipen.edu
\course:	CSD 3401 - Software Engineering Project 5
\brief:		This file contains the base Message class and derived Messages.
			It also contains the Enumerator MessageID used to identify message types.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*************************************************************************/
#pragma once
// #include "ECS/Components.h"

#include <string>
#include <memory>
#include <vector>

/**
 * @brief Enumerates the types of messages that can be sent in the system.
 *
 * This enum helps identify the type of message.
 */
enum MessageID
{
	INPUT_KEY,
	INPUT_MOUSE_,
	COLLISION_ON_COLLISION,
	COLLISION_ON_TRIGGER_ENTER,
	COLLISION_ON_TRIGGER_EXIT,
	LOGIC_ON_CLICK_PRESSED,
	LOGIC_ON_CLICK_RELEASED,
	CAMERA_CALCULATION_UPDATE
};

/**
 * @brief The base class for all message types used in the messaging system.
 *
 * All specific message types inherit from this class.
 * Messages are used to communicate between systems.
 */
class Message
{
	protected:
		MessageID messageID{};
	public:
		/**
		* @brief Retrieves the message's ID.
		*
		* This function provides a fast way to get the MessageID without additional overhead.
		*/
		inline MessageID Get_Message_ID() const { return messageID; }						// Getter without the overhead
		inline void Set_Message_ID(MessageID msgID) { this->messageID = msgID; }			// Setter without the overhead

		/**
		* @brief Virtual clone function for duplicating messages while using unique_ptrs.
		*
		* This function provides a way to clone unique_ptrs.
		*/
		virtual std::unique_ptr<Message> clone() const = 0;

		/**
		* @brief Virtual destructor for the Message class.
		*
		* This ensures that derived message types are properly destroyed.
		*/
		virtual ~Message() = default;
};

/**
 * @brief A message representing input events.
 *
 * This structure contains data about keys pressed, keys released and mouse button events.
 */
struct InputMessage : Message
{
	std::vector<char> keyPressed;
	std::vector<char> keyReleased;
	std::vector<std::string> mousePressed;
	std::vector<std::string> mouseReleased;

	public:
		/**
		* @brief Clone function for duplicating input message.
		*
		* This function provides a way to clone unique_ptrs.
		*/
		std::unique_ptr<Message> clone() const override
		{
			return std::make_unique<InputMessage>(*this);
		}
};

// /**
//  * @struct CollisionMessage
//  * @brief A message representing collision events between objects.
//  *
//  * This structure contains data about a component involved in the collision.
//  */
// struct CollisionMessage : Message
// {
// 	std::array<char, 12> componentUUID;
// 	std::string collidedObjectName;

// 	public:
// 		/**
// 		* @brief Clone function for duplicating collision message
// 		*
// 		* This function provides a way to clone unique_ptrs.
// 		*/
// 		std::unique_ptr<Message> clone() const override
// 		{
// 			return std::make_unique<CollisionMessage>(*this);
// 		}
// };

// /**
//  * @struct ComponentMessage
//  * @brief A message containing a Component's UUID
//  *
//  * This structure contains data about a component that can be used to pass 1 single gameobject
//  */
// struct ComponentMessage : Message
// {
// 	std::array<char, 12> componentUUID{};

// 	public:
// 		/**
// 		* @brief Clone function for duplicating collision message
// 		*
// 		* This function provides a way to clone unique_ptrs.
// 		*/
// 		std::unique_ptr<Message> clone() const override
// 		{
// 			return std::make_unique<ComponentMessage>(*this);
// 		}
// };