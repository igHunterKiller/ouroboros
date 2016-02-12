This readme describes the contents of the Build directory for Ouroboros.

== msvs ==
Microsoft Visual Studio quality of life improvements: plugins, common property sheets, user types and the like.
* oNoStepInto.reg: merges some filters so the VS debugger will never step into certain boilerplate functions.
* Mostly HLSL and stdint keywords
* A reg file to make VS2012's menus not yell at you.
* PhatStudio: If all you want is Alt-O and Find-File-Quick, use this instead of VisualAssist.
* Property sheets used in oProjects

== Reporting ==
We used to keep score by how many checkins you did. "But you could game the system by making lots of small checkins." Yes. Yes you should, er could.

== SystemStability ==
Some scripts for trying to convince MS Windows it is a 24-7 always-up public display O/S.