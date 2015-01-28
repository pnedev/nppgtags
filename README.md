nppgtags
========

GTags plugin for Notepad++

This is a frontend to GNU Global source code tagging system (GTags) for
Notepad++.
You'll need GTags binaries for Win32 to use this plugin.
Those are supplied with the plugin binary for convenience.
You can also download them from GNU Global official website:
http://www.gnu.org/software/global/global.html

Put GTags Win32 binaries in folder named 'NppGTags'.


Installation:
========================

Copy NppGTags.dll and NppGTags folder containing GTags binaries to your
Notepad++ plugins directory, start Notepad++ and you are ready to go.


Usage:
========================

You can find all supported commands in the NppGTags plugin menu.

First you need to create GTags database for your project - simply select your
project's top folder and GTags will index all supported source files
recursively. It will create database files (GTAGS, GRTAGS and GPATH) in the
selected folder.

Open source file from the project.

If you run one of the plugin's 'Find' commands (including Grep) it will search
the database for:
1) what you have selected if there is selection;
2) the word that is under the cursor if there is no selection;
3) what you have entered in the text box that will appear if there is no word
under the cursor.

'Find File' command will skip 2), it will directly go to step 3) if there
is no selection. It will automatically fill the text box (from step 3) with
the name of the current file without the extension to make switching between
source <-> header file easier.

All 'Find' commands will show Notepad++ docking window with the results.
Each such command will place its results in a separate tab that will
automatically become active. Clicking on another tab will show that command's
results. Double-clicking (or hitting 'Space') on search result line will take
you to the source location. Right clicking will close the currently active
search results tab.

In GTags' terminology, Symbol is identifier that doesn't have definition.

If you search for Definition/Reference and GTags doesn't find anything the
plugin will automatically invoke search for Symbol. This will be reported
in the search results window header.

Auto-complete will show found Definitions + found Symbols. It will always look
for the whole word under the cursor.

Everything else should be self-explanatory.
