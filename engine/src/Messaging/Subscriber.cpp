/*!************************************************************************
\file:		Subscriber.cpp
\author:	Saminathan Aaron Nicholas
\email:		s.aaronnicholas@digipen.edu
\course:	CSD 3401 - Software Engineering Project 5
\brief:		This file contains definitions used for subscription functions.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*************************************************************************/
#include "./Messaging/Subscriber.h"

// Equality operator using function traits to determine if the callbacks match.
bool operator==(const MessageCallback& lhs, const MessageCallback& rhs)
{
	const MessageFunctionPtr* lhsFuncPtr = lhs.target<MessageFunctionPtr>();
	const MessageFunctionPtr* rhsFuncPtr = rhs.target<MessageFunctionPtr>();

	if (lhsFuncPtr && rhsFuncPtr) // Both have valid targets
	{
		return *lhsFuncPtr == *rhsFuncPtr;
	}
	else if (!lhsFuncPtr && !rhsFuncPtr)
	{
		return true; // Both are empty
	}
	else
	{
		return false; // One is valid
	}
}

bool Subscriber::operator==(const Subscriber& other) const
{
	return (this->messageID == other.messageID) && (this->callbackEager == other.callbackEager) && (this->callbackNormal == other.callbackNormal);
}

std::vector<Subscriber*> SubscriberList::Call_Subscribers(MessageID messageID)
{
	std::vector<Subscriber*> newSubList{};
	for (int i{}; i < this->subscriberList.size(); ++i)
	{
		if (this->subscriberList[i]->Get_Message_ID() == messageID)
		{
			newSubList.push_back(this->subscriberList[i].get());
		}
	}
	return newSubList;
}

void SubscriberList::Add_Subscriber(std::unique_ptr<Subscriber> subPointer)
{
	this->subscriberList.push_back(std::move(subPointer));
}

void SubscriberList::Delete_Subscriber(std::unique_ptr<Subscriber> subPointer)
{
	this->subscriberList.erase(
		std::remove_if(this->subscriberList.begin(), this->subscriberList.end(),
			[&](const std::unique_ptr<Subscriber>& sub)
			{
				return *sub == *subPointer;
			}),
		this->subscriberList.end());
}

void SubscriberList::Empty_Subscribers()
{
	this->subscriberList.clear();
}