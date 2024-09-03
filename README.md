To support NppGTags consider donating. Thank You.
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/pnedev)


**--- NppGTags ---**
======================
**GTags plugin for Notepad++**

This is a front-end to GNU Global source code tagging system (GTags) for Notepad++. It provides various functions to ease project code navigation - search for file, definition, reference, literal, regular expression, perform auto-completion.

The GNU Global official website is https://www.gnu.org/software/global.

You'll need GTags binaries for Windows (Win32 API) to use this plugin. Those are supplied with the plugin binary for convenience.
You can also find Windows ported GTags binaries at http://adoxa.altervista.org/global.

The Universal Ctags parser project website is https://ctags.io.


**Build Status**
======================

AppVeyor [![Build status](https://ci.appveyor.com/api/projects/status/b4aam50a4q2vacd7?svg=true)](https://ci.appveyor.com/project/pnedev/nppgtags)


**Installation**
======================

For Notepad++ versions 7.6.3 and above you can use the built-in PluginAdmin dialog (accessible through the *Plugins* menu).

You can manually install the plugin by copying `NppGTags.dll`, `bin` folder (containing GTags and Universal Ctags binaries) and `share` folder (containing `pygments parser.py` script) to your Notepad++ plugins directory in a separate sub-folder named `NppGTags`.
For Notepad++ versions below 7.6, the above files should be copied directly in Notepad++ plugins directory.
Restart Notepad++ and you are all set.


**Settings**
======================

Use plugin **Settings** to tune its operation.
There are two types of settings - 'generic' plugin settings and database related settings.

Among the 'generic' settings are:

- The default database - if enabled it will be used to perform searches from active files (documents) in Notepad++ that don't have their own database (unparsed files). This is kind-of library database for unparsed files.

The database related settings come in two distinct copies that are identical.
One is regarding the settings default values for each newly generated database. You can access those at any time - just open the **Settings** window.
The second one is the database settings for the currently used database (determined by the currently active document - somewhere in its path the database files reside). You can access those when you open **Settings** window while you are editing a project file.

Among the database related settings are:

- The code parser to be used while creating the database.
The default is the built-in *GTags* parser but it supports only C, C++, Java, PHP and several other languages at the time this doc was written.
*Ctags* (this is actually the Universal Ctags parser) supports considerably more languages and is continuously evolving but does not allow reference search.
*Pygments* also supports lots of languages + reference search but requires external Python library (*Pygments*) that is not supplied with the plugin (as you need to have Python installed and *Pygments* library added for it via *pip* for example).

From **Settings** you can also set the auto-update database behavior, the linked libraries databases (if any) and the ignored sub-paths. The linked libraries are completely manageable from the settings window (meaning they can be created and updated directly from there). The ignored sub-paths setting is used for results filtering - the configured database sub-paths will actually be searched as well but will be excluded from the search results.


**Usage**
======================

You can find all supported commands in the NppGTags plugin menu.
To make your life easier use Notepad++'s shortcut settings to assign whatever shortcuts you like to the plugin commands. There are no predefined shortcuts in the plugin to avoid possible conflicts with other Notepad++ plugins.

To start using the plugin first you need to create GTags database for your project - **Create Database**.
In the dialog simply select your project's top folder and GTags will index recursively all files supported by the chosen parser. It will create database files (*GTAGS*, *GRTAGS*, *GPATH* and *NppGTags.cfg*) in the selected folder.

**Delete Database** invoked when a file from the project is the currently active document in Notepad++ will delete the above-mentioned files.

If you run one of the plugin's **Find** commands (those include the **Search** commands as well) from any active document from the project it will search the database for:

1. what you have selected if there is selection;
2. the word that is under the caret if there is no selection;
3. what you have entered in the search box that will appear if there is no word under the caret.

The search box allows choosing case sensitivity and regexp for the **Find** command where applicable.
It also provides search completion list where possible which appears when you enter several characters in the box.
If the **Find** command is started without the search box (case 1 and 2) the search is literal (no regexp) and the case sensitivity depends on the menu flag **Ignore Case**.

**Find File** command will skip step 2, it will directly go to step 3 if there is no selection.
It will automatically fill the search box (from step 3) with the name of the current file without the extension to make switching between source and header file easier.
**Find File** will search for paths containing the given string (anywhere in the name) although its search completion will show only paths that start with the entered string. That means that even if there is no completion shown you can still perform the search and find files containing the entered string.
All paths are relative to the project's directory.

In GTags' terminology, *Symbol* is reference to identifier for which definition was not found. All local variables are symbols for example.
If you search for *Definition* / *Reference* and GTags doesn't find anything the plugin will automatically invoke search for *Symbol*. This will be reported in the search results window header.

Any **Search** will search for a string either "literally" (not using regular expressions) or using regular expressions if that is selected through the search box options.
**Search in Source Files** will look only in files that are recognized as sources by the parser during database creation.
**Search in Other Files** respectively will look in all other (non-binary) files. This is useful to dig into documentation (text files) or Makefiles for example.

As a summary, **Find Definition / Reference** will search for identifiers (single whole words) whereas **Search...** will search for strings in general (parts of words, several consecutive words, etc.) either literally or using regular expressions.

**AutoComplete** will show found *Definitions* + found *Symbols*. It will look for the string from the beginning of the word to the caret position.
Autocomplete case sensitivity also depends on the menu flag **Ignore Case**.

While auto complete results window is active you can narrow the results shown by continuing typing.
*Backspace* will undo the narrowing one step at a time (as the newly typed characters are deleted).
Double-clicking or pressing *Enter* or *Tab* will insert the selected auto complete result.

**AutoComplete File Name** is useful if you will be including headers for example.

**AutoComplete** and **Find Definition** commands will also search library databases if such are used. That is configured per database through the plugin's **Settings** window.

All **Find** commands will show Notepad++ docking window with the results. The exception to this is if the result is just one - then you will be taken directly to its location. Otherwise the command results will be placed in a separate tab that will automatically become active (the docking window will receive focus).
Clicking on another tab will show that command's results. You can also use the *ALT* + *Left* and *ALT* + *Right* arrow keys to switch between tabs.

Double-clicking, hitting *Space* or *Enter* on search result line will take you to the source location. You can also do that by left-clicking on the highlighted searched word in the result line. Your currently edited document location will be saved - use **Go Back** command to visit it again. You can 'undo' the **Go Back** action by using **Go Forward** command.
By using the mouse or the arrow keys you can move around result lines and you can trigger new searches directly from the results window
(same rules about the searched string apply).

Right clicking or hitting *ESC* will close the currently active search results tab.

Left-clicking on the margin area ([+] / [-] signs) or pressing *'+'* / *'-'* keys will unfold / fold lines. To fold a line it is not necessary to click exactly the [-] sign in the margin - clicking on any sub-line's margin will do. Pressing *ALT* + *'+'* / *'-'* keys will unfold / fold all lines.
You can accomplish that also by double-clicking or hitting *Space* or *Enter* on the search results header line - this will toggle all lines fold / unfold state.
Clicking on header line margin or pressing *'+'* / *'-'* keys while head line is the active one will do the same.

The results window is Scintilla window actually (same as Notepad++). This means that you can use *CTRL* + mouse scroll to zoom in / out or you can select text and copy it (*CTRL* + *'C'*).

When the focus is on the results window pressing *CTRL* + *'F'* will open a search dialog. Fill-in what you are looking for and press *Enter*. The search dialog will remain open until you press *ESC*. While it is open you can continue searching by pressing *Enter* again. *Shift* + *Enter* searches backwards. If you close the search dialog you can continue searching for the same thing using *F3* and *Shift* + *F3* (forward or backward respectively). *F3* works while the search dialog is open as well. The search always wraps around when it reaches the results end - the Notepad++ window will blink to notify you in that case.

When the focus is on the results window pressing *F5* will re-run the same search as the active results tab. This is kind-of active results tab refresh.

**Toggle Windows Focus** command is added for convenience. It switches the focus between the edited document and the currently opened NppGTags windows (results window and search window). It's meant to be used with a shortcut so you can use the plugin through the keyboard entirely.

Enjoy!
