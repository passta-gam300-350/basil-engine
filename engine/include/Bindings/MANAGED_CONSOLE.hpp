/******************************************************************************/
/*!
\file   MANAGED_CONSOLE.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedConsole class, which
is responsible for handling logging messages from managed code (C#).


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef MANAGED_CONSOLE_HPP
#define MANAGED_CONSOLE_HPP
#include <string>
#include <vector>
#include <mutex>
#include <span>
typedef struct _MonoString MonoString;
class ManagedConsole {
public:
	struct Message {
		enum class Type {
			Info,
			Warning,
			Error
		} type;

		std::string content;


	};

private:


	static std::vector<std::pair<int, Message>> s_Messages;
	static std::mutex s_MessagesMutex;




public:
	static void Initialize();
	static void Shutdown();

	

	

	static void LogInfo(MonoString* message);
	static void LogWarning(MonoString* message);
	static void LogError(MonoString* message);


	static std::vector<std::pair<int, Message>> TryGetMessages(unsigned filters=0x7);

};
#endif