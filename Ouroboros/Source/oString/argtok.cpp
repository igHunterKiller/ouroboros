// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/argtok.h>
#include <oString/string_path.h>
#include <string.h>
#include <stdlib.h>

namespace ouro {

const char** argtok(void* (*allocate)(size_t), const char* app_path, const char* command_line, int* out_num_args)
{
	// http://alter.org.ua/docs/win/args/
	// Alexander A. Telyatnikov

	char** argv;
	char* _argv;
	size_t len;
	int argc;
	char a;
	size_t i, j;

	bool in_QM;
	bool in_TEXT;
	bool in_SPACE;

	size_t path_size = 0;

	len = (size_t)strlen(command_line);
	if (app_path)
	{
		path_size = strlen(app_path) + 1;
		len += path_size;
	}

	i = ((len+2)/2)*sizeof(void*) + sizeof(void*);

	if (app_path)
		i += sizeof(void*);
	
	if (!allocate)
		allocate = malloc;

	size_t full_size = i + len + 2;

	argv = (char**)allocate(full_size);
	memset(argv, 0, full_size);

	_argv = (char*)(((unsigned char*)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = false;
	in_TEXT = false;
	in_SPACE = true;
	i = 0;
	j = 0;

	// optionally insert exe path so this is exactly like argc/argv
	if (app_path)
	{
		clean_path(argv[argc], path_size, app_path, '\\');
		j = (size_t)strlen(argv[argc])+1;
		argc++;
		argv[argc] = _argv+j;
	}

	while( (a = command_line[i]) != 0 ) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = false;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
			case '\"':
				in_QM = true;
				in_TEXT = true;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = false;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = false;
				in_SPACE = true;
				break;
			default:
				in_TEXT = true;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = false;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = nullptr;

	*out_num_args = argc;
	return (const char**)argv;
}

}
