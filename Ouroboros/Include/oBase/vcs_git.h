// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// vcs-compliant interface for handling version control through git

#pragma once
#include <oBase/vcs.h>

namespace ouro {

class git_t : public vcs_t
{
public:
	git_t() {}
	git_t(const vcs_init_t& init) { initialize(init); }

	vcs_protocol protocol() const override;
	bool available() const override;
	bool is_up_to_date(const char* path, const vcs_revision& at_revision = vcs_revision()) const override;
	char* root(char* dst, size_t dst_size, const char* path) const override;
	template<size_t size> char* root(char (&dst)[size], const char* path) const { return root(dst, size, path); }
	vcs_revision revision(const char* path) const override;
	void status(const char* path, vcs_enumerate_fn enumerator, void* user, vcs_visit_option option = vcs_visit_option::modified_and_untracked, const vcs_revision& up_to_revision = vcs_revision()) const override;

	// not yet implemented
	void add   (const char** paths, size_t num_paths) const override;
	void edit  (const char** paths, size_t num_paths) const override;
	void remove(const char** paths, size_t num_paths) const override;
	void revert(const char** paths, size_t num_paths) const override;
	void commit(const char** paths, size_t num_paths) const override;
};

}
