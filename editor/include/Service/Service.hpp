/******************************************************************************/
/*!
\file   Service.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the Service class, which
provides a base interface for services in editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SERVICE_HPP
#define SERVICE_HPP

class Service
{
public:
	enum struct ServiceState
	{
		STARTING,
		RUNNING,
		ENDING,
		STOPPED
	};
	
protected:
	
	ServiceState state = ServiceState::STOPPED;
public:
	virtual void start() = 0;
	virtual void run() = 0;
	virtual void end() = 0;

	virtual ~Service() = default;

	ServiceState getState() const { return state; }
};
#endif // SERVICE_HPP