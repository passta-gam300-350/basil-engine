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
	// This loop employs FIFO
	while (messageQueue.size())
	{
		std::vector<Subscriber*> subscribers = this->subList.Call_Subscribers(messageQueue[0]->Get_Message_ID());
		for (size_t i{}; i < subscribers.size(); ++i)
		{
			if (subscribers[i]->callbackNormal != nullptr)
			{
				subscribers[i]->callbackNormal(std::move(messageQueue[0]->clone()));
			}
		}
		messageQueue.pop_front();
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