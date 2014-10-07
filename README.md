nppgtags
========

GTags plugin for Notepad++

This is GUI frontend to GNU Global source code tagging system (GTags) for
Notepad++.
You'll need GTags binaries for Win32 to use this plugin. You can download them
from GNU Global official website:
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

'Find File' command will not search for 2), it will directly go for 3) if there
is no selection. It will automatically fill in the text box (case 3) the name
of the current file to make switching between source <-> header easier.

All 'Find' commands will show Notepad++ docking Tree View UI with the results.
Double-clicking (or hitting 'Space') on search result will take you to the
source location. Right clicking will close the search result branch that is
selected (there might be several search result branches - those are with bold
in the tree view).
If you search for Definition/Reference and GTags doesn't find anything the
plugin will automatically invoke search for Symbol name. This will be reported
in the search result Tree View UI branch name.

Auto-complete will show found Definitions + found Symbols. It will always look
for the whole word under the cursor.

Everything else should be self-explanatory.
