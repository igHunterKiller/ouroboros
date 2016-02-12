// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/reporting.h>
#include <oGUI/msgbox_reporting.h>
#include "btt_test.h"
#include "pivot_draw.h"

using namespace ouro;

int main(int argc, const char* argv[])
{
	reporting::set_prompter(prompt_msgbox);
	//btt_test app;
	pivot_draw app;
	app.run();
	return 0;
}
