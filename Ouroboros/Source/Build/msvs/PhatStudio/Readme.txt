NOTE: PhatStudio is not yet ready for VS2015. Ctrl-, isn't a bad replacement,
so this will likely be deprecated once the codebase moves to VS2015.

PhatStudio is an awesome tool, but switch files doesn't recurse through multi-
layer filters. Here the v1.11 source code for SwitchFile.cs has been modified
to be fully recursive and the DLL is v1.11 rebuilt with the change. 

To use:
1. Close all instances of Visual Studio
2. install v1.11 
3. overwrite C:\Program Files (x86)\PhatStudio\PhatStudio.dll with version in 
   this folder.
4. Restart visual studio
