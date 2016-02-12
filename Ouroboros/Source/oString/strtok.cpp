// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>

namespace ouro {

char* strtok(const char* token, const char* delim, char** ctx)
{
	struct context { char* Context; char strToken[1]; };
	context* internal_ctx = (context*)*ctx;
	char* s = nullptr;
	if (token)
	{
		size_t size = strlen(token) + 1;
		*ctx = new char[size + sizeof(context)];
		internal_ctx = (context*)*ctx;
		strlcpy(internal_ctx->strToken, token, size);
		s = internal_ctx->strToken;
	}
	char* tok = strtok_r(s, delim, &internal_ctx->Context);
	if (!tok) delete [] *ctx, *ctx = nullptr;
	return tok;
}

void end_strtok(char** ctx)
{
	if (ctx && *ctx) delete [] *ctx;
}

}
