// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oConcurrency/future.h>

namespace ouro { namespace future_detail {

class future_category_impl : public std::error_category
{
public:
	const char* name() const { return "future"; }
	std::string message(int _ErrCode) const
	{
		switch (_ErrCode)
		{
			case future_errc::broken_promise: return "broken_promise: This means the promise has been destroyed before it has set a value or error code and the future may wait forever for a value that isn't going to come.";
			case future_errc::future_already_retrieved: return "future_already_retrieved: get_future can only be called once per promise. Use shared_future for multiple futures.";
			case future_errc::promise_already_satisfied: return "promise_already_satisfied: this promise has already had a value or error set on it. Thus the future waiting on the value may have already unblocked itself as a result.";
			case future_errc::no_state: return "no_state: The future/promise is not initialized.";
			case future_errc::no_implementation: return "this case is not yet handled by ouro::future";
			default: return "unrecognized future state";
		}
	}
};

}

const std::error_category& future_category()
{
	static future_detail::future_category_impl sSingleton;
	return sSingleton;
}

}
