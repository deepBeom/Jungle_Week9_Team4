#pragma once
#pragma once
#include <functional>
#include <vector>

// MutlicastDelegate는 반환값이 void
template<typename... Args>
class TMulticastDelegate
{
public:
	using HandlerType = std::function<void(Args...)>;

	// 일반 함수나 람다 등록
	void Add(const HandlerType& handler)
	{
	}

	// 클래스 멤버 함수 바인딩
	template<typename T>
	void AddDynamic(T* Instance, void (T::* Func)(Args...))
	{
	}

	void Broadcast(Args... args)
	{
	}

private:
	std::vector<HandlerType> Handlers;
};


