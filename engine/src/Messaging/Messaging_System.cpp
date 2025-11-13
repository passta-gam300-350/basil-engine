/*!************************************************************************
\file:		Messaging_System.cpp
\author:	Saminathan Aaron Nicholas
\email:		s.aaronnicholas@digipen.edu
\course:	CSD 3401 - Software Engineering Project 5
\brief:		This file contains the Messaging System Class used to manage messages.
			It defines methods used to publish message, add and remove subscribers.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*************************************************************************/
#include "./Messaging/Messaging_System.h"

void MessagingSystem::Init()
{
	this->subList.Empty_Subscribers();
}

void MessagingSystem::Update()
{
	// Swap messageQueue with local copy to prevent re-entrant modification during callbacks
	std::deque<std::unique_ptr<Message>> localQueue;
	localQueue.swap(messageQueue);

	// Process messages from local copy (FIFO)
	// New messages published during callbacks will go to the now-empty messageQueue
	while (!localQueue.empty())
	{
		auto& currentMessage = localQueue.front();

		// Get subscribers for this message
		std::vector<Subscriber*> subscribers = this->subList.Call_Subscribers(currentMessage->Get_Message_ID());

		// Invoke callbacks
		for (size_t i = 0; i < subscribers.size(); ++i)
		{
			if (subscribers[i]->callbackNormal != nullptr)
			{
				subscribers[i]->callbackNormal(std::move(currentMessage->clone()));
			}
		}

		localQueue.pop_front();
	}
}

void MessagingSystem::Subscribe(MessageID messageID, const MessageCallback& eagerCallBack, const MessageCallback& normalCallBack)
{
	this->subList.Add_Subscriber(std::make_unique<Subscriber>(messageID, eagerCallBack, normalCallBack));
}

void MessagingSystem::Unsubscribe(std::unique_ptr<Subscriber> subPointer)
{
	this->subList.Delete_Subscriber(std::move(subPointer));
}

void MessagingSystem::Unsubscribe(MessageID messageID, const MessageCallback& eagerCallBack, const MessageCallback& normalCallBack)
{
	this->subList.Delete_Subscriber(std::make_unique<Subscriber>(messageID, eagerCallBack, normalCallBack));
}

void MessagingSystem::Publish(MessageID messageID, std::unique_ptr<Message> message)
{
	message->Set_Message_ID(messageID);

	std::vector<Subscriber*> eagerSubs = this->subList.Call_Subscribers(messageID);
	for (size_t i{}; i < eagerSubs.size(); ++i)
	{
		if (eagerSubs[i]->callbackEager != nullptr)
		{
			eagerSubs[i]->callbackEager(std::move((message->clone())));
		}
	}

	this->messageQueue.push_back(std::move(message));
}

MessagingSystem messagingSystem;