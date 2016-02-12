=== oExternals ===

Ouroboros uses certain 3rd-party libraries. They are provided here in a form that compiles with compatible settings by using the same property sheets and same lib/bin directories as the internal Ouroboros code.

The oExternals solution contains:
ispc_texcomp libjpegTurbo libpng lzma snappy zlib for building

These are built to Ouroboros/Lib/...

The following 3rd-party source is so small, it's included directly in oProjects projects:
bullet calfaq OpenBSD tlsf

TBB is used from its binary build. See TBB/oNotes for more info.