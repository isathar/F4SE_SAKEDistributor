# SAKE Distributor Configuration

------------------------------------------------------------------------------------
## Configuration Files

- Config files use the JSON format (see https://www.json.org/json-en.html ).
	- Replace \\ with \\\\ in strings.  *if viewed in a regular text editor: that's 1 backslash and 2 backslashes*
	- You can add any number of blank lines between objects to help make things more readable.
		- It's also possible to do the opposite and combine as many entries as you want into single lines.
- Any variables not specified as required can be omitted. They will either use their default values or be ignored.
- The JSON spec and the parser used in this tool do not support comments.
	- If you need to add a comment somewhere, use variables named "comment" (or any unused variable names).
	- They won't be used in code, but are still loaded into memory, so don't overdo it.
	- Multi-line comments are possible using arrays.
- A User-Defined Language file for Notepad++ is included in the nppUDL folder to make the json configuration files easier to read.
- **FormID String Format**
	- "PluginName.esp|000000"
	- Replace the first byte of the *FormID* with its plugin name and extension.
	- The rest must be a hex string (**don't convert it to an int**).
	- Leading 0s can be omitted. *-ex.:  Plugin.esp|00012F == Plugin.esp|12F*
	- Use "none" if something needs to be null. (for disabling naming rules, for example)

------------------------------------------------------------------------------------
## SAKE.ini

- Optional file found in the F4SE Plugins directory.
- **sDataPath** *(String) - optional*
	- Path to the folder containing Config.json and *Overrides*.
	- Relative to Fallout 4's base directory.
- **iDebugLogLevel** *(int) - optional*
	- Controls the amount of information written to the log.
		- 0: *errors and warnings only*
		- 1: *errors, warnings and basic loading info*
		- 2: *errors, warnings and extended loading info*

------------------------------------------------------------------------------------
## Config Presets

- Contain the list of Overrides and their load order.
- Found in (sDataPath)/Config/
- **name** *(String) - optional*
	- For documentation and eventual display in a menu.
- **description** *(String) - optional*
	- For documentation and eventual display in a menu.
- **active** *(Array(Object)) - required*
	- The list of *Overrides* to load and their requirements, if any.
	- Paths can be specific json files or folder names. 
		- If a folder name is used, all json files it contains are loaded. Subdirectorie are ignored and can be used for optional files.
	- Overrides are loaded in the list's order.
	- **Object Variables:**
		- **name** *(String) - required*
			- The path to the *Override* to load, relative to *sDataPath*
		- **includeIf** *(Array(String)) - optional*
			- A list of mod plugins that are required to load this entry.
		- **excludeIf** *(Array(String)) - optional*
			- A list of mod plugins that will prevent this entry from being loaded.

------------------------------------------------------------------------------------
## Overrides

- **JSON files containing form edits.**
- Found in (sDataPath)/Overrides/
- **menuName** *(String) - optional*
	- For documentation and eventual display in a menu.
- **menuDescription** *(String) - optional*
	- For documentation and eventual display in a menu.
- **requirements** *(Array(String)) - optional*
	- List of mods (plugin names) that need to be enabled to load this *Override* file.
- **raceOverrides** *(Array(Object)) - optional*
	- The list of edits to Race base forms.
	- See *FormTypes*.md for information on possible form edits.
	- Loaded during the first pass before any other overrides.
- **overrides** *(Array(Object)) - optional*
	- The list of edits to base forms.
	- See *FormTypes*.md for information on possible form edits.
	- Loaded during the second pass for generic forms.
- **gameSettings** *(array(Object)) - optional*
	- List of GameSettings to edit.
	- Loaded during the second pass for generic forms.
	- **Object Variables:**
		- **name** *(String) - required*
			- Setting name.
		- **valueFloat** *(float) - optional*
			- Value to change the setting to if it's a float setting.
		- **valueInt** *(int) - optional*
			- Value to change the setting to if it's an integer setting.
		- **valueBool** *(bool) - optional*
			- Value to change the setting to if it's a boolean setting.
		- **valueString** *(String) - optional*
			- Value to change the setting to if it's a string setting.
- **namePrefixes** *(Array(Object)) - optional*
	- List of prefixes to add to the specified forms' names.
	- Can be used for inventory sorting.
	- Loaded in the final pass after all other overrides.
	- **Object Variables:**
		- **prefix** *(String) - required*
			- The text to add to the front of the specified forms' names.
		- **forms** *(Array(FormID String)) - required*
			- List of forms to apply the prefix to.
			- Limited to types that can be stored in inventories (except holotapes and books/notes for now).
