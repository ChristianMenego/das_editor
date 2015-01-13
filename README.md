# das_editor
Dragon Age: Inquisition command line save game editor.

This is a hackish command line style tool to allow modification of some stuff inside of the save files. Note that this likely has many bugs and requires user testing. [b]This opens save files in read only mode and will output a new file. I still recommend backing up saves.[/b]

For now, these are the values that can be modified:
	Eyeliner Intensity & Color
	Eye Shadow Intensity & Color
	Under-Eye Color
	Blush Intensity & Color
	Lip Shine, Intensity & Color
	Lip Liner Color
	Under-Brow Intensity & Color
	Eyebrow Color
	Eyelash Color
	Hair Color
	Scalp Hair Color
	Facial Hair Color
	Inner & Outer Iris Color

Instructions:
- Drag and drop a DAS file onto das_editor.exe OR run from the command line.
- If file opens succesfully, some basic info about the save will be printed, and then the menu. At this point you have 4 options:

1) Export values to file
	Creates a DASFACE file that stores the values from a save to a binary file.

2) Import values from file
	Imports values from a DASFACE file.

3) Manually edit all values
	This interactive mode lets you manually change every single value individually. It will print out the current value, and the min/max. Leaving it blank will keep the original value. This is useful for colors not included in the game, as well as having different lash/brow/hair colors.

4) Export XML files
	Finds all XML files embedded in the save and exports them to file. Note these XMLs are not indented properly, you will have to do that yourself.
