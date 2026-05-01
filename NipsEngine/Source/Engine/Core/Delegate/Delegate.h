#pragma once
#include <functional>
#include <cassert>

// 템플릿 특수화를 위한 전방선언
template<typename>
class TDelegate;

// SinglecastDelegate -> 반환 값 존재 하나만 등록
template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>
{
public:
	using HandlerType = std::function<Ret(Args...)>;

public:
	void Bind(HandlerType handler)
	{
		Handler = std::move(handler);
	}

	// void BindStatic();
	// void BindRaw();
	// void BindSP();

	template<typename T>
	void BindUObject(T* Instance, Ret(T::*func)(Args...))
	{
		Handler = [Instance, func](Args... args) -> Ret
			{
				return (Instance->*func)(std::forward<Args>(args)...);
			};
	}

	template<typename T>
	void BindUObject(const T* Instance, Ret(T::*func)(Args...) const)
	{
		Handler = [Instance, func](Args... args) -> Ret
			{
				return (Instance->*func)(std::forward<Args>(args)...);
			};
	}

	void UnBind() { Handler = nullptr; }

	bool IsBound() const { return Handler != nullptr; }

	Ret Execute(Args... args)
	{
		assert(IsBound() && "Execute called on unbound delegate");
		return Handler(std::forward<Args>(args)...);
	}

	// void 반환 전용
	void ExecuteIfBound(Args... args)
	{
		static_assert(std::is_void_v<Ret>, "ExecuteIfBound requires void return type");

		if (IsBound())
			Handler(std::forward<Args>(args)...);
	}
private:
	HandlerType Handler;
};
