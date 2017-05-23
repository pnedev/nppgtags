**--- NppGTags ---**
======================
**GTags plugin for Notepad++**

This is a front-end to GNU Global source code tagging system (GTags) for Notepad++. It provides various functions to ease project code navigation - search for file, definition, reference, literal, regular expression, perform auto-completion.

You'll need GTags binaries for Win32 to use this plugin. Those are supplied with the plugin binary for convenience.
You can also download them from [GNU Global official website](http://www.gnu.org/software/global/global.html) - look for the Win32 port. Put GTags Win32 binaries in folder named *NppGTags*.


**Build Status**
======================

AppVeyor `VS2013` and `VS2015`  [![Build status](https://ci.appveyor.com/api/projects/status/b4aam50a4q2vacd7?svg=true)](https://ci.appveyor.com/project/pnedev/nppgtags)


**Installation**
======================

Copy *NppGTags.dll* and *NppGTags* folder containing GTags binaries to your Notepad++ plugins directory, start Notepad++ and you are ready to go.


**Usage**
======================

You can find all supported commands in the NppGTags plugin menu.
To make your life easier use Notepad++'s shortcut settings to assign whatever shortcuts you like to the plugin commands. There are no predefined shortcuts in the plugin to avoid possible conflicts with other Notepad++ plugins.

Use plugin **Settings** to tune its operation.
There you can select the code parser to be used.
The default is the built-in *GTags* parser but it supports only C, C++, Java, PHP and several other languages at the time this doc was written.
*Ctags* supports considerably more languages and is continuously evolving but does not allow reference search at the moment.
*Pygments* also supports lots of languages + reference search but requires external Python library (*Pygments*) that is not supplied with the plugin.

From **Settings** you can also set the auto-update database behavior, the linked libraries databases (if any) and the ignored sub-paths. The linked libraries are completely manageable from the settings window. The ignored sub-paths setting is used for results filtering - the configured database sub-paths will be excluded from the search results.

There are two copies of the above-mentioned settings that are identical:

1. The global (default) ones: Those are used whenever the project / library database is created for the first time. You can access those at any time - just open the **Settings** window.
2. Per database settings: Those are related to the specific database. You can access those when you open **Settings** window while you are editing a project file.

You can also set a default database that will be used when performing searches from files without their own database (unparsed files). This setting is only global.


To start using the plugin first you need to create GTags database for your project - **Create Database**.
In the dialog simply select your project's top folder and GTags will index recursively all supported by the chosen parser source files. It will create database files (*GTAGS*, *GRTAGS*, *GPATH* and *NppGTags.cfg*) in the selected folder.

**Delete Database** invoked from any opened file in the project will delete those.

If you run one of the plugin's **Find** commands (those include the **Search** commands) from any opened file in the project it will search the database for:

1. what you have selected if there is selection;
2. the word that is under the caret if there is no selection;
3. what you have entered in the search box that will appear if there is no word under the caret.


The search box allows choosing case sensitivity and regexp for the **Find** command where applicable.
It also provides search completion list where possible which appears when you enter several characters in the box.
If the **Find** command is started without the search box (case 1 and 2) the search is case sensitive and literal (no regexp).

**Find File** command will skip step 2, it will directly go to step 3 if there is no selection.
It will automatically fill the search box (from step 3) with the name of the current file without the extension to make switching between source <-> header file easier.
**Find File** will search for paths containing the given string (anywhere in the name) although its search completion will show only paths that start with the entered string. That means that even if there is no completion shown you can still perform the search and find files containing the entered string.
All paths are relative to the project's directory.

In GTags' terminology, *Symbol* is reference to identifier for which definition was not found. All local variables are symbols for example.
If you search for *Definition* / *Reference* and GTags doesn't find anything the plugin will automatically invoke search for *Symbol*. This will be reported in the search results window header.

Any **Search** will search for a string either "literally" (not using regular expressions) or using regular expressions if that is selected through the search box options.
**Search in Source Files** will look only in files that are recognized as sources by the parser during database creation.
**Search in Other Files** respectively will look in all other (non-binary) files. This is useful to dig into documentation (text files) or Makefiles for example.

As a summary, **Find Definition / Reference** will search for identifiers (single whole words) whereas **Search...** will search for strings in general (parts of words, several consecutive words, etc.) either literally or using regular expressions.

**AutoComplete** will show found *Definitions* + found *Symbols*. It will look for the string from the beginning of the word to the caret position.

While auto complete results window is active you can narrow the results shown by continuing typing.
*Backspace* will undo the narrowing one step at a time (as the newly typed characters are deleted).
Double-clicking or pressing *Enter*, *Tab* or *Space* will insert the selected auto complete result.

**AutoComplete Filename** is useful if you will be including headers for example.

**AutoComplete** and **Find Definition** commands will also search library databases if such are used. That is configured per database through the plugin's **Settings** window.

All **Find** commands will show Notepad++ docking window with the results.
Each such command will place its results in a separate tab that will automatically become active.
Clicking on another tab will show that command's results. You can also use the *ALT* + *Left* and *ALT* + *Right* arrow keys to switch between tabs.

Double-clicking, hitting *Space* or *Enter* on search result line will take you to the source location. You can also do that by left-clicking on the highlighted searched word in the result line. Your currently edited document location will be saved - use **Go Back** command to visit it again. You can 'undo' the **Go Back** action by using **Go Forward** command.
By using the mouse or the arrow keys you can move around result lines and you can trigger new searches directly from the results window
(same rules about the searched string apply).

Right clicking or hitting *ESC* will close the currently active search results tab.

Left-clicking in the margin area ([+] / [-] signs) or pressing *'+'* / *'-'* keys will unfold / fold lines. To fold a line it is not necessary to click exactly the [-] sign in the margin - clicking in any sub-line's margin will do. Pressing *ALT* + *'+'* / *'-'* keys will unfold / fold all lines.
You can accomplish that also by double-clicking or hitting *Space* or *Enter* on the search results head line - this will toggle all lines fold / unfold state.
Clicking in head line margin or pressing *'+'* / *'-'* keys while head line is the active one will do the same.

The results window is Scintilla window actually (same as Notepad++). This means that you can use *CTRL* + mouse scroll to zoom in / out or you can select text and copy it (*CTRL* + *'C'*).

When the focus is on the results window pressing *CTRL* + *'F'* will open a search dialog. Fill-in what you are looking for and press *Enter*. The search dialog will remain open until you press *ESC*. While it is open you can continue searching by pressing *Enter* again. *Shift* + *Enter* searches backwards. If you close the search dialog you can continue searching for the same thing using *F3* and *Shift* + *F3* (forward or backward respectively). *F3* works while the search dialog is open as well. The search always wraps around when it reaches the results end - the Notepad++ window will blink to notify you in that case.

**Toggle Results Window Focus** command is added for convenience. It switches the focus back and forth between the edited document and the results window. It's meant to be used with a shortcut so you can use the plugin through the keyboard entirely.

Enjoy!
