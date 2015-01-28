## --- NppGTags ---
**GTags plugin for Notepad++**

This is a frontend to GNU Global source code tagging system (GTags) for Notepad++.
You'll need GTags binaries for Win32 to use this plugin. Those are supplied with the plugin binary for convenience.
You can also download them from [GNU Global official website](http://www.gnu.org/software/global/global.html) - look for the Win32 port.
Put GTags Win32 binaries in folder named *NppGTags*.

### Installation:

Copy *NppGTags.dll* and *NppGTags* folder containing GTags binaries to your Notepad++ plugins directory, start Notepad++ and you are ready to go.

### Usage:

You can find all supported commands in the NppGTags plugin menu.

First you need to create GTags database for your project - simply select your project's top folder and GTags will index all supported source files recursively.
It will create database files (*GTAGS*, *GRTAGS* and *GPATH*) in the selected folder.

Open source file from the project.
If you run one of the plugin's *Find* commands (including *Grep*) it will search the database for:

1. what you have selected if there is selection;
2. the word that is under the caret if there is no selection;
3. what you have entered in the text box that will appear if there is no word under the caret.

*Find File* command will skip step 2, it will directly go to step 3 if there is no selection.
It will automatically fill the text box (from step 3) with the name of the current file without the extension to make switching between source - header file easier.

All *Find* commands will show Notepad++ docking window with the results.
Each such command will place its results in a separate tab that will automatically become active.
Clicking on another tab will show that command's results.
Double-clicking or hitting *Space* on search result line will take you to the source location. Your current location will be saved - use *Go Back* command to visit it again.
Right clicking will close the currently active search results tab.

In GTags' terminology, *Symbol* is reference to identifier for which definition was not found.

If you search for *Definition*/*Reference* and GTags doesn't find anything the plugin will automatically invoke search for *Symbol*.
This will be reported in the search results window header.

*Find Literal* will search for a string "literally" (not using regular expressions).

As a summary, *Find Definition/Reference* will search for identifiers (single whole words) whereas
*Find Literal* and *Grep* will search for strings in general (parts of words, several consecutive words, etc.) either literally or using regular expressions.

*Find File* will search for paths containing the given string.
All paths are relative to the project directory and are case sensitive.

*AutoComplete* will show found *Definitions* + found *Symbols*. It will always look for the whole word under the caret.
While auto complete results window is active you can narrow the results shown by continuing typing.
*Backspace* will undo the narrowing one step at a time (as the newly typed characters are deleted).
Double-clicking or pressing *Enter*, *Tab* or *Space* will insert the selected auto complete result.

*AutoComplete File* is useful if you will be including headers for example.
Have in mind however that the results shown start from the matched string to the end of the found path
and since the matched string might be in the middle of a filename the result might not be valid.
For example if your file is *\inc\Source.h* inside the project directory and you try to auto complete file string '*our*'
you will get as result *ource.h* which is obviously not what you need.
To work-around this always start *AutoComplete File* for the desired place in the file path.
What I mean is if you want to include *Source.h* you should type '*Sou*', invoke *AutoComplete File* and you will get as result *Source.h*.
If you want to specify the path with the include type '*inc\S*', invoke *AutoComplete File* and you will get as result *inc\Source.h*.
