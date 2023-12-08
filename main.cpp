#include <windows.h>
#include <magnification.h>
#include <fstream>
#include <iostream>
#include <winuser.h>

int main()
{
	// Initialize Magnification API
	if (!MagInitialize())
	{
		std::cerr << "Failed to initialize Magnification API" << std::endl;
		return 1;
	}

	// Define a 5x5 matrix
	MAGCOLOREFFECT colorEffect;

	// Load the matrix from a text file
	std::ifstream file("matrix.txt");
	if (!file)
	{
		std::cerr << "Failed to open matrix file" << std::endl;
		return 1;
	}

	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < 5; ++j)
		{
			file >> colorEffect.transform[i][j];
		}
	}

	// Set the color effect
	BOOL success = MagSetFullscreenColorEffect(&colorEffect);

	// Check if the color effect was set successfully
	if (success)
	{
		std::cout << "Color effect set successfully" << std::endl;
	}
	else
	{
		DWORD errorCode = GetLastError();
		LPSTR errorMessage = nullptr;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&errorMessage),
			0,
			nullptr);

		std::cerr << "Failed to set color effect. Error code: " << errorCode << std::endl;
		std::cerr << "Error message: " << errorMessage << std::endl;

		LocalFree(errorMessage);
		return 1;
	}

	// Wait for the user to press Enter
	std::cout << "Press Enter to reset the color effect" << std::endl;
	std::cin.get();

	// Cleanup
	MagUninitialize();
}
