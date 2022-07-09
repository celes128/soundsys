// Always include this header before the line #define WIN32_LEAN_AND_MEAN
#include "../soundsys/SoundSystem.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>

sound::SoundSystem		*g_soundSys;
LPCWSTR					kWindowClassName = L"SoundDemoClass";

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

Error	Initialize(IN HINSTANCE instance);
void	Shutdown(HINSTANCE instance);
void	Run();

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdline, int showCmd)
{
	auto err = Initialize(IN instance);
	if (!err) {
		Run();
	}
	
	Shutdown(instance);
	return 0;
}

Error MakeWindow(IN HINSTANCE instance, OUT HWND *window)
{
	assert(window != nullptr);

	WNDCLASSEXW winClass{ 0 };
	winClass.cbSize = sizeof(WNDCLASSEX);
	winClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	winClass.lpfnWndProc = WindowProcedure;
	winClass.hInstance = instance;
	winClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winClass.lpszClassName = kWindowClassName;

	if (!RegisterClassEx(&winClass)) {
		return ERROR_FAILURE;
	}

	*window = CreateWindowEx(
		0,
		kWindowClassName,
		L"Demo Sound System",
		WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_SIZEBOX | WS_OVERLAPPEDWINDOW,
		300, 400, 800, 600,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (!window) {
		return ERROR_FAILURE;
	}

	return ERROR_NONE;
}

Error Initialize(IN HINSTANCE instance)
{
	HWND	window;
	auto err = MakeWindow(IN instance, OUT &window);
	if (err) {
		return err;
	}

	err = sound::CreateSoundSystem(IN window, OUT &g_soundSys);
	if (err) {
		return ERROR_FAILURE;
	}

	return ERROR_NONE;
}

void Shutdown(HINSTANCE instance)
{
	sound::DestroySoundSystem(&g_soundSys);
	UnregisterClass(kWindowClassName, instance);
}

void Run()
{
	MSG message;
	while (1) {
		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	auto handled = true;

	switch (message) {
	case WM_DESTROY: {
		PostQuitMessage(0);
	}break;

	case WM_KEYDOWN: {
		if (wparam == VK_RETURN) {
			//g_soundSys->Play("ff3boss_raccourcie.bin");
			g_soundSys->Play("JtS Stage 3.bin");
		}
	}break;

	default: {
		handled = false;
	}break;
	}
	
	if (!handled) {
		return DefWindowProc(window, message, wparam, lparam);
	}

	return 0;
}