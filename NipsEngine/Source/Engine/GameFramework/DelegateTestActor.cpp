#include "DelegateTestActor.h"
#include <cassert>
#include "Editor/UI/EditorConsoleWidget.h"

DEFINE_CLASS(ADelegateTestActor, AActor)

void ADelegateTestActor::BeginPlay()
{
    AActor::BeginPlay();
    TestSinglecastDelegate();
    TestMulticastDelegate();
}

void ADelegateTestActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void ADelegateTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    AActor::EndPlay(EndPlayReason);
}

void ADelegateTestActor::TestSinglecastDelegate()
{
    UE_LOG("[Delegate] === SinglecastDelegate Test ===\n");

    // 1. 초기 상태 확인
    FIntDelegate Delegate;
    assert(!Delegate.IsBound());
    UE_LOG("[Delegate] IsBound() == false : PASS\n");

    // 2. 람다 바인딩 후 Execute
    Delegate.Bind([](int A, int B) { return A + B; });
    assert(Delegate.IsBound());
    assert(Delegate.Execute(3, 4) == 7);
    UE_LOG("[Delegate] Bind(lambda) + Execute : PASS\n");

    // 3. UnBind 후 IsBound 확인
    Delegate.UnBind();
    assert(!Delegate.IsBound());
    UE_LOG("[Delegate] UnBind() : PASS\n");

    // 4. BindUObject로 멤버 함수 바인딩
    Delegate.BindUObject(this, &ADelegateTestActor::OnSinglecastCallback);
    assert(Delegate.Execute(10, 5) == 15);
    UE_LOG("[Delegate] BindUObject + Execute : PASS\n");

    // 5. ExecuteIfBound — 바인딩 여부에 상관없이 안전하게 호출
    FVoidIntDelegate VoidDelegate;
    VoidDelegate.ExecuteIfBound(42); // 바인딩 없이 호출해도 크래시 없어야 함
    VoidDelegate.Bind([](int X) { (void)X; });
    VoidDelegate.ExecuteIfBound(42);
    UE_LOG("[Delegate] ExecuteIfBound : PASS\n");

    UE_LOG("[Delegate] === SinglecastDelegate All Passed ===\n\n");
}

void ADelegateTestActor::TestMulticastDelegate()
{
    UE_LOG("[Delegate] === MulticastDelegate Test ===\n");

    FMultiIntDelegate Multi;

    // 1. 람다 2개 등록 후 Broadcast
    MulticastResult = 0;
    FDelegateHandle H1 = Multi.Add([this](int X) { MulticastResult += X; });
    FDelegateHandle H2 = Multi.Add([this](int X) { MulticastResult += X * 2; });
    Multi.Broadcast(5);
    assert(MulticastResult == 15); // H1: 5, H2: 10
    UE_LOG("[Delegate] Add(lambda) x2 + Broadcast : PASS\n");

    // 2. H1 제거 후 Broadcast — H2만 남아야 함
    Multi.Remove(H1);
    MulticastResult = 0;
    Multi.Broadcast(5);
    assert(MulticastResult == 10); // H2: 10
    UE_LOG("[Delegate] Remove(handle) : PASS\n");

    // 3. AddUObject로 멤버 함수 등록 후 Broadcast
    FDelegateHandle H3 = Multi.AddUObject(this, &ADelegateTestActor::OnMulticastCallback);
    MulticastResult = 0;
    Multi.Broadcast(3);
    assert(MulticastResult == 9); // H2: 6, H3: 3
    UE_LOG("[Delegate] AddUObject + Broadcast : PASS\n");

    // 4. RemoveAll 후 Broadcast — 아무것도 실행되지 않아야 함
    Multi.RemoveAll();
    MulticastResult = 0;
    Multi.Broadcast(100);
    assert(MulticastResult == 0);
    UE_LOG("[Delegate] RemoveAll : PASS\n");

    UE_LOG("[Delegate] === MulticastDelegate All Passed ===\n\n");
}

int ADelegateTestActor::OnSinglecastCallback(int A, int B)
{
    return A + B;
}

void ADelegateTestActor::OnMulticastCallback(int Value)
{
    MulticastResult += Value;
}
