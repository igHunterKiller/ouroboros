// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oBase/scc.h>
//#include "scc_git.h"
//#include "scc_p4.h"
#include "scc_svn.h"

namespace ouro {

template<> const char* as_string(const scc_protocol& protocol)
{
	static const char* s_names[] =
	{
		"perforce",
		"svn",
		"git",
	};
	return as_string(protocol, s_names);
}

template<> const char* as_string(const scc_status& status)
{
	static const char* s_names[] =
	{
		"unknown",
		"unchanged",
		"unversioned",
		"ignored",
		"modified",
		"missing",
		"added",
		"removed",
		"replaced",
		"copied",
		"conflict",
		"merged",
		"obstructed",
		"out_of_date",
	};
	return as_string(status, s_names);
}

namespace detail {

class scc_category_impl : public std::error_category
{
public:
	const char* name() const noexcept override { return "future"; }
	std::string message(int errc) const override
	{
		switch (errc)
		{
			case scc_error::none:                     return "no error";
			case scc_error::command_string_too_long:  return "the command string is too long for internal buffers";
			case scc_error::scc_exe_not_available:    return "scc executable not available";
			case scc_error::scc_exe_error:            return "scc exe reported an error";
			case scc_error::scc_server_not_available: return "scc server not available";
			case scc_error::file_not_found:           return "file not found";
			case scc_error::entry_not_found:          return "entry not found";
			default: break;
		}
		return "unrecognized scc error code";
	}
};

} // namespace detail

const std::error_category& scc_category()
{
	static detail::scc_category_impl sSingleton;
	return sSingleton;
}

std::shared_ptr<scc> make_scc(scc_protocol _Protocol, scc_spawn_fn _Spawn, void* _User, unsigned int _TimeoutMS)
{
	switch (_Protocol)
	{
		case scc_protocol::svn: return detail::make_scc_svn(_Spawn, _User, _TimeoutMS);
		//case scc_protocol::perforce: return detail::make_scc_p4(_Spawn, _User, _TimeoutMS);
		//case scc_protocol::git: return detail::make_scc_git(_Spawn, _User, _TimeoutMS);
		default: break;
	}
	oThrow(std::errc::protocol_not_supported, "");
}

}
