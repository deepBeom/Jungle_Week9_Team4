#pragma once

class UWorld;

// Drift Salvage Boat actor movement.
// Uses the same gameplay movement keys handled by FGameInputController:
//   W/S: +X/-X, D/A: +Y/-Y
class FBoatInputSystem
{
public:
    void Tick(UWorld* World, float DeltaTime);

private:
    float MoveSpeed = 5.0f; // 단위/초
};
