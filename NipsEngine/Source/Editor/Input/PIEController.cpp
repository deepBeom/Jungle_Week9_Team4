#include "Editor/Input/PIEController.h"

#include "Engine/Input/GameInputController.h"
#include "Engine/Input/InputSystem.h"

void FPIEController::Tick(float DeltaTime, FGameInputController& GameInputController)
{
    (void)DeltaTime;

    InputSystem& Input = InputSystem::Get();
    if (Input.GetKeyDown(VK_ESCAPE) && EndPIECallback)
    {
        EndPIECallback();
        return;
    }

    if (Input.GetKeyDown(VK_F4))
    {
        bHostControlReleased = !bHostControlReleased;
    }

    // F4는 Lua가 원하는 상태를 지우지 않고, 지금 적용 가능한지만 바꿉니다.
    GameInputController.SetCursorHiddenAllowed(!bHostControlReleased);
    GameInputController.SetMouseLockAllowed(!bHostControlReleased);
}

void FPIEController::Reset()
{
    bHostControlReleased = false;
}
