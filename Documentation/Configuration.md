# SAKE Distributor Configuration

------------------------------------------------------------------------------------
## 1 - Configuration Files

- **Notes**

	- Config files use the JSON format (see https://www.json.org/json-en.html ).
		- Replace \\ with \\\\ in strings.  *if viewed in a regular text editor: that's 1 backslash and 2 backslashes*
		- You can add any number of blank lines between objects to help make things more readable.
			- It's also possible to go the opposite direction, and combine as much as you want into single lines.
	- Paths are relative to Fallout 4's base directory.
	- Any variables not specified as required can be omitted. They will either use their default values or be ignored.
	- The JSON spec and the parser used in this tool do not support comments.
		- If you need to add a comment somewhere, use variables named "comments", "documentation", or something like that.
		- They won't be used in code, but are still loaded into memory, so don't overdo it.
		- Multi-line comments are possible using arrays.
	- You may have noticed that all edits to races are confined to separate files that are placed in Templates\\RaceEdits.
		- **This is because any overrides to Race forms must be processed before Actors or ActorValues will reset.**
		- Add "hasRaceEdits": true to an override file to process it before other overrides.

- **FormID String Format**

	- "PluginName.esp|000000"
	- Replace the first byte of the *FormID* with its plugin name and extension.
	- The rest must be a hex string (**don't convert it to an int**).
	- Leading 0s can be omitted. *-ex.:  Plugin.esp|000123 -> Plugin.esp|123*

------------------------------------------------------------------------------------
## 2 - Main Config.json

- **Variables**

	- *activeProfile - required*
		- The name of the *Profile* to load.
	- *templatesPath - required*
		- Path to the folder containing *Templates* and *Overrides*.
	- *profilesPath - required*
		- Path to the folder containing *Profiles*.
	- *debugLogLevel - required*
		- Controls the amount of information written to the log.
			- 0: *errors and warnings only*
			- 1: *errors, warnings and basic loading info*
			- 2: *errors, warnings and extended loading info*

------------------------------------------------------------------------------------
## 3 - Overrides

- **Notes**

	- JSON files containing the actual form edits.

- **Variables**

	- *name (string) - optional*
		- For documentation and eventual display in a menu.
	- *description (string) - optional*
		- For documentation and eventual display in a menu.
	- *hasRaceEdits (bool) - optional*
		- Used to mark *Override* files that contain Races.
	- *requirements (array(string)) - optional*
		- List of mods that need to be enabled to load this *Override* file.
	- *overrides (array(Object)) - required*
		- The list of edits to base forms.
		- Determines the load order.
		- The type of data loaded is determined by the *formType* variable.
		- See *FormTypes*.md for more information on possible form edits.

------------------------------------------------------------------------------------
## 4 - Templates

- **Notes**

	- Folders in *templatesPath* that can contain multiple *Overrides*.
	- Activated by adding the *Template's* name/path to the loaded *Profile's* *active* list.
		- The Template's name used in the *active* list is it's path relative to *templatesPath*.
	- Every JSON file at an active *Template's* path is loaded.
		- Subdirectories are not loaded, and can be used to contain optional *Overrides* to load for different *Profiles*.

------------------------------------------------------------------------------------
## 5 - Profiles

- **Notes**

	- Contain lists of *Overrides* and/or *Templates* to load.
	- Allow conditional loading based on installed mods.

- **Variables**

	- *name (string) - optional*
		- For documentation and eventual display in a menu.
	- *description (string) - optional*
		- For documentation and eventual display in a menu.
	- *active (array(Object)) - required*
		- The list of *Templates* and/or *Overrides* to load and their requirements, if any.
		- Also determines the load order.
		- *name (string) - required*
			- The path to the *Template* or *Override* to load, relative to *templatesPath*
		- *includeIf (array(string)) - optional*
			- A list of mod plugins that are required to load this entry.
		- *excludeIf (array(string)) - optional*
			- A list of mod plugins that will prevent this entry from being loaded.

------------------------------------------------------------------------------------
