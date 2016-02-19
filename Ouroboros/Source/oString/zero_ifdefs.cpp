// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <memory.h>
#include <regex>
#include <string>
#include <oCore/assert.h>
#include <oCore/countof.h>
#include <oString/string_source.h>
#include <unordered_map>

using namespace std;

namespace ouro {

struct ifdef_block
{
	enum type
	{
		unknown,
		ifdef,
		ifndef,
		if_,
		elif,
		else_,
		endif,
	};

	enum type type;
	const char* expression_start; // what comes after one of the opening TYPES. NULL for ELSE and ENDIF
	const char* expressionend;
	const char* block_start; // The data within the block
	const char* blockend;
};

// Matches #ifdef SYM, #ifndef SYM, #else, #endif
// Match1: ifndef or ifdef
// Match2: n or empty
// Match3: SYM or empty
// Match4: else or elseif or empty
// Match5: if or empty
// Match6: endif
static regex reIfdef("#[ \\t]*(if(n?)def)[ \\t]+([a-zA-Z0-9_]+)|#[ \\t]*(else(if)?)|#[ \\t]*(endif)", regex_constants::optimize); // ok static (duplication won't affect correctness)

static enum ifdef_block::type get_type(const cmatch& matches)
{
	const char* Opener = matches[1].first;
	const char* Else = matches[4].first;
	const char* Endif = matches[6].first;

	if (!memcmp(Opener, "ifdef", 5)) return ifdef_block::ifdef;
	else if (!memcmp(Opener, "ifndef", 6)) return ifdef_block::ifndef;
	else if (!memcmp(Opener, "if", 2)) return ifdef_block::if_;
	else if (!memcmp(Else, "elif", 4)) return ifdef_block::elif;
	else if (!memcmp(Else, "else", 4)) return ifdef_block::else_;
	else if (!memcmp(Endif, "endif", 4)) return ifdef_block::endif;
	return ifdef_block::unknown;
}

static void next_matching_ifdef(ifdef_block* p_blocks, size_t max_num_blocks, size_t* out_num_valid_blocks, const char* src_begin, const char* src_end)
{
	ifdef_block* pLastBlock = 0; // used to late-populate blockend
	ifdef_block* pCurBlock = 0;
	size_t blockIndex = 0;
	int open = 0;

	const char* cur = src_begin;

	cmatch matches;
	while (regex_search(cur, matches, reIfdef))
	{
		if (src_end && matches[0].first >= src_end)
			break;

		oCheck(blockIndex < max_num_blocks-1, std::errc::no_buffer_space, "block buffer too small");

		enum ifdef_block::type type = get_type(matches);

		// increment openness for each opening statement
		switch (type)
		{
			case ifdef_block::if_:
			case ifdef_block::ifdef:
			case ifdef_block::ifndef:
				open++;
				break;
			default:
				break;
		}

		// add new blocks on first and at-level-1 openness
		if (open == 1)
		{
			pCurBlock = &p_blocks[blockIndex++];
			memset(pCurBlock, 0, sizeof(ifdef_block));
			pCurBlock->type = type;

			// Record expression and block info if at same level
			switch (type)
			{
				case ifdef_block::if_:
				case ifdef_block::ifdef:
				case ifdef_block::ifndef:
					pCurBlock->expression_start = matches[3].first;
					pCurBlock->expressionend = matches[3].second;
					pCurBlock->block_start = matches[0].second;
					break;

				case ifdef_block::elif:
					// todo: record expression
					// pass thru

				case ifdef_block::else_:
				case ifdef_block::endif:
					pCurBlock->block_start = matches[0].second;
					pLastBlock->blockend = matches[0].first;
					break;
			}
		}

		// If a closing statement, close out everything
		if (type == ifdef_block::endif)
		{
			open--;
			if (!open)
			{
				pCurBlock->block_start = matches[0].second;
				pCurBlock->blockend = matches[0].second;
				break;
			}
		}

		// Move to next position
		cur = matches[0].second;

		if (open == 1)
			pLastBlock = pCurBlock;
	}

	if (out_num_valid_blocks)
		*out_num_valid_blocks = blockIndex;
}

typedef unordered_map<string, string> macros_t;

// An internal version that works on an already-hash macros container
static char* zero_ifdefs_internal(const macros_t& macros, char* src_begin, char* src_end, char replacement)
{
	size_t numBlocks = 0;
	ifdef_block blocks[32];
	next_matching_ifdef(blocks, countof(blocks), &numBlocks, src_begin, src_end);
	
	if (numBlocks)
	{
		bool zeroBlock = false;
		for (ifdef_block* pCurBlock = blocks; pCurBlock->type != ifdef_block::endif; pCurBlock++)
		{
			switch (pCurBlock->type)
			{
				case ifdef_block::ifdef:
					zeroBlock = macros.end() == macros.find(string(pCurBlock->expression_start, pCurBlock->expressionend));
					break;
				case ifdef_block::ifndef:
					zeroBlock = macros.end() != macros.find(string(pCurBlock->expression_start, pCurBlock->expressionend));
					break;

				case ifdef_block::else_:
					// else will be the last block before endif, so it means that if we 
					// haven't zeroed a block yet, we should now
					zeroBlock = !zeroBlock;
					break;

				default:
					oThrow(std::errc::invalid_argument, "Unhandled case (Did #if/#elif get implemented?)");
			}

			if (zeroBlock)
			{
				for (const char* cur = pCurBlock->block_start; cur < pCurBlock->blockend; cur++)
					*const_cast<char*>(cur) = replacement;
			}

			else
			{
				// If we're not going to zero this block, then recurse into it looking for
				// other blocks to zero
				zero_ifdefs_internal(macros, const_cast<char*>(pCurBlock->block_start), const_cast<char*>(pCurBlock->blockend), replacement);
			}
		}
	}

	return src_begin;
}

static void hash_macros(macros_t& out_macros, const macro* macros)
{
	while (macros && macros->symbol && macros->value)
	{
		out_macros[macros->symbol] = macros->value;
		macros++;
	}
}

char* zero_ifdefs(char* str, const macro* macros, char replacement)
{
	macros_t internal_macros;
	hash_macros(internal_macros, macros);
	return zero_ifdefs_internal(internal_macros, str, nullptr, replacement);
}

}
