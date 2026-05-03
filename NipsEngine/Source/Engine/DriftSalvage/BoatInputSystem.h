#pragma once

class UWorld;

// 임시 테스트용: 화살표 키로 Boat actor를 월드 XY 평면에서 이동시킨다.
//   ↑ : +X / ↓ : -X / → : +Y / ← : -Y
// 회전은 없고, 단순 평면 이동.
class FBoatInputSystem
{
public:
    void Tick(UWorld* World, float DeltaTime);

private:
    float MoveSpeed = 5.0f; // 단위/초
};