// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/opttok.h>
#include <oGUI/msgbox.h>
//#include <oPlatform/oVersionUpdate.h>

using namespace ouro;

static option sCmdOptions[] = 
{
	{ 'w', "launcher-wait", "process-id", "Wait for process to terminate before launching" },
	{ 't', "wait-timeout", "milliseconds", "Try to forcibly terminate the -w process after\nthis amount of time" },
	{ 'v', "version", "Maj.Min.Build.Rev", "Force execution of the specified version" },
	{ 'e', "exe-name-override", "exe-name", "Override the default of <version>/launcher-name\nwith an explicit exe name" },
	{ 'c', "command-line", "options", "The command line to pass to the actual executable" },
	{ 'p', "prefix", "prefix", "A prefix to differentiatethe actual exe from the\nlauncher exe" },
	{ 'h', "help",  0, "Displays this message" },
};

namespace ouro {

	bool from_string(const char** _ppConstStr, const char* _Value) { *_ppConstStr = _Value; return true; }

}

#define oOPT_CASE(_ShortNameConstant, _Value, _Dest) case _ShortNameConstant: { if (!from_string(&(_Dest), value)) { throw std::invalid_argument("-%c %s cannot be interpreted as a(n) %s", (_ShortNameConstant), (_Value), typeid(_Dest).name()); } break; }
#define oOPT_CASE_DEFAULT(_ShortNameVariable, _Value, _NthOption) \
	case ' ': { throw std::invalid_argument("There should be no parameters that aren't switches passed"); break; } \
	case '?': { throw std::invalid_argument("Parameter %d is not recognized", (_NthOption)); break; } \
	case ':': { throw std::invalid_argument("Parameter %d is missing a value", (_NthOption)); break; } \
	default: { oTRACE("Unhandled option -%c %s", (_ShortNameVariable), oSAFESTR(_Value)); break; }
#if 0
void oParseCmdLine(int argc, const char* argv[], oVERSIONED_LAUNCH_DESC* _pDesc, bool* _pShowHelp)
{
	*_pShowHelp = false;
	const char* value = 0;
	char ch = opttok(&value, argc, argv, sCmdOptions);
	int count = 1;
	while (ch)
	{
		switch (ch)
		{
			oOPT_CASE('w', value, _pDesc->WaitForPID);
			oOPT_CASE('t', value, _pDesc->WaitForPIDTimeout);
			oOPT_CASE('v', value, _pDesc->SpecificVersion);
			oOPT_CASE('e', value, _pDesc->SpecificModuleName);
			oOPT_CASE('c', value, _pDesc->PassThroughCommandLine);
			oOPT_CASE('p', value, _pDesc->ModuleNamePrefix);
			oOPT_CASE_DEFAULT(ch, value, count);
			case 'h': *_pShowHelp = true; break;
		}

		ch = opttok(&value);
		count++;
	}
}

static void oLauncherMain(int argc, const char* argv[])
{
	oVERSIONED_LAUNCH_DESC vld;

	bool ShowHelp = false;
	oParseCmdLine(argc, argv, &vld, &ShowHelp);
	if (ShowHelp)
	{
		char help[1024];
		if (optdoc(help, path(argv[0]).filename().c_str(), sCmdOptions))
			printf(help);
		return;
	}

	oVURelaunch(vld);
}
#endif

int main(int argc, const char* argv[])
{
	throw std::system_error(std::errc::not_supported, std::system_category(), "Disabled until version update is resurrected");

#if 0

	try { oLauncherMain(argc, argv); }

	catch (std::exception& e)
	{
		path ModuleName = this_module::get_path();
		msgbox(msg_type::error, nullptr, ModuleName.filename(), "%s", e.what());
		return -1;
	}

	return 0;
#endif
}
