/*!************************************************************************
\file:		Messaging_System.h
\author:	Saminathan Aaron Nicholas
\email:		s.aaronnicholas@digipen.edu
\course:	CSD 3401 - Software Engineering Project 5
\brief:		This file contains the Messaging System Class used to manage messages.
			It declares methods used to publish message, add and remove subscribers.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*************************************************************************/
#pragma once
#include "Subscriber.h"
#include "Message.h"

/**
 * @brief A class that manages the communication between different systems via messages.
 *
 * The MessagingSystem handles publishing messages, subscribing to specific message types, and notifying subscribers of new messages.
 * It also maintains a message queue and processes the queue in each frame update.
 */
class MessagingSystem
{
	private:
		std::deque<std::unique_ptr<Message>> messageQueue;
		SubscriberList subList;

		public:
		/**
		* @brief Subscribes to messages of a specific type with two callbacks.
		*
		* @param[in] messageID The ID of the message type to subscribe to.
		* @param[in] eagerCallBack The callback function that is triggered immediately when the message is published.
		* @param[in] normalCallBack The callback function that is triggered when the message is processed during the update.
		*/
		void Subscribe(MessageID messageID, const MessageCallback& eagerCallBack, const MessageCallback& normalCallBack);

		/**
		* @brief Unsubscribes a subscriber from receiving messages.
		*
		* @param subPointer[in] A unique pointer to the subscriber to remove.
		*/
		void Unsubscribe(std::unique_ptr<Subscriber> subPointer);

		/**
		* @brief Overload for unsubscribing a subscriber from receiving messages.
		*
		* @param[in] messageID The ID of the message type used to identify the subscriber.
		* @param[in] eagerCallBack The callback function that is triggered immediately used to identify the subscriber.
		* @param[in] normalCallBack The callback function that is triggered when the message is processed during the update used to identify the subscriber.
		*/
		void Unsubscribe(MessageID messageID, const MessageCallback& eagerCallBack, const MessageCallback& normalCallBack);

		/**
		* @brief Publishes a message to the system.
		*
		* The message is added to the message queue and all eager subscribers are immediately notified.
		*
		* @param[in] messageID The ID of the message type being published.
		* @param[in] message A unique pointer to the message being published.
		*/
		void Publish(MessageID messageID, std::unique_ptr<Message> message);

		/**
		* @brief Empties the Subscriber List.
		*
		* This function removes the Subscribers that are in the subscriber list.
		* This function acts as both cleanup and init.
		*/
		void Init();


		/**
		* @brief Processes the message queue and notifies subscribers.
		*
		* This function processes all messages in the queue, triggering the normal callbacks for all subscribers.
		* After processing, the queue is cleared.
		*/
		void Update(); 
};

extern MessagingSystem messagingSystem;