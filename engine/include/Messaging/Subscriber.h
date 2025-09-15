/*!************************************************************************
\file:		Subscriber.h
\author:	Saminathan Aaron Nicholas
\email:		s.aaronnicholas@digipen.edu
\course:	CSD 3401 - Software Engineering Project 5
\brief:		This file contains the Subscriber / SubscriberList class used in the MessagingSystem Class.
			It contains a list of subscribers and ways to interface with them.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*************************************************************************/
#pragma once
#include "Message.h"
#include <iostream>
#include <functional>

using MessageCallback = std::function<void(std::unique_ptr<Message>)>;
using MessageFunctionPtr = void(*)(std::unique_ptr<Message>);

/**
 * @brief A class representing a subscriber to messages of a specific type.
 *
 * A Subscriber listens for messages of a specific type and defines two types
 * of callbacks: eager and normal. The eager callback is triggered immediately
 * when a message is published, while the normal callback is triggered during
 * the message system's update phase.
 */
class Subscriber
{
	MessageID messageID;
	public:
		MessageCallback callbackEager;
		MessageCallback callbackNormal;

		/**
		* @brief Returns the message ID that the subscriber is listening for.
		*
		* @return The message ID.
		*/
		inline MessageID Get_Message_ID() const { return messageID; } // Getter without the overhead

		/**
		* @brief Constructor for the Subscriber class.
		*
		* @param[in] msgID The ID of the message type.
		* @param[in] cbEager The eager callback function.
		* @param[in] cbNormal The normal callback function.
		*/
		Subscriber(MessageID msgID, MessageCallback cbEager, MessageCallback cbNormal) : messageID{ msgID }, callbackEager{ cbEager }, callbackNormal{ cbNormal } {}

		/**
		* @brief Equality operator for the Subscriber class.
		*
		* @param[in] other The subscriber to compare to.
		*/
		bool operator==(const Subscriber& other) const;
};

/**
 * @brief A class managing a list of subscribers.
 *
 * The SubscriberList class provides methods for adding, removing, and notifying subscribers.
 */
class SubscriberList
{
	private:
		std::vector<std::unique_ptr<Subscriber>> subscriberList;
	public:
		/**
		* @brief Returns all subscribers that are interested in a specific message ID.
		*
		* @param[in] messageID The ID of the message type.
		* @return A vector of pointers to the subscribers.
		*/
		std::vector<Subscriber*> Call_Subscribers(MessageID messageID);

		/**
		* @brief Adds a subscriber to the list.
		*
		* @param[in] subPointer A unique pointer to the subscriber to add.
		*/
		void Add_Subscriber(std::unique_ptr<Subscriber> subPointer);

		/**
		* @brief Removes a subscriber from the list.
		*
		* @param[in] subPointer A unique pointer to the subscriber to remove.
		*/
		void Delete_Subscriber(std::unique_ptr<Subscriber> subPointer);

		/**
		* @brief Empties the Subscriber List.
		*/
		void Empty_Subscribers();
};

