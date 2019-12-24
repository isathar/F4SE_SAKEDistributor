# SAKE Distributor v0.9.5

An experimental dynamic patcher for Fallout 4 with the capability to edit various form types in memory.

-------------------------------------------------

## What It Does

- Enables the creation of dynamic patches using JSON files.
- Makes it possible to create simple mods with edits to existing forms that don't require additional mod plugins to be loaded.
- Compatibility patches that allow different mods to work together are also possible.
- Overrides are loaded after the game finishes loading all mods/forms, so they're independent of plugin load order.
- Includes a profile system that allows loading different combinations of overrides depending on your loaded mods.
- Supported Form types: LeveledItems, LeveledCharacters, Races, Actors, Armors, Weapons, MiscItems, Ammo, and Ingestibles.
- Also supports adding prefixes to item names for inventory sorting.
- Overrides do not affect save games, and most of them should be safe to enable/disable at any time.
- *Note: Templates and Overrides are very unfinished for now. More will be finished and added as I get to them over time.*

-------------------------------------------------

## Compiling Notes

- Created using Visual Studio 2017 v141, Windows SDK 10.0.17763.0

-------------------------------------------------

## Credits/License Info

- Thank you to the F4SE team for providing and updating the tools that make this kind of stuff possible.
- Uses *JSON for Modern C++* (https://github.com/nlohmann/json)
- Thank you to Bethesda Game Studios for Fallout 4 and its Creation Kit.
- This is a mod, so the following applies: (*the Software* being Fallout 4, and *the Publisher* being Zenimax/Bethesda Softworks)
	- THIS MATERIAL IS NOT MADE, GUARANTEED OR SUPPORTED BY THE PUBLISHER OF THE SOFTWARE OR ITS AFFILIATES.

-------------------------------------------------
