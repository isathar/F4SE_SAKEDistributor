# SAKE Distributor v0.10.0

An experimental dynamic patcher for Fallout 4.

- [Documentation](https://github.com/isathar/F4SE_SAKEDistributor/wiki)

-------------------------------------------------
## What It Does

- This tool allows you use json-based text files to create simple mods and compatibility patches that edit game forms in memory.
- All overrides are independent of plugin load order and most have no effect on save files (the main exceptions being spells and enchantments with scripts).
- It also supports adding prefixes to item names for inventory sorting and editing GameSettings.
- Currently supported form types:
	- LeveledItem, LeveledNPC, Race, Actor, Armor, Weapon, MiscItem, Key, Ammo, Ingestible, Projectile, Explosions, Containers, and Encounter Zone.
- *Note: Most of the included overrides are incomplete and will be finished eventually.*

-------------------------------------------------
## Compiling Notes

- The solution and project files were made *Visual Studio 2017* and probably require *Windows SDK 10.0.17763.0*.
- Instructions:
	- Copy the F4SE source files to a new directory.
	- Copy the SAKE source files to the same directory - the SAKE folder should be in *src\f4se*.
	- Open the solution using *src\f4se\SAKE.sln*.
	- Once loaded, edit the properties for the f4se project to change its *Configuration Type* to *Static Library* (under *General*).
	- It's also a good idea to remove the *Post-Build Event* from the *f4se*, *f4se_loader* and *f4se_steam_loader* projects.
		- They create unnecessary copies of the compiled F4SE binaries.
	- Make sure the compiler is set to Release mode.
	- After compiling, the DLL can be found in *src\out* by default. 
		- Edit the SAKE project's *Post-Build Event* if you want to use another directory for the output.

-------------------------------------------------
## Credits/License Info

- Created with *Microsoft Visual Studio Community 2017*.
- Uses *JSON for Modern C++* by Niels Lohmann and contributors. (https://github.com/nlohmann/json)
- Thank you to the F4SE team for providing and updating the tools that make this kind of stuff possible.
- Thank you to Bethesda Game Studios for Fallout 4 and its Creation Kit.
- This is a mod (I think?), so the following applies: (*the software* being Fallout 4)
- *THIS MATERIAL IS NOT MADE, GUARANTEED OR SUPPORTED BY THE PUBLISHER OF THE SOFTWARE OR ITS AFFILIATES.*

-------------------------------------------------
