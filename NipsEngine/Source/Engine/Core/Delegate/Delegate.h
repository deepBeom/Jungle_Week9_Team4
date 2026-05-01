#pragma once
#include <functional>
#include <vector>

template<typename>
class TDelegate;

// SinglecastDelegate
template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>
{
public:

private:
	std::vector<HandlerType> Handlers;
};

