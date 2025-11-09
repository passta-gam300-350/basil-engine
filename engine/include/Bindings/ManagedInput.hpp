#ifndef MANAGED_INPUT_HPP
#define MANAGED_INPUT_HPP
class ManagedInput
{
public:
	static bool GetKey(int keycode); // returns true as long as the key is held down
	static bool GetKeyDown(int keycode); // returns true only on the frame the key was pressed
	static bool GetKeyUp(int keycode); // returns true only on the frame the key was released
	static bool GetKeyPress(int keycode); // returns true only on the frame the key was pressed

};


#endif // MANAGED_INPUT_HPP