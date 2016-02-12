This describes OOOii lib's usage of MSVC Property Sheets.

One of the more frustrating occurrences in programming is getting 3rd party libs to work together. To better enable this in larger projects Ouroboros uses property sheets and ensures all projects built internally use the property sheets so that all build settings remain consistent. When setting up property sheets the rules are:
1. Higher property sheets in the list override those below. vcproj settings are at the highest level.
2. Anything in bold overrides things beneath it. Anything not in bold will take the highest priority setting from the property sheets.

The intended order for Ouroboros libs is:

Debug32/64 | Release32/64: These are overrides specified to a debug/release build. Because some featured don't behave in 64-bit the same as they do in 32-bit there are 4 permutations of build-specific overrides.

Common: Most of the settings and policy decisions (RTTI or not, exceptions or not, etc.) are done here.

pch: These are the settings for precompiled headers. The intent is that if pch isn't included then a regular build is done.

Ouroboros(Private|Public)ExternalDependencies. Ouroboros libs use the private one for paths to external libs. Client applications should use the public version.