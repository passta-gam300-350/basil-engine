
/******************************************************************************/
/*!
\file   ManagedInput.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedInput class, which
is responsible for managing and getting various input-related functionalities in the
managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef MANAGED_INPUT_HPP
#define MANAGED_INPUT_HPP
class ManagedInput
{
public:
	static bool GetKey(int keycode); // returns true as long as the key is held down
	static bool GetKeyDown(int keycode); // returns true only on the frame the key was pressed
	static bool GetKeyUp(int keycode); // returns true only on the frame the key was released
	static bool GetKeyPress(int keycode); // returns true only on the frame the key was pressed
	static void GetMousePosition(float* xp, float* yp); // Mouse screen position

};


#endif // MANAGED_INPUT_HPP