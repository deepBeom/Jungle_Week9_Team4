#include "Engine/Runtime/EngineLoop.h"

#include "Editor/EditorEngine.h"
#include "Misc/ObjViewer/ObjViewerEngine.h"
#include "sol/sol.hpp"


void FEngineLoop::CreateEngine()
{
#if IS_OBJ_VIEWER
	GEngine = UObjectManager::Get().CreateObject<UObjViewerEngine>();
#elif WITH_EDITOR
	GEngine = UObjectManager::Get().CreateObject<UEditorEngine>();
#else
	GEngine = UObjectManager::Get().CreateObject<UEngine>();
#endif
}

bool FEngineLoop::Init(HINSTANCE hInstance, int nShowCmd)
{
	(void)nShowCmd;
	
	UE_LOG("Hello, ZZup Engine!");

	// Sol2 / Lua 연동 테스트
	{
		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::math);

		lua.set_function("add", [](int a, int b) { return a + b; });

		lua.script(R"(
			result = add(3, 7)
			greeting = "Hello from Lua!"
		)");

		int result = lua["result"];
		std::string greeting = lua["greeting"];
		UE_LOG("[Lua] add(3, 7) = %d", result);
		UE_LOG("[Lua] %s", greeting.c_str());
	}

	if (!Application.Init(hInstance))
	{
		return false;
	}

	Application.SetOnSizingCallback([this]()
		{
			Timer.Tick();
			GEngine->Tick(Timer.GetDeltaTime());
		});

	Application.SetOnResizedCallback([](unsigned int Width, unsigned int Height)
		{
			if (GEngine)
			{
				GEngine->OnWindowResized(Width, Height);
			}
		});

	CreateEngine();
	GEngine->Init(&Application.GetWindow());
	GEngine->SetTimer(&Timer);
	GEngine->BeginPlay();

	Timer.Initialize();

	return true;
}

int FEngineLoop::Run()
{
	while (!Application.IsExitRequested())
	{
		Application.PumpMessages();

		if (Application.IsExitRequested())
		{
			break;
		}

		Timer.Tick();
		GEngine->Tick(Timer.GetDeltaTime());
	}

	return 0;
}

void FEngineLoop::Shutdown()
{
	if (GEngine)
	{
		GEngine->Shutdown();
		UObjectManager::Get().DestroyObject(GEngine);
		GEngine = nullptr;
	}
}
