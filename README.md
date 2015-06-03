**--- NppGTags ---**
======================
**GTags plugin for Notepad++**

This is a frontend to GNU Global source code tagging system (GTags) for Notepad++. It provides various functions to ease project code navigation - search for file, definition, reference, literal, regular expression, perform auto-completion.

You'll need GTags binaries for Win32 to use this plugin. Those are supplied with the plugin binary for convenience.
You can also download them from [GNU Global official website](http://www.gnu.org/software/global/global.html) - look for the Win32 port. Put GTags Win32 binaries in folder named *NppGTags*.


**Installation**
======================

Copy *NppGTags.dll* and *NppGTags* folder containing GTags binaries to your Notepad++ plugins directory, start Notepad++ and you are ready to go.


**Usage**
======================

You can find all supported commands in the NppGTags plugin menu.
To make your life easier use Notepad++'s shortcut settings to assign whatever shortcuts you like to the plugin commands. There are no predefined shortcuts in the plugin to avoid possible conflicts with other Notepad++ plugins.

Use plugin **Settings** to tune its operation. There you can select the code parser to be used. The default is the built-in GTags parser but it supports only C, C++, Java and several other languages at the time this doc was written. Ctags supports considerably more languages but does not allow reference search. Pygments also supports lots of languages + reference search but requires external Python library (Pygments) that is not supplied with the plugin.

First you need to create GTags database for your project - **Create Database**. In the dialog simply select your project's top folder and GTags will index all supported source files recursively.
It will create database files (*GTAGS*, *GRTAGS* and *GPATH*) in the selected folder.
**Delete Database** invoked from any openned file in the project will delete those.

If you run one of the plugin's **Find** commands (including **Search**) from any openned file in the project it will search the database for:

1. what you have selected if there is selection;
2. the word that is under the caret if there is no selection;
3. what you have entered in the search box that will appear if there is no word under the caret.

The search box allows choosing case sensitivity and regexp for the **Find** command where applicable. It also provides search completion list where possible which appears when you enter several characters in the box. If the **Find** command is started without the search box (case 1 and 2) the search is case sensitive and literal (no regexp).

---

**Find File** command will skip step 2, it will directly go to step 3 if there is no selection.
It will automatically fill the search box (from step 3) with the name of the current file without the extension to make switching between source - header file easier.
**Find File** will search for paths containing the given string.
All paths are relative to the project's directory and are case sensitive.

All **Find** commands will show Notepad++ docking window with the results.
Each such command will place its results in a separate tab that will automatically become active.
Clicking on another tab will show that command's results. You can also use the *Left* and *Right* arrow keys to switch between tabs.

Double-clicking, hitting *Space* or *Enter* on search result line will take you to the source location. You can also do that by left-clicking on the highlighted searched word in the result line. Your current location will be saved - use **Go Back** command to visit it again. You can 'undo' the **Go Back** by using **Go Forward** command.

Right clicking or hitting *ESC* will close the currently active search results tab.

Left-clicking in the margin area ([+] / [-] signs) or pressing *'+'* / *'-'* keys will unfold / fold lines. To fold a line it is not necessary to click exactly the [-] sign in the margin - clicking in any sub-line's margin will do.

The results window is Scintilla window actually (same as Notepad++). This means that you can use *Ctrl* + mouse scroll to zoom in / out or you can select text and copy it (*Ctrl* + *'C'*).

In GTags' terminology, *Symbol* is reference to identifier for which definition was not found. All local variables are symbols for example.

If you search for *Definition* / *Reference* and GTags doesn't find anything the plugin will automatically invoke search for *Symbol*.
This will be reported in the search results window header.

**Search** will search for a string either "literally" (not using regular expressions) or using regular expressions if that is selected through the search box options.

As a summary, **Find Definition / Reference** will search for identifiers (single whole words) whereas
**Search** will search for strings in general (parts of words, several consecutive words, etc.) either literally or using regular expressions.

**AutoComplete** will show found *Definitions* + found *Symbols*. It will always look for the whole word under the caret.

While auto complete results window is active you can narrow the results shown by continuing typing.
*Backspace* will undo the narrowing one step at a time (as the newly typed characters are deleted).
Double-clicking or pressing *Enter*, *Tab* or *Space* will insert the selected auto complete result.

**AutoComplete File** is useful if you will be including headers for example.

**AutoComplete** and **Find Definition** commands will also search library databases if such are used. That is configured through the plugin's **Settings** window.
