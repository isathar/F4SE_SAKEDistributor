# SAKE Distributor Configuration

------------------------------------------------------------------------------------
## Configuration Files

- Config files use the JSON format (see https://www.json.org/json-en.html ).
	- Replace \\ with \\\\ in strings.  *if viewed in a regular text editor: that's 1 backslash and 2 backslashes*
	- You can add any number of blank lines between objects to help make things more readable.
		- It's also possible to go the opposite direction, and combine as much as you want into single lines.
- Paths are relative to Fallout 4's base directory.
- Any variables not specified as required can be omitted. They will either use their default values or be ignored.
- The JSON spec and the parser used in this tool do not support comments.
	- If you need to add a comment somewhere, use variables named "comment/s" (or any unused variable names).
	- They won't be used in code, but are still loaded into memory, so don't overdo it.
	- Multi-line comments are possible using arrays.
- A User-Defined Language file for Notepad++ is included in the nppUDL folder to make editing the json configuration files easier.
- **FormID String Format**
	- "PluginName.esp|000000"
	- Replace the first byte of the *FormID* with its plugin name and extension.
	- The rest must be a hex string (**don't convert it to an int**).
	- Leading 0s can be omitted. *-ex.:  Plugin.esp|000123 -> Plugin.esp|123*
	- Set to "none" to use a null form. (for disabling naming rules, for example)

------------------------------------------------------------------------------------
## Main Config.json

- **activeProfile** *(String) - required*
	- The name of the *Profile* to load.
- **templatesPath** *(String) - required*
	- Path to the folder containing *Templates* and *Overrides*.
- **profilesPath** *(String) - required*
	- Path to the folder containing *Profiles*.
- **debugLogLevel** *(int) - required*
	- Controls the amount of information written to the log.
		- 0: *errors and warnings only*
		- 1: *errors, warnings and basic loading info*
		- 2: *errors, warnings and extended loading info*

------------------------------------------------------------------------------------
## Overrides

- **JSON files containing the actual form edits.**
- **name** *(String) - optional*
	- For documentation and eventual display in a menu.
- **description** *(String) - optional*
	- For documentation and eventual display in a menu.
- **requirements** *(Array(String)) - optional*
	- List of mods (plugin names) that need to be enabled to load this *Override* file.
- **raceOverrides** *(Array(Object)) - optional*
	- The list of edits to Race base forms.
	- Loaded during the first pass before any other overrides, but otherwise uses the same rules.
- **overrides** *(Array(Object)) - optional*
	- The list of edits to base forms.
	- See *FormTypes*.md for information on possible form edits.
	- Loaded during the second pass for generic forms.
- **gameSettings** *(array(Object)) - optional*
	- List of GameSettings to edit.
	- Loaded during the second pass for generic forms.
	- **Variables:**
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
	- **Variables:**
		- **prefix** *(String) - required*
			- The text to add to the front of the specified forms' names.
		- **forms** *(Array(FormID String)) - required*
			- List of forms to apply the prefix to.
			- Limited to types that can be stored in inventories.

------------------------------------------------------------------------------------
## Templates

- Folders in *templatesPath* that can contain multiple *Overrides*.
- Activated by adding the *Template's* name/path to the loaded *Profile's* *active* list.
	- The Template's name used in the *active* list is its path relative to *templatesPath*.
- Every JSON file at an active *Template's* path is loaded.
	- Subdirectories are not loaded, and can be used to contain optional *Overrides* to load for different *Profiles*.

------------------------------------------------------------------------------------
## Profiles

- **Contain lists of *Overrides* and/or *Templates* to load.**
- **Allow conditional loading based on installed mods.**
- **name** *(String) - optional*
	- For documentation and eventual display in a menu.
- **description** *(String) - optional*
	- For documentation and eventual display in a menu.
- **active** *(Array(Object)) - required*
	- The list of *Templates* and/or *Overrides* to load and their requirements, if any.
	- Also determines the load order.
	- **Variables:**
		- **name** *(String) - required*
			- The path to the *Template* or *Override* to load, relative to *templatesPath*
		- **includeIf** *(Array(String)) - optional*
			- A list of mod plugins that are required to load this entry.
		- **excludeIf** *(Array(String)) - optional*
			- A list of mod plugins that will prevent this entry from being loaded.

------------------------------------------------------------------------------------
