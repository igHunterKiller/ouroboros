// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#ifndef oBase_scc_svn_h
#define oBase_scc_svn_h

#include <oBase/scc.h>

namespace ouro {
	namespace detail {

class scc_svn : public scc
{
public:
	scc_svn(const scc_spawn& _Spawn, unsigned int _SpawnTimeoutMS = 10000) : Spawn(_Spawn), SpawnTimeoutMS(_SpawnTimeoutMS) {}

	scc_protocol::value protocol() const override { return scc_protocol::svn; }
	bool available() const override;
	char* root(const char* _Path, char* _StrDestination, size_t _SizeofStrDestination) const override;
	unsigned int revision(const char* _Path) const override;
	void status(const char* _Path, unsigned int _UpToRevision, scc_visit_option::value _Option, const scc_file_enumerator& _Enumerator) const override;
	scc_revision change(const char* _Path, unsigned int _Revision) const override;
	void sync(const char* _Path, unsigned int _Revision, bool _Force = false) override;
	void sync(const char* _Path, const ntp_date& _Date, bool _Force = false) override;
	void add(const char* _Path) override;
	void remove(const char* _Path, bool _Force = false) override;
	void edit(const char* _Path) override;
	void revert(const char* _Path) override;
private:
	scc_spawn Spawn;
	unsigned int SpawnTimeoutMS;
private:
	void spawn(const char* _Command, const scc_get_line& _GetLine) const;
	void spawn(const char* _Command, ouro::xlstring& _Stdout) const;
};

std::shared_ptr<scc> make_scc_svn(const scc_spawn& _Spawn, unsigned int _TimeoutMS);

	} // namespace detail
}

#endif