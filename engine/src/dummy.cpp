#include <iostream>	
#include "mono/metadata/assembly.h"
int main() {
	std::cout << "Hello, World!\n";
	MonoAssembly* assembly = mono_assembly_open("dummy.dll", NULL);

	return 0;

}