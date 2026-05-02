#pragma once
#include "Actor.h"
#include "Core/Delegate/DelegateMacros.h"


// 테스트용 Delegate 타입 선언
FUNC_DECLARE_DELEGATE(FIntDelegate, int, int, int)        // int(int, int)
DECLARE_DELEGATE_OneParam(FVoidIntDelegate, int)           // void(int)
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiIntDelegate, int) // void(int) multicast

class ADelegateTestActor : public AActor
{
public:
    DECLARE_CLASS(ADelegateTestActor, AActor)
    ADelegateTestActor() = default;
    ~ADelegateTestActor() = default;

public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void TestSinglecastDelegate();
    void TestMulticastDelegate();

    // BindUObject / AddUObject 테스트용 멤버 함수
    int  OnSinglecastCallback(int A, int B);
    void OnMulticastCallback(int Value);

    int MulticastResult = 0;
};
