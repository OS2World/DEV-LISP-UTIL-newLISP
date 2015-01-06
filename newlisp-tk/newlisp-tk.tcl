#!/bin/sh
# the next line restarts using wish \
exec wish "$0" "$@"

#
# newLISP-TK a GUI frontend for newLISP <http://www.newlisp.org>
#
#
# You need Tcl/Tk 8.3 or 8.4 installed to run this program.
# under CYGWIN is works also with 8.0.
#
# newLISP v.8.0.0 or later must be in the execution path
# so this program can find it and start it.
# Under Windows you need newLISP compiled under CYGWIN or Win32
#
# after startup the copyright message should appear in the console
# window, this shows that the GUI could start newLISP and connect
# to it. If it doen's come up, check if newlisp is already running
# (from a previous attempt) and destroy the process (kill -9 process-id).

# The process-id can be obtained by doing a 'ps' from the command line.
#
# In Windows check the taskmaeneger for occurrences of 'newlisp' and
# 'newlisp-tk' to abort
# 
# then try the following program from newLISP, to see how to drive TK
# from newLISP
#
#
#(define (tk-callback x) 
#   (silent (tk ".tw.lb config -text " x)))

#(define (tk-example ) 
#  (tk "toplevel .tw")
#  (tk "button .tw.tb -text Push -command {newlisp {(tk-callback \"{Hello World}\")}}")
#  (tk "label .tw.lb -width 20 -bg white")
#  (tk "pack .tw.tb .tw.lb -side top -padx 10 -pady 10"))

# (tk-example)#
#
#
# CHANGES
# 0.71 - supress symbol 'true' for showing up in variable list box
# 0.72 - let newLISP load/process all files in commandline
#        change windows c:\tmp to c:\temp, which is more usual in Windows
# 0.73 - different TCLTKreader for Windows and Linux for great speedup
#        of graphics performance under Linux, no delay anymore when
#        pushing debugger buttons
# 0.74 - the TCLTKreader for Linux is now the reader for both Windows and Linux
#        because some NT4.0 systems work better with the Linux reader
#     
# 0.75 - on LINUX button images are now stored in /usr/share/newlisp/newlisp-tk/images
#
# 0.76 - change in ExitProc
#
# 0.80 - inclusion of BWidget widget set - see http://tcllib.sourceforge.net
#
# 0.81 - balloon help for all icons instead of help in status bar
#      - added (tk-args) and for retrieving commandline args from newlisp-tk
#      - made 'result' local in newLISP 'tk' function
#
# 0.91 - now can serve a newLISP running on a remote host and ports specified
#        in: $Ide(newLISPhost), $Ide(newLISPport) and $Ide(newLISPbufferPort)
#      - shortcut for Manual now Ctrl-M, Ctrl-R is now reload of last loaded file
#      - changed defintion of newLISP 'tk' to be able to work with Win32 versions
#        which cannot do socket I/O on raw file handles like UNIX can
# 0.92 - released
#
# 0.93 - the filextension in file dialogs can now be configured in newlisp-tk.config
# 0.94 - fixed bug in tk: result -> _result
# 0.95 - changes for new sntax og 'context'
# 0.96 - replaces 'concat' with 'append' in lisp code
#
# 1.00 - big speed up for Linux with change in NewLISPreader(), 'tk' command can
#        can now process mutiline statements
#       
# 1.01 - fixed bug for handling $1 $2 etc variables in frontend (now filtered out)
#
# 1.02 - on Linux the tmp directory is now looked for in the home directory
#        of the user starting newlisp-tk, the old way using /tmp caused
#        problems in multiuser environments
#
# 1.03 - globalized 'tk' and 'net-read-line', redefinition of 'exit'
#        didn't find tmp directory when changing dir while loading files
#        now tmp is looked for in the HOME directory when  not in Windows
#	 BWidgets get automatically loaded when in Windows
#
# 1.04 - tk-args was not defined correctly (praenthesis in wrong place)
#
# 1.05 - windows X and Y position now saved when saving settings (Hans-Peter W.)
#
# 1.06 - eliminated usage of newLISPbufferPort, also now unlimited size in
#        NewlispEvaluateBuffer() for evaluation of editor browser contents
#        (requires newLISP v 7.3.5 or later)
#
# 1.07 - doc directopry changed to /usr/share/doc/newlisp
#
# 1.08 - replaces 'remove' with 'replace'
#
# 1.09 - eliminated usage of temp directory using new 'source' in 7.5.4
#
# 1.10 - avoid opening debgugger and codebrowsers together with warning
#        message
#
# 1.11 - changed help to load framed version of the manual
#
# 1.12 - load first font from menu if 'Fixedsys' or 'Lucida
#        Console'  or 'fixed' (Unix) cannot be found
#        newlisp-tl.config now correctly save in users home on Unix
#
# 1.13 - changed documentation directory to /usr/share/newlisp/doc for IDE
#        /usr/share/doc/newlisp-x.x.x exists too but is not referenced
#        from this application
#
# 1.14 - added configuration variable for initial ditrectory in load file
#        dialog
# 1.15 - fix in (ctxts) function accounting for context vars
#
# 1.16 - suppresses display of all vars starting with '$', because of
#        incompatibility with Tcl/Tk
#
# 1.17 - new config variable  for alternate path to newLISP executable
#
# 1.18 - remember last loaded directory
#
# 1.19 - added configuration variable for newLISP app to execute
#
# 1.20 - changed 'tk' macro to function
#
# 1.21 - all references changed for new Win32 install location: $PROGRAMFILES/newlisp
#
# 1.22 - fixed newLISPbin path for Win32, backslashes get translated to fwd slashes
#        when saving settings
#
# 1.24 - misc cleanup and suppression of display of illegal symbols in browser
#
# 1.25 - Darwin OSX TCL tripped over lastLoadedFile ""
#
# 1.26 - UTF-8 filenames and symbols treated correctly
#
# 1.27 - improved colors for Mac OSX
#
# 1.28 - misspelled initialDir in config
# 1.29 - catch load errors in commandline
# 1.30 - replaced 'symbol' wioth shorter 'sym'
#
# 1.31 - added command help when selecting function in context menu and F1 on Win32
# 1.32 - help buttons now work on MacOS X
# 1.33 - help context menu now works on MacOS X
# 1.34 - handle question mark handling in command help handling
# 1.35 - double click did not highlight expression in console window
# 1.36 - sym was used as variable in make-context-verify
# 1.37 - doc directoy changed to /usr/share/doc/newlisp

set IDEversion "newLISP-tk v.1.37"


#
# Copyright (C) 1992-2006 Lutz Mueller <lutz@nuevatec.com>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, 1991,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

# load BWidget package - this is done automatically for windows but else must
# outcomment first and fource following lines if you have the BWdiget set installed
# in the right path.

#lappend auto_path "/bwidget"
#package require BWidget

# default properties
# can be changed for personal preference editing file newlisp-tk.config
# if the file doesn't exist following settings apply

set Ide(imageDir)           "/usr/share/newlisp/newlisp-tk/images"
set Ide(fontName)           "fixed"
set Ide(fontSize)           14

set Ide(consoleWidth)       80
set Ide(consoleHeight)      30
set Ide(consoleBackground)  white
set Ide(consoleForeground)   navy

set Ide(editorWidth)        65
set Ide(editorHeight)       30
set Ide(editorBackground)   white
set Ide(editorForeground)    navy

set Ide(debuggerWidth)        65
set Ide(debuggerHeight)       24
set Ide(debuggerBackground)   white
set Ide(debuggerForeground)    navy

set Ide(maxHistory)         50
set Ide(newLISPport)        64001
set Ide(newLISPhost)        "127.0.0.1"
set Ide(TCLTKport)          64002
set Ide(TCLTKhost)          "127.0.0.1"
set Ide(HelpProgram)        "mozilla"
set Ide(HelpFile)           "/usr/share/doc/newlisp/newlisp_manual.html"
set Ide(HelpTopic)          "/usr/share/doc/newlisp/manual_frame.html"
set Ide(HelpTopic-tk)       "/usr/share/doc/newlisp/newlisp-tk.html"
set Ide(initCommand)        ""
set Ide(newLISPapp)         ""
set Ide(lispFileExtension)   ".lsp"
set Ide(WinPosX)		    "100"
set Ide(WinPosY)		    "100"
set Ide(initialDir)         "/usr/share/newlisp/newlisp-tk/"

# overwrite platform dependant properties, which can be overwritten by newlisp-tk.config

# location of newlisp.exe

set Ide(platform) $tcl_platform(platform)

if { $tcl_platform(platform) == "unix" } {
    if { [exec uname] == "Darwin" } {
        set Ide(HelpProgram) "open"
        set Ide(HelpFile) "file:///usr/share/doc/newlisp/newlisp_manual.html"
        }
   }

if { $tcl_platform(platform) == "windows" } {
	set Ide(fontName) "Fixedsys"
	set Ide(platform) "windows"
	set Ide(fontSize) 10
	set Ide(imageDir) "/freewrap/images"
	set Ide(HelpProgram) "$env(PROGRAMFILES)/Internet Explorer/IEXPLORE.EXE"
	set Ide(HelpFile) "$env(PROGRAMFILES)/newlisp/newlisp_manual.html"
	set Ide(HelpTopic) "$env(PROGRAMFILES)/newlisp/manual_frame.html"
	set Ide(HelpTopic-tk)  "$env(PROGRAMFILES)/newlisp/newlisp-tk.html"
	set Ide(initialDir) "$env(PROGRAMFILES)/newlisp"
	set Ide(newLISPbin) "$env(PROGRAMFILES)/newlisp"
	lappend auto_path "/bwidget"
	package require BWidget
	} else { 
	set Ide(Home) $env(HOME)
	set Ide(newLISPbin) "/usr/bin"
	}


# global variables

set temp ""                       ;# name of temporary directory
set selectionBuff ""              ;# copy/cut/paste selection
set statusText ""                 ;# contents of status window on bottom of console
set lastCommand " "               ;# last command issued to console
set newLISPpid 0                  ;# process id of newLISP process
set nlio 0                        ;# I/O channel socket for communications <-> newLISP server
set TCLKsock 0                    ;# I/O chanbel socket for communications <-> TCLTK server
set TCLTKlisten 0                 ;# listen socket for TCLTK
set txt ""                        ;# console text windows
set cmdStart 0                    ;# mark for txt start of command
set historyIdx 0                  ;# current index into history
set historyEnd 0                  ;# last entry in history
set commandHistory(0) " "         ;# command history
set newLISPpid 0                  ;# process id pid of newLISP backend process
set browserCount 0                ;# number of active browsers
set debugIsOn 0                   ;# flag to show if debugger is on
set debugOutput ""                ;# newLISP output to Debugger
set debugInOutput 0               ;# debugger receiving debug output
set editboxFillIsOn 0             ;# editbox is receiveing data
set browserCurrent ""             ;# current selected browser/editor for editbox fill
set lastLoadedFile "."            ;# last file loaded with menu option 'Load ...'
set remoteFlag 0                  ;# '1' if in remote mode else '0'
set tkCommand ""

############################# setup main console window ########################################


proc SetupConsole {} {

	global Ide IDEversion txt FontSizeMenu FontNameMenu commandHistory

	wm title . $IDEversion

	set txt [text .console ]

	set buttonFrame [SetupConsoleToolbar $txt]

	$txt config -fg $Ide(consoleForeground) -bg $Ide(consoleBackground) 
	$txt config -width $Ide(consoleWidth) -height $Ide(consoleHeight) -setgrid true	
	$txt config -yscrollcommand { .scroll set }
	$txt config -insertofftime 0 -insertwidth 4
	$txt config -exportselection true
	$txt config -highlightcolor lightgrey
	scrollbar .scroll -command {$txt yview }

	grid $buttonFrame -row 0 -columnspan 2 -sticky ew
	grid $txt -row 1 -column 0 -sticky news
	grid .scroll -row 1 -column 1 -sticky news

	# setup main window menu

	menu .menubar
	. config -menu .menubar
	foreach m {File Options Help} {
		set $m [menu .menubar.m$m -tearoff 0]
		.menubar add cascade -label $m -menu .menubar.m$m
		}

	.menubar insert 2 command -label Edit -command CodeBrowser
	.menubar insert 3 command -label Clear -command ClearConsoleScreen
	.menubar insert 4 command -label Debug -command {Debugger}



	$File add command -label {Load ...} -accelerator {Ctrl-O} -underline 1 -command LoadFile
	$File add command -label {Reload} -accelerator {Ctrl-R} -underline 0 -command ReLoadFile
	$File add command -label {Save All as ...} -accelerator {Ctrl-S} -underline 0 -command SaveAllFile
	$File add separator
	$File add command -label Exit -underline 1 -command exit

	set FontSizeMenu [menu .menubar.mOptions.sFontSize -tearoff 0]
	set FontNameMenu [menu .menubar.mOptions.sFontName -tearoff 0]

	$Options add cascade -label "Font Size" -underline 3 -menu $FontSizeMenu 
	$Options add cascade -label "Font Name" -underline 5 -menu $FontNameMenu 
	$Options add command -label {Background ...} -underline 0 -command "SetConsoleColors consoleBackground"
	$Options add command -label {Foreground ...} -underline 0 -command "SetConsoleColors consoleForeground"
	$Options add separator
	$Options add command -label {Save settings} -underline 0 -command "SaveSettings"


	$Help add command -label {Manual & Reference} -underline 0 -command HelpAction
	$Help add command -label {newLISP-tk} -underline 9 -command HelpAction-tk
	$Help add separator
	$Help add command -label About -underline 0 -command AboutBox

	MakeFontSizeMenu $FontSizeMenu
	MakeFontNameMenu $FontNameMenu

	ResetFont fontSize $Ide(fontSize) $FontSizeMenu
	ResetFont fontName $Ide(fontName) $FontNameMenu

	# setup console popup menu

	set consolePopup [menu .console.popup]
	SetupPopupMenu $consolePopup

	# setup status bar

	frame .statusBar
	label .statusBar.label -textvariable statusText -relief sunken -bd 1 -font "Helvetica 10" -anchor w
	grid .statusBar.label -sticky ew
	grid columnconfigure .statusBar 0 -weight 1
	grid .statusBar -row 2 -column 0 -columnspan 2 -sticky ew

	grid rowconfigure . 1 -weight 1
	grid columnconfigure . 0 -weight 1

	# setup marks and key handling in console window 

	$txt mark set cmdStart insert
	$txt mark gravity cmdStart left

	# initialize command history

	for {set i 1 } {$i <= $Ide(maxHistory)} {incr i} {
		set commandHistory($i) "" }

	bind $txt <Return> { ProcessConsoleInput; break } 
	bind $txt <Key-Up> { PutPriorCommand ; break }
	bind $txt <Key-Down> { PutNextCommand ; break }
	bind $txt <Home> { $txt mark set insert cmdStart; break }
	bind $txt <Double-1> {HighlightExpression %W; break }
	bind $txt <Tab> { $txt insert insert "  "; break }
	bind $txt <Control-l> { ClearConsoleScreen; break }
	bind $txt <Control-a> { $txt mark set insert cmdStart; break }

	bind $txt <Shift-KeyPress> {if { "%A" == "("} { EditShowMatchingPar %W 0 } else {
				if { "%A" == ")"} { EditShowMatchingPar %W 1 }}}

	bind $txt <Button-3> "tk_popup $consolePopup %X %Y 0"
	bind $txt <Control-Button-1> "tk_popup $consolePopup %X %Y 0"

	bind $txt <Control-x> { CutCopyText %W }
	bind $txt <Control-c> { CutCopyText %W }
	bind $txt <Control-v> { PasteText %W }
	bind $txt <Control-o> LoadFile
	bind $txt <Control-r> ReLoadFile
	bind $txt <Control-s> SaveAllFile
	bind $txt <Control-b> {CodeBrowser; break}
	bind $txt <Control-g> Debugger
	bind $txt <Control-m> HelpAction
	bind $txt <Control-k> HelpAction-tk

	if { $Ide(platform) == "windows" } { 
		bind $txt <F1> { CommandHelp %W } 
		} 

	bind . <Destroy> {if {"%W" == "."} {ExitProc}}

	wm geometry . +$Ide(WinPosX)\+$Ide(WinPosY)

	focus -force $txt
	}


# set up a icon toolbar for console menu choices

proc 	SetupConsoleToolbar { widget } {

	global Ide

	frame .bframe

	foreach name {openImg saveImg reloadImg editImg debugImg copyImg cutImg pasteImg clearImg 
				helpImg nltkImg } {
		[set $name [image create photo]] read "$Ide(imageDir)/$name.gif"
		}

	foreach no {1 2 3 4 5} {
		label .bframe.vertical$no -text {   } -relief flat
		}

	set st groove


	button .bframe.open -image $openImg -relief $st -command LoadFile
	button .bframe.reload -image $reloadImg -relief $st -command ReLoadFile
	button .bframe.save -image $saveImg -relief $st -command SaveAllFile
	button .bframe.edit -image $editImg -relief $st -command CodeBrowser
	button .bframe.debug -image $debugImg -relief $st -command Debugger
	button .bframe.copy -image $copyImg -relief $st -command "event generate $widget <Control-c>"
	button .bframe.cut -image $cutImg -relief $st -command "event generate $widget <Control-x>"
	button .bframe.paste -image $pasteImg -relief $st -command "event generate $widget <Control-v>"
	button .bframe.clear -image $clearImg -relief $st -command "ClearConsoleScreen"
	button .bframe.help -image $helpImg -relief $st -command HelpAction
	button .bframe.nltk -image $nltkImg -relief $st -command HelpAction-tk

	pack .bframe.open .bframe.reload .bframe.save .bframe.vertical1 -side left 
	pack .bframe.edit .bframe.debug .bframe.vertical2 -side left 
	pack .bframe.copy .bframe.cut .bframe.paste .bframe.vertical3 -side left 
	pack .bframe.clear .bframe.vertical5 -side left -padx 1
	pack .bframe.help .bframe.nltk -side left  

	balloon_help .bframe.open { Load source   Ctrl-O }
	balloon_help .bframe.reload { Reload last source Ctrl-R }
	balloon_help .bframe.save { Save workspace   Ctrl-S }
	balloon_help .bframe.edit { Browser / Editor   Ctrl-B }
	balloon_help .bframe.debug { Debugger   Ctrl-G }
	balloon_help .bframe.copy { Copy selection   Ctrl-C }
	balloon_help .bframe.cut { Cut selection   Ctrl-X }
	balloon_help .bframe.paste { Paste selection   Ctrl-V }
	balloon_help .bframe.clear { Clear console   Ctrl-L }
	balloon_help .bframe.help { newLISP Reference   Ctrl-M }
	balloon_help .bframe.nltk { newLISP-tk Intro   Ctrl-K }
	
	return .bframe
	}


proc balloon_help {w msg} {
        bind $w <Enter> "after 600 \"balloon_aux %W [list $msg]\""
        bind $w <Leave> "after cancel \"balloon_aux %W [list $msg]\"
                         after 100 {catch {destroy .balloon_help}}"
    }


proc balloon_aux {w msg} {
        if { [winfo exists $w] != 1 } { return }
        set t .balloon_help
        catch {destroy $t}
        toplevel $t
        wm overrideredirect $t 1
        if {$::tcl_platform(platform) == "macintosh"} {
         unsupported1 style $t floating sideTitlebar
        }
        pack [label $t.l -text $msg -relief solid -bd 1 -bg #FFFFE0 -padx 2 -pady 2] -fill both
        set x [expr [winfo rootx $w]+[winfo width $w]/2]
        set y [expr [winfo rooty $w]+16+[winfo height $w]/2]
        wm geometry $t +$x\+$y
        bind $t <Enter> {after cancel {catch {destroy .balloon_help}}}
        bind $t <Leave> "catch {destroy .balloon_help}"
    }



proc MakeFontSizeMenu { menuName } {
	foreach size { 6 8 9 10 11 12 13 14 15 16 18 22 24 32 48 } {
		$menuName add command -label $size -command "ResetFont fontSize $size $menuName" 
		}
	}


proc MakeFontNameMenu { menuName } {
	foreach name [lsort [font families]] {
		$menuName add command -label $name -command "ResetFont fontName \"$name\" $menuName"
		}
	}


proc GotoWindow { window } {
	focus -force $window
	if {$window == ".console"} {raise .} else {raise $window}
	}


################################# general selection utilities ###########################

proc SetupPopupMenu { widget } {

	$widget add command -label Copy -accelerator Ctrl-C -command "event generate $widget <Control-c>"
	$widget add command -label Cut -accelerator Ctrl-X -command "event generate $widget <Control-x>"
	$widget add command -label Paste -accelerator Ctrl-V -command "event generate $widget <Control-v>"
	$widget add command -label Eval -command "EvaluateText $widget"
	$widget add command -label EvalPrint -command "EvaluatePrintText $widget"
	$widget add command -label Help -command "CommandHelp $widget"
	$widget config -tearoff 0
	return $widget
	}

proc CutCopyText { widget } {
	global selectionBuff txt Ide

	if { $Ide(platform) == "windows"} { return }

	catch [set selectionBuff [selection get -displayof $widget ]]
	}


proc PasteText { widget } {
	global selectionBuff Ide

	if { $Ide(platform) == "windows"} { return }

	$widget insert insert [selection get -selection CLIPBOARD]
	}


proc EvaluateText { widget } {

	set buff ""

	catch [set buff [selection get -displayof $widget ]]

	if {[string first "\n" $buff] != -1} {
		tk_messageBox -message "no multi line evaluations\nare allowed from selections"
		}
	
	NewlispEvaluate "(silent [string trim $buff])"	
	}


proc EvaluatePrintText { widget } {
	
	set buff ""

	catch [set buff [selection get -displayof $widget ]]

	if {[string first "\n" $buff] != -1} {
		tk_messageBox -message "no multi line evaluations\nare allowed from selections"
		}
	
	NewlispEvaluate "[string trim $buff]"
	}
	
proc CommandHelp { widget } {

	global selectionBuff Ide

	catch [set selectionBuff [selection get -displayof $widget ]]
	regsub -all {\?} $selectionBuff {p} safeselectionBuff

	exec $Ide(HelpProgram) "file://$Ide(HelpFile)#$safeselectionBuff" &
} 

 
################################ console utility functions ###############################

# exit the program

proc ExitProc {} {

	global nlio TCLTKsock TCLTKlisten newLISPpid
	catch { close $nlio }
	catch { close TCLTKsock }
	catch { close TCLTKlisten }

	catch { exec kill $newLISPpid }
	}


proc ClearConsoleScreen {} {
.console delete 1.0 end
NewlispEvaluate ""

}


# access command history

proc PutPriorCommand {} {
	global txt cmdStart commandHistory historyIdx Ide

	$txt delete cmdStart end
	$txt insert end $commandHistory($historyIdx)

	if { $historyIdx == 0 } {
		set historyIdx $Ide(maxHistory)
		while { $commandHistory($historyIdx) == "" } { 
			incr historyIdx -1 
			}
		} else { incr historyIdx -1 }
	}


proc PutNextCommand {} {
	global txt cmdStart commandHistory historyIdx Ide

	if { $historyIdx == $Ide(maxHistory) } { 
		set historyIdx 0 } else { incr historyIdx }
	
	while { $commandHistory($historyIdx) == "" } {
		if {$historyIdx == $Ide(maxHistory)} { 
			set historyIdx 0 } else { incr historyIdx }
		}
		
	$txt delete cmdStart end
	$txt insert end $commandHistory($historyIdx)
	}


# after hitting <Return> send command to newLISP

proc ProcessConsoleInput {} {

	global txt nlio cmdStart commandHistory historyIdx historyEnd Ide

	$txt mark set insert end
	set cmd [$txt get cmdStart end]

	set lastCommand [string trim $cmd]

	if { $commandHistory($historyEnd) != $lastCommand && $lastCommand != ""} {

		if { $historyEnd == $Ide(maxHistory) } {
			set historyEnd 0
			} else { incr historyEnd } 

	
		set commandHistory($historyEnd) $lastCommand
		}
	set historyIdx $historyEnd

	$txt insert end "\n"
	$txt mark set cmdStart insert
	$txt see cmdStart

	.console config -cursor watch
	update

	puts $nlio $lastCommand
	flush $nlio
	}


# set font name and size in console
#	catch [set idx [$menu index "*$Ide($property)*"]]

proc ResetFont { property value menu } {
	global txt Ide statusText

	set idx 0
        if [ catch {$menu index "*$Ide($property)*"} idx ] {
		set value ""
		set idx 0
		}

	$menu delete $idx
	$menu insert $idx command -label $Ide($property) -foreground black \
		-command "ResetFont $property \"$Ide($property)\" $menu"

	set Ide($property) "$value"

	set idx 0
	catch [set idx [$menu index "*$value*"]]


	$menu delete $idx
	$menu insert $idx command -label $value -foreground yellow -background navy \
		-command "ResetFont $property \"$value\" $menu"

	set spec [concat \"$Ide(fontName)\" $Ide(fontSize)] ;# protect against spaces in font names

	$txt config -font $spec -fg $Ide(consoleForeground)

	set statusText "Console font set to $Ide(fontName) $Ide(fontSize)"
	}


# set console background and font colors

proc SetConsoleColors { property } {

	global Ide txt

	set color [tk_chooseColor -initialcolor $Ide($property) -title "Set $property color"]

	if {$color == ""} return

	set Ide($property) $color

	$txt config -bg $Ide(consoleBackground) -fg $Ide(consoleForeground)
	}


# save options settings to file newlisp-tk.config

proc SaveSettings {} {

	global Ide txt statusText

   	set Ide(WinPosX) [winfo x .]
   	set Ide(WinPosY) [winfo y .]

	if { $Ide(platform) == "windows" } {
		set initFile [open "$Ide(newLISPbin)/newlisp-tk.config" w] } else {
		set initFile [open "$Ide(Home)/newlisp-tk.config" w] }

	puts $initFile "# newlisp-tk.config - newLISP Tcl/Tk configuration file"
	puts $initFile "#"
	puts $initFile {# This file is generated by menu "Options/Save settings"}
	puts $initFile "#\n"

	foreach idx [lsort [array names Ide]] {
#		set Ide($idx) [regsub {:\\} $Ide($idx) {:/}]
		puts $initFile "set Ide\($idx\) \{$Ide($idx)\}"
		}

	close $initFile

	set statusText {Saved settings in newlisp-tk.config}
	}


# get value for enviroment variable

proc GetEnv { name } {
	global env

	set elmnts [array get env]

	for {set i 1 } {$i < [llength $elmnts]} {incr i} {
		if {[lindex $elmnts $i] == $name} {
			return [lindex $elmnts [expr $i + 1]]
			}
		}
	return ""
	}


# Load/Save a newLISP source file  with the file dialog

proc FileDialog { type title parent } {

	toplevel .filedialog
	label .filedialog.label -text "Enter a file name or hit <Esc>"
	text .filedialog.edit -width 64 -height 1
	pack .filedialog.label .filedialog.edit -side top -padx 10 -pady 5

	if { $type == "load" } {
		bind .filedialog.edit <Return> { LoadFileRemote; break }
		}
	if { $type == "save" } {
		bind .filedialog.edit <Return> { SaveAllFileRemote; break }
		}
	if { $type == "context" } {
		bind .filedialog.edit <Return> { SaveContextAsRemote; break }
		}


	bind .filedialog.edit <Escape> "destroy .filedialog"

	wm title .filedialog $title
	wm transient .filedialog $parent

	wm geometry .filedialog \
		+[expr [winfo x $parent] + 120]+[expr [winfo y $parent] + 120]

	GotoWindow .filedialog.edit
	}


proc LoadFileRemote {} {
	global statusText lastLoadedFile

	set spec [string trim [.filedialog.edit get 1.0 end]]

	if {$spec == ""} {destroy .filedialog; return}

	NewlispEvaluate "(silent (load {$spec}))"

	destroy .filedialog
	set lastLoadedFile $spec
	set statusText "Loaded file $spec"
	}


proc LoadFile {} {

	global statusText FileTypeList lastLoadedFile remoteFlag
	
	if { [set idx [string last "/" $lastLoadedFile]] != -1} {
		set loadDir [string range $lastLoadedFile 0 $idx] 
	} else { set loadDir $lastLoadedFile }
		
	if {$remoteFlag == 0 } {
		set spec [tk_getOpenFile -title "Load a newLISP source file" \
			-filetypes $FileTypeList -initialdir $loadDir]
	} else { FileDialog "load" "Load a newLISP source file" . ; return }
		

	if { $spec == "" } { return }

	set cd [file dirname $spec]

        NewlispEvaluate "(silent (load {$spec}) (change-dir {$cd}))"

        set lastLoadedFile $spec
	set statusText "Loaded file $spec"
	}


proc ReLoadFile {} {
    global lastLoadedFile statusText

    if {$lastLoadedFile == ""} { return }

    NewlispEvaluate "(silent (load \"$lastLoadedFile\"))"
    set statusText "Reloaded file $lastLoadedFile"
    }



proc SaveAllFileRemote {} {
	global statusText

	set spec [string trim [.filedialog.edit get 1.0 end]]

	if {$spec == ""} {destroy .filedialog; return}

	NewlispEvaluate "(silent (save {$spec}))"

	destroy .filedialog

	set statusText "Saved all as $spec"
	}


proc SaveAllFile { } {

	global statusText FileTypeList remoteFlag

	if {$remoteFlag == 0 } {
	  set spec [tk_getSaveFile -initialfile save.lsp -title "Save all as ..." -filetypes $FileTypeList]
	} else { FileDialog "save" "Save all as ..." . ; return }

	if { $spec == "" } { return }

	set cd [file dirname $spec]

	.console config -cursor watch; update
	catch {file delete $spec}

        NewlispEvaluate "(silent (SYS:save-all-file {$cd} {$spec}))"
	}


proc SaveAllFileFinish { spec } {
    global statusText

    .console config -cursor xterm; update
    set statusText "Saved all as $spec"
    }


# Help menu

proc HelpAction {} {
	global Ide

	exec $Ide(HelpProgram) $Ide(HelpTopic) &
	}

proc HelpAction-tk {} {
	global Ide

	exec $Ide(HelpProgram) $Ide(HelpTopic-tk) &
	}


proc AboutBox {} {

	global IDEversion Ide

	set bg #f0f0f0

	toplevel .about  
	frame .about.f -background $bg -borderwidth 3 -relief ridge
	label .about.f.version -text "$IDEversion" -background $bg -pady 20
	label .about.f.copy -text "copyright (c) 2006, Lutz Mueller" -background $bg -padx 52
	label .about.f.rights -text "All rights reserved." -background $bg
	label .about.f.url -text "http://www.newlisp.org" -background $bg -pady 20 -fg navy

	pack .about.f -side top
	pack .about.f.version .about.f.copy .about.f.rights .about.f.url -side top

	.about.f.version config -font {Times 18} -fg #D00040
	
	wm title .about "$IDEversion"
	wm transient .about .console

	wm geometry .about +[expr [winfo rootx .console] + 100]+[expr [winfo rooty .console] + 60]

	bind .about <Escape> "destroy .about"
	focus .about
	}


################################ set up Code Browser/Editor ###############################


proc ScrollSet {scrollbar geoCmd offset size} {
	if {$offset != 0.0 || $size != 1.0} { eval $geoCmd }
	$scrollbar set $offset $size
	}


proc ScrolledListbox { f args} {
	frame $f

	listbox $f.list -yscrollcommand \
		[list ScrollSet $f.scroll [list grid $f.scroll -row 0 -column 1 -sticky ns]]

	$f.list config -relief raised

	eval {$f.list configure} $args

	scrollbar $f.scroll -orient vertical -command [list $f.list yview]
	grid $f.list -sticky news
	grid rowconfigure $f 0 -weight 1
	grid columnconfigure $f 0 -weight 1
	
	return $f
	}


proc CodeBrowser {} {

	global browserCount Ide remoteFlag debugIsOn 

	if {$debugIsOn == 1} {
		tk_messageBox -message "Close debugger first!";
		return;
		}
 
	incr browserCount

	set br [toplevel .browser$browserCount]

	set buttonFrame [SetupEditorToolbar $br]

	menu $br.menubar
	$br config -menu $br.menubar

	foreach m {File Options} {
		set $m [menu $br.menubar.m$m -tearoff 0]
		$br.menubar add cascade -label $m -menu $br.menubar.m$m
		}

	$br.menubar insert 2 command -label New -command "NewEdit $br"
	$br.menubar insert 3 command -label Evaluate -command "EvaluateEdit $br 0"
	$br.menubar insert 4 command -label EvaluatePrint -command "EvaluateEdit $br 1"
	$br.menubar insert 5 command -label Delete -command "DeleteEdit $br"
	$br.menubar entryconfigure 5 -state disabled

	if { $remoteFlag == 0 } {
	  $File add command -label {Create Context ...} -accelerator {Crtl-T} -underline 10 -command "CreateContext $br"
	  $File add command -label {Save Context As ...} -accelerator {Ctrl-S} -underline 0 -command "SaveContextAs $br"
	  }

	$File add command -label Refresh -accelerator {Ctrl-R} -underline 0 -command "RefreshContextList $br"
	$File add separator
	$File add command -label {Exit Editor} -underline 1 -command "ExitEditor $br"

	$Options add command -label {Background ...} -underline 0 -command "SetEditorColors editorBackground $br"
	$Options add command -label {Foreground ...} -underline 0 -command "SetEditorColors editorForeground $br"
	$Options add separator
	$Options add command -label {Save settings} -underline 0 -command "SaveSettings"

	frame $br.lbf
	set ctxbox [ScrolledListbox $br.lbf.ctx -height 1]
	set varbox [ScrolledListbox $br.lbf.var]

	pack $ctxbox $varbox -side top -expand true -fill both
	
	text $br.edit -width $Ide(editorWidth) -height $Ide(editorHeight)
	$br.edit config -yscrollcommand  "$br.scroll set"
	$br.edit config -insertofftime 0 -insertwidth 4
	$br.edit config -highlightcolor lightgrey
	
	scrollbar $br.scroll -command "$br.edit yview"

	grid $buttonFrame -row 0 -columnspan 2 -sticky ew
	grid $br.lbf -row 1 -column 0 -sticky news
	grid $br.edit -row 1 -column 1 -sticky news
	grid $br.scroll -row 1 -column 2 -sticky news


	grid rowconfigure $br 1 -weight 1
	grid columnconfigure $br 0 -weight 1
	grid columnconfigure $br 1 -weight 10


	frame $br.statusBar
	label $br.statusBar.label -textvariable statusBrowser -relief sunken -bd 1 -font "Helvetica 10" -anchor w
	grid $br.statusBar.label -sticky ew

	grid columnconfigure $br.statusBar 0 -weight 1
	grid $br.statusBar -row 2 -column 0 -columnspan 3 -sticky ew


	set spec [concat \"$Ide(fontName)\" $Ide(fontSize)]
	foreach item "$br.edit $br.lbf.var.list $br.lbf.ctx.list" {
		$item config -fg $Ide(editorForeground) -bg $Ide(editorBackground)
		$item config -font $spec
		}

	GotoWindow $br.edit

	wm title $br "MAIN"

	NewlispEvaluate "(silent (SYS:fill-listbox-var \"$varbox.list\" 'MAIN))"
	NewlispEvaluate "(silent (SYS:fill-listbox-ctx \"$ctxbox.list\"))"

	# setup browser popup menu

	set browserPopup [menu $br.popup]
	SetupPopupMenu $browserPopup

	# events for listboxes	
	bind $varbox.list <ButtonRelease-1> "ShowEditContent %W $br"
	bind $ctxbox.list <ButtonRelease-1> "ChangeEditContext %W $varbox $br"


	# events for the edit box
	bind $br.edit <Double-1> {HighlightExpression %W; break }
	bind $br.edit <Tab> "$br.edit insert insert {  }; break"

	bind $br.edit <Shift-KeyPress> {if { "%A" == "("} {EditShowMatchingPar %W 0} else {
			    if { "%A" == ")"} {EditShowMatchingPar %W 1}}}

	bind $br.edit <Button-3> "tk_popup $browserPopup %X %Y 0"
	bind $br.edit <Control-Button-1> "tk_popup $browserPopup %X %Y 0"

	bind $br.edit <Control-x> { CutCopyText %W }
	bind $br.edit <Control-c> { CutCopyText %W }
	bind $br.edit <Control-v> { PasteText %W }
	bind $br.edit <Destroy> {incr browserCount -1}
	bind $br <Control-p> "EvaluateEdit $br 1"
	
	bind $br <Control-t> "CreateContext $br"
	bind $br <Control-s> "SaveContextAs $br"
	bind $br <Control-r> "RefreshContextList $br"
	bind $br <Control-n> "NewEdit $br"
	bind $br <Control-b> "GotoWindow .console; break"
	bind $br <Control-d> "DeleteEdit $br"
	bind $br <Control-g> "Debugger"

	if { $Ide(platform) == "windows" } { 
		bind $br <F1> { CommandHelp %W } 
		}  
	}



# set up toolbar for browser / editor
proc 	SetupEditorToolbar { br } {

	global Ide remoteFlag

	frame $br.bframe

	foreach name {fileImg saveImg  newImg consoleImg copyImg cutImg pasteImg evalImg evalPrintImg deleteImg} {
		[set $name [image create photo]] read "$Ide(imageDir)/$name.gif"
		}

	foreach no {1 2 3 4 5} {
		label $br.bframe.vert$no -text {   } -relief flat
		}

	set st groove

	button $br.bframe.create -image $fileImg -relief $st -command "CreateContext $br"
	button $br.bframe.save -image $saveImg -relief $st -command "SaveContextAs $br"
	button $br.bframe.new -image $newImg -relief $st -command "NewEdit $br"
	button $br.bframe.console -image $consoleImg -relief $st -command "GotoWindow .console"
	button $br.bframe.copy -image $copyImg -relief $st -command "event generate $br.edit <Control-c>"
	button $br.bframe.cut -image $cutImg -relief $st -command "event generate $br.edit <Control-x>"
	button $br.bframe.paste -image $pasteImg -relief $st -command "event generate $br.edit <Control-v>"
	button $br.bframe.eval -image $evalImg -relief $st -command "EvaluateEdit $br 0"
	button $br.bframe.evalPrint -image $evalPrintImg -relief $st -command "EvaluateEdit $br 1"
	button $br.bframe.delete -image $deleteImg -relief $st -command "DeleteEdit $br"


	pack $br.bframe.create $br.bframe.save $br.bframe.new $br.bframe.vert2 -side left 
	pack $br.bframe.console $br.bframe.console $br.bframe.vert3 -side left
	pack $br.bframe.copy $br.bframe.cut $br.bframe.paste $br.bframe.vert4 -side left 
	pack $br.bframe.eval $br.bframe.evalPrint $br.bframe.vert5 $br.bframe.delete -side left 

	balloon_help $br.bframe.create { Create new context   Ctrl-T }
	balloon_help $br.bframe.save { Save context   Ctrl-S }
	balloon_help $br.bframe.new { Edit a new definition or function   Ctrl-N }
	balloon_help $br.bframe.console { Switch back to console   Ctrl-B }
	balloon_help $br.bframe.copy { Copy selection   Ctrl-C }
	balloon_help $br.bframe.cut { Cut selection   Ctrl-X }
	balloon_help $br.bframe.paste { Paste selection   Ctrl-V }
	balloon_help $br.bframe.eval { Evaluate editbox }
	balloon_help $br.bframe.evalPrint { EvaluatePrint editbox   Ctrl-P }
	balloon_help $br.bframe.delete { Delete symbol   Ctrl-D }

	$br.bframe.delete config -state disabled
	
	return $br.bframe
	}



# change colors

proc SetEditorColors { property widget } {

	global Ide

	set color [tk_chooseColor -initialcolor $Ide($property)]

	if {$color == ""} {GotoWindow $widget.edit; return}

	set Ide($property) $color

	$widget.edit config -bg $Ide(editorBackground) -fg $Ide(editorForeground)
	$widget.lbf.ctx.list config -bg $Ide(editorBackground) -fg $Ide(editorForeground)
	$widget.lbf.var.list config -bg $Ide(editorBackground) -fg $Ide(editorForeground)

	GotoWindow $widget.edit
	}


# jump to the matching parenhtesis in editor

proc EditShowMatchingPar { edit type } {

	set pos [$edit index insert]

	set idx_bal [EditGetMatchingPar $edit $pos $type]

	set idx [lindex $idx_bal 0]
	set balance [lindex $idx_bal 1]

	if {$balance == 0} {
		$edit mark set insert $idx
		$edit see $idx
		update ; after 300;
		$edit mark set insert $pos
		}
	}


# highlight expression when doubleclicking mouse

proc HighlightExpression { edit } {

	set pos [$edit index insert]

	# look for balanced opening to the left 

	set left_bal [EditGetMatchingPar $edit $pos 1]
	set left [lindex $left_bal 0]
	if {$left == ""} { return }

	# go one to the right

	set lin [lindex [split $left "."] 0]
	set col [lindex [split $left "."] 1]
	set left "$lin.[incr col]"

	# look for matching par to the right

	set right_bal [EditGetMatchingPar $edit $left 0]
	set right [lindex $right_bal 0]
	set balance [lindex $right_bal 1]

	if {$balance == 0} {
		set lin [lindex [split $left "."] 0]
		set col [lindex [split $left "."] 1]
		set left "$lin.[incr col -1]"
		$edit tag add sel $left $right
		}
	}


# get index of mathing paranthesis type: 0=open 1=close

proc EditGetMatchingPar { edit pos type } {

	set idx $pos
	if { $type == 0 } { set balance 1 } else { set balance -1 }

	while {$balance} {
		set pre $idx
		if { $type == 0 } {
			set idx [$edit search -regexp {[()]} $idx end]
		} else {
			set idx [$edit search -backwards -regexp {[()]} $idx 1.0]
			}

		if {$idx == "" } { break }

		set par [$edit get $idx]

		if {$par == "("} { incr balance }
		if {$par == ")"} { incr balance -1 }

		if { $type == 0 } {
			set lin [lindex [split $idx "."] 0]
			set col [lindex [split $idx "."] 1]
			set idx "$lin.[incr col]"
			}
		}
	return [list $idx $balance]
	}


proc BrowserWaitOn { browser } {
    $browser config -cursor watch; update
    $browser.edit config -cursor watch; update
    $browser.lbf.var config -cursor watch; update
    $browser.lbf.ctx config -cursor watch; update
    }


proc BrowserWaitOff { browser } {
    $browser config -cursor left_ptr; update
    $browser.edit config -cursor xterm; update
    $browser.lbf.var config -cursor left_ptr; update
    $browser.lbf.ctx config -cursor left_ptr; update
    }

######################### handle editor/browser listbox events ############################

proc ShowEditContent { varlist browser } {

	global Ide browserCurrent editboxFillIsOn

	set editboxFillIsOn 1
	set browserCurrent $browser


	set symbolName [$varlist get [$varlist curselection]]
	set contextName [lindex [split [wm title $browser] ":"] 0]

	wm title $browser "$contextName:$symbolName"

	BrowserWaitOn $browser

	$browser.edit delete 1.0 end

	NewlispEvaluate "(silent (SYS:show-edit-content '\"$contextName\" '\"$symbolName\"))" 
	update
	after 100
	}


proc ShowEditContentFinish {} {
	global browserCurrent editboxFillIsOn

	update
	after 100
	
    	BrowserWaitOff $browserCurrent

	$browserCurrent.menubar entryconfigure 5 -state normal	
	$browserCurrent.bframe.delete config -state normal
	}


proc ChangeEditContext { ctxlbox varbox browser} {
	set contextName [$ctxlbox get [$ctxlbox curselection]]

	wm title $browser $contextName
	RefreshVarlist $browser
	}


proc RefreshVarlist { browser } {
	set contextName [lindex [split [wm title $browser] ":"] 0]
	set varlist $browser.lbf.var.list
	$varlist delete 0 end
	NewlispEvaluate "(silent (set 'SYS:oldContext (context) ))"
	NewlispEvaluate "(silent (context 'MAIN))"
	NewlispEvaluate "(silent (SYS:fill-listbox-var \"$varlist\" '$contextName))"
	NewlispEvaluate "(silent (context SYS:oldContext))"
	$browser.menubar entryconfigure 5 -state disabled
	$browser.bframe.delete config -state disabled
	}


proc RefreshContextList { br } {

	$br.lbf.ctx.list delete 0 end
	NewlispEvaluate "(silent (SYS:fill-listbox-ctx \"$br.lbf.ctx.list\"))"
	}

########################### handle editor/browser menu commmands ##########################

proc CreateContext { browser } {

	toplevel .context
	label .context.label -text "Enter a context name or hit <Esc>"
	text .context.edit -width 24 -height 1
	pack .context.label .context.edit -side top -padx 10 -pady 5

	bind .context.edit <Return> "CreateNewContext $browser; break"
	bind .context.edit <Escape> "destroy .context"

	wm title .context "Create new context"
	wm transient .context $browser

	wm geometry .context \
		+[expr [winfo x $browser] + 300]+[expr [winfo y $browser] + 120]

	GotoWindow .context.edit
	}


proc CreateNewContext { browser } {

	set entry [string trim [.context.edit get 1.0 end]]

	if {$entry == ""} {destroy .context; return}

	# make sure nothing illegal in contextname
	set contextName [lindex [split $entry " :.()[],"] 0]

	$browser.lbf.ctx.list delete 0 end

	NewlispEvaluate "(silent (set 'SYS:oldContext (sym (string (context))) ))"
	NewlispEvaluate "(silent (context 'MAIN))"
	NewlispEvaluate "(silent (SYS:make-context-verify '$contextName \"$browser\"))"
	NewlispEvaluate "(silent (context SYS:oldContext))"

	destroy .context
	GotoWindow $browser.edit
	}


proc SelectContext { browser contextName } {

	set idx 0
	set found 0
	foreach name [$browser.lbf.ctx.list get 0 end] {
		if {$name == $contextName} { 
			set found 1
			break 
			}
		incr idx
		}

	if {$found == 1} {
		$browser.lbf.ctx.list selection set $idx
		wm title $browser $contextName
		RefreshVarlist $browser
		} 
	}


proc SaveContextAs { browser } {

	global currentBrowser remoteFlag

	set currentBrowser $browser

	set contextName [lindex [split [wm title $browser] ":"] 0]


	if {$remoteFlag == 0 } {
		set fileName [tk_getSaveFile -initialfile $contextName.lsp \
			-parent $browser -title "Save context $contextName as ..."]
	} else { FileDialog "context" "Save context $contextName as ..." $browser ; return }
		
	if { $fileName == "" } { GotoWindow $browser.edit; return }

	BrowserWaitOn $browser

	NewlispEvaluate "(silent (SYS:save-context-as \"$browser\" '$contextName \"$fileName\"))"
	}


proc SaveContextAsRemote {} {

	global statusText currentBrowser

	set br $currentBrowser

	set context [lindex [split [wm title $br] ":"] 0]

	set spec [string trim [.filedialog.edit get 1.0 end]]
	if {$spec == ""} {destroy .filedialog; return}

	BrowserWaitOn $br

	NewlispEvaluate "(silent (SYS:save-context-as \"$br\" '$context \"$spec\"))"

	destroy .filedialog
	}



proc SaveContextAsFinish { browser contextName fileName } {
    global statusText

    set statusText "Saved context $contextName to $fileName"
    BrowserWaitOff $browser
    }


proc EvaluateEdit { browser flag } {

	global Ide

	BrowserWaitOn $browser

	set contextName [lindex [split [wm title $browser] ":"] 0]
	set content [$browser.edit get 1.0 end]

	NewlispEvaluate "(silent (set 'SYS:oldContext (context) ))"
	NewlispEvaluate "(silent (context '$contextName))"
	NewlispEvaluateBuffer $content $flag
	NewlispEvaluate "(silent (context SYS:oldContext))"

	if {$flag == 1} {NewlispEvaluate ""}

	RefreshVarlist $browser
	BrowserWaitOff $browser
	}


proc NewEdit { browser } {
	set contextName [lindex [split [wm title $browser] ":"] 0]
	wm title $browser $contextName
	$browser.edit delete 1.0 end
	RefreshVarlist $browser
	}


proc DeleteEdit { browser } {
	set varlist $browser.lbf.var.list
	set idx [$varlist curselection]
	if {$idx == ""} { return }
	set symbolName [$varlist get $idx]
	set contextName [lindex [split [wm title $browser] ":"] 0]

	if { $contextName == "MAIN" && [IsProtected $symbolName] } {
		tk_messageBox -parent $browser -message "'$symbolName' symbol is protected, cannot delete."
		GotoWindow $browser.edit
		return
		}
	
	if { $contextName == "SYS" } {
		tk_messageBox -parent $browser -message {SYS context is protected, cannot delete.}
		GotoWindow $browser.edit
		return
		}

	NewlispEvaluate "(silent (set '$contextName:$symbolName nil))"
	$varlist delete 0 end
	NewlispEvaluate "(silent (SYS:fill-listbox-var \"$varlist\" '$contextName))"
	$browser.edit delete 1.0 end
	$browser.menubar entryconfigure 5 -state disabled
	$browser.bframe.delete config -state disabled
	wm title $browser $contextName
	}
	

proc IsProtected { sym } {
	if {$sym == "tk" || $sym == "net-read-line"} { return 1 } else { return 0 }
	}
	

proc ExitEditor { browser } {
	destroy $browser
	}

####################################### Debugger ##########################################

proc Debugger {} {

	global Ide debugIsOn debugOutput cmdStart txt browserCount 

	if {$browserCount != 0} {
		tk_messageBox -message "Close browsers first!";
		return;
		}

	if {$debugIsOn == 1} {
		GotoWindow .debug
		return
		}

	foreach name {stepImg nextImg continueImg quitImg} {
		[set $name [image create photo]] read "$Ide(imageDir)/$name.gif"
		}


	set db [toplevel .debug]

	text .debug.txt -width $Ide(debuggerWidth) -height $Ide(debuggerHeight)
	set fb [frame .debug.buttons]
	button $fb.step -image $stepImg -command DebuggerStep -relief groove
	button $fb.next -image $nextImg -command DebuggerNext -relief groove
	button $fb.continue -image $continueImg -command DebuggerContinue -relief groove
	button $fb.quit -image $quitImg -command DebuggerQuit -relief groove

	balloon_help $fb.step { Step }
	balloon_help $fb.next { Next }
	balloon_help $fb.continue { Continue }
	balloon_help $fb.quit { Quit }

	pack $fb.step $fb.next $fb.continue $fb.quit \
			-side left -fill both -expand true -pady 2 -padx 2

	grid $db.txt -row 0 -column 0 -sticky news
	grid $db.buttons -row 1 -column 0 -sticky news
	grid rowconfigure .debug 0 -weight 1
	grid columnconfigure .debug 0 -weight 1

	set spec [concat \"$Ide(fontName)\" $Ide(fontSize)]
	$db.txt config -font $spec -fg $Ide(debuggerForeground) -bg $Ide(debuggerBackground)
	$db.txt config -insertwidth 0
	$db.txt config -highlightcolor lightgrey

	global $db

	bind $db <Destroy> {if {"%W" == ".debug"} {DebuggerExit}}

	$db.txt tag configure reverseTag \
		-foreground $Ide(debuggerBackground) -background $Ide(debuggerForeground)

	$db.txt tag configure errorTag -foreground yellow -background red
	$db.txt tag configure infoTag -foreground black -background green

	set debugIsOn 1
	set debugOutput ""

	$db.buttons.step configure -state disabled
	$db.buttons.next configure -state disabled
	$db.buttons.continue configure -state disabled

	bind $db <Control-b> {GotoWindow .console; break}
	bind $db.txt <KeyPress> {break;}

	update idletasks

	NewlispEvaluate {(silent (trace-highlight "#>#" "#<#" "#!#\n" " > "))}


	$db.txt insert end {



	leave this window open and enter in the console:
	    
	    (debug (my-func params))

	where '(my-func params)' is a user defined function
	with parameters 'params'
	}

	update idletasks

	GotoWindow $db
	}


proc DebuggerExit {} {
	global debugIsOn

	set debugIsOn 0
	NewlispEvaluate {(silent (trace-highlight "#" "#" "\n-----\n\n" " s|tep n|ext c|ont q|uit > "))}
	}

proc DebuggerStep {} { 
	.debug.buttons.step configure -state disabled
	.debug.buttons.next configure -state disabled
	.debug.buttons.continue configure -state disabled
	NewlispEvaluate "s" 
	}

proc DebuggerNext {} { 
	.debug.buttons.step configure -state disabled
	.debug.buttons.next configure -state disabled
	.debug.buttons.continue configure -state disabled
	NewlispEvaluate "n" 
	}

proc DebuggerContinue {} { 
	NewlispEvaluate "c" 
	after 200
	destroy .debug
	}

proc DebuggerQuit {} {
	NewlispEvaluate "q"
	after 200
	destroy .debug
	}


proc DebuggerProcessOutput { line } {
	global debugOutput cmdStart txt

	append debugOutput $line

	if { [regexp {.*\[(->|<-).*\] > } $debugOutput] == 0 } { 
		if {[regexp {.*> } $debugOutput] == 1} {
			$txt insert end $debugOutput
			$txt mark set cmdStart insert	
			$txt see cmdStart
			set debugOutput ""
			.console config -cursor xterm; update
			}
		return 
		}

	.console config -cursor xterm; update

	set err ""
	set func ""
	set pre ""
	set exp ""
	set post ""
	set ppost ""
	set result ""
	set prompt ""
	set isError 0

	if { [string match "*#!#*" $debugOutput] == 1 } {

		regexp {(.*)#!#(.*)\[(->.*|<-.*)\] > } $debugOutput match err func prompt
		regexp {(.*)#>#(.*)#<#(.*)} $func match pre exp post

		set isError [string match "*ERR:*" $err]

		.debug.txt delete 1.0 end
		if {[regexp {(.*)(RESULT: .*)} $post match ppost result] == 1} {
			.debug.txt insert end $pre
			.debug.txt insert end $exp reverseTag
			.debug.txt insert end $ppost

			if { $isError == 1 } {
				.debug.txt insert end [string trim $err] errorTag 
			} else {
				$txt insert end $err }

			$txt insert end "\n$result"

		} else {
			.debug.txt insert end $pre
			.debug.txt insert end $exp reverseTag
			.debug.txt insert end $post

			if { $isError == 1 } {
				.debug.txt insert end [string trim $err] errorTag 
			} else {
				$txt insert end $err }
			}

		focus -force .debug
		}

	if { $isError  == 1} {
		.debug.buttons.step configure -state disabled
		.debug.buttons.next configure -state disabled
		.debug.buttons.continue configure -state disabled
	} else {
		.debug.buttons.step configure -state normal
		.debug.buttons.next configure -state normal
		.debug.buttons.continue configure -state normal
		}

	if { $prompt != "" } {
		$txt insert end "\n\[$prompt\] > "
	} else {
		$txt insert end $debugOutput
		}

	$txt mark set cmdStart insert	
	$txt see cmdStart

	set debugOutput ""
	}


############################## newLISP specific stuff #####################################

# send expressions to newLISP for evaluation

proc NewlispEvaluate { cmd } {
	
	global nlio

	puts $nlio $cmd
	flush $nlio
	}


# handle incoming output from newLISP

proc NewLISPreader {} {
	global txt nlio cmdStart debugIsOn editboxFillIsOn browserCurrent

	set line [read $nlio]

	if {$debugIsOn == 1} {
		DebuggerProcessOutput $line
		update
		after 100
		return
		}

        if {$editboxFillIsOn == 1} {
		
		if { [string match "*##EOT##" $line] == 1 } {
			set editboxFillIsOn 0 
			set start [string last "##EOT##" $line]
			set line [string range $line 0 [expr $start - 1]]
			}
		$browserCurrent.edit insert end $line
            update
            after 100
            return
            }

	$txt insert end $line
	$txt mark set cmdStart insert
	$txt see cmdStart

	.console config -cursor xterm
	update
	after 100
	}


############## user call back to newLISP from (tk {.......}) commands ##################
# sent a command to newLISP as a string

proc Newlisp { command } {
	global nlio

	puts $nlio "$command"
	flush $nlio
	}

############################ utility functions for dragging on canvas ####################
# supposed to be callled from newLISP


proc CanvasMark { x y can } {
	global canvas
	set x [$can canvasx $x]
	set y [$can canvasy $y]

	set canvas($can,obj) [$can find closest $x $y]
	set canvas($can,x) $x
	set canvas($can,y) $y
	}


proc CanvasDrag { x y can } {
	global canvas

	set x [$can canvasx $x]
	set y [$can canvasy $y]

	set dx [expr $x - $canvas($can,x)]
	set dy [expr $y - $canvas($can,y)]

	$can move $canvas($can,obj) $dx $dy
	set canvas($can,x) $x
	set canvas($can,y) $y
	}

###########################################################################################
#                                 MAIN ENTRY POINT                                        #
###########################################################################################

# overwrite settings with config file if it exists
#
# on Win32 load config file from current directory
#
# on Unix load config file from home directory

if { $Ide(platform) == "windows" } {
	if { [file exists "newlisp-tk.config"] } { 
		source "newlisp-tk.config" }
} else {
	if { [file exists "$Ide(Home)/newlisp-tk.config"] } { 
		source "$Ide(Home)/newlisp-tk.config" }
}


if { $Ide(newLISPhost) != $Ide(TCLTKhost) } { set remoteFlag 1 }

set FileTypeList [list [list "newLISP source files" "$Ide(lispFileExtension)" {"LSP "}] {"All files" {*}}];

eval $Ide(initCommand)

SetupConsole


########### Start newLISP in port server mode to handle requests from Tcl/Tk ##############

# start local copy of newLISP only if running newLISP and newLISP-tk on the same host
# else assume that newLISP already has been started at the remote $Ide(newLISPhost)
if { $remoteFlag == 0 } {
	set newLISPpid [exec $Ide(newLISPbin)/newlisp -p $Ide(newLISPport) &] }

# connect to newLISP

set count 0
while {[catch {set nlio [socket $Ide(newLISPhost) $Ide(newLISPport)]} result]} {
    incr count
    if { $count == 3} { 
	tk_messageBox -message "Cannot connect to newLISP at $Ide(newLISPhost):$Ide(newLISPport)" 
	exit
	}
    after 300}

set statusText "Tcl/Tk version $tcl_version on $tcl_platform(platform) $tcl_platform(osVersion)"

# setup event handler

fileevent $nlio readable NewLISPreader
fconfigure $nlio -blocking 0 -buffering none -buffersize 0


########################### preload newLISP with several functions ############################

set init_lsp {
;; some user functions to communicate with Tcl/Tk 
;;
(global 'tk 'tk-args)

(define (tk)
        (let (result (append (apply string (args)) ";##EOT##"))
          (net-send SYS:tk-sock 'result)
          ;; on Unix's we could just use 'read-line', but doesn't work on sockets in win32
          (while (starts-with (SYS:net-read-line SYS:tk-sock 'result) "VOID-TK:"))
          (if (starts-with result "ERR-TK:") (reset) result)))

(context 'SYS)

;; define several functions used by the browser/editor

(define (net-read-line socket buff)
        (net-receive socket buff 1000 "\n")
        (set buff (chop (eval buff))))

(define (myexit) 
	(tk "exit"))

(set '_README
	(append "please do not touch anything in this namespace, "
              "everything in SYS is generated at start up and "
              "used by the TCLTK GUI frontend"))

;; get a list of all variable symbols (not primitves and no contexts)
;;
(define (vars ctx) 
	(filter (lambda (x) 
		(and 
			(legal? (name x))
			(not (primitive? (eval x))) 
			(not (context? (eval x))) 
			(eval x) )) 
		(symbols ctx)))


;; fill the TK browser/editor variable listbox
;;
(define (fill-listbox-var lb-name ctx , items)
     (set 'items (map (lambda (x) (first (reverse (parse (string x) ":")))) (vars ctx)))
     (replace "true" items)
     (set 'items (filter (fn (x) (not (starts-with x "$"))) items))
     (set 'items (join items " "))
     (tk lb-name " insert end " items))


;; get all context symbols
;;
(define (ctxts)
	(unique (filter context? (map eval (symbols 'MAIN)))))

;; fill the TK browser/editor context listbox
;;
(define (fill-listbox-ctx lb-name , items)
        (set 'items (join (map string (ctxts)) " "))
        (tk lb-name " insert end " items))


;; fill the Tk current browser/editor edit box
;;
(define (show-edit-content contextName symbolName , buff ln)
    (set 'SYS:oldContext (context) )
        (context (sym contextName))
    (set 'buff  (source (sym symbolName)))
    (context SYS:oldContext)
        (replace "\r\n" buff "\n")
        (dolist (ln (parse buff "\n")) 
            (println ln))
        (print "##EOT##")
    (tk "ShowEditContentFinish"))


;; make new context after verifying correct parameter and highlight
;;
(define (make-context-verify sm browser)
	(if (symbol? sm) 
		(begin
			(context sm)
			(fill-listbox-ctx (append browser ".lbf.ctx.list"))
			(tk "SelectContext " browser " " sm))
		(tk "tk_messageBox -icon warning -parent "  browser " -message  {Illegal context name}")))


;; save all newLISP contents to a file
;;
(define (save-all-file newdir filespec)
        (save filespec)
        (change-dir newdir)
        (tk "SaveAllFileFinish {" filespec "}"))


;; save a context to a file
;;
(define (save-context-as browser contextname filespec)
        (save  filespec contextname)
        (tk "SaveContextAsFinish {" browser "} {" (string contextname) "} {" filespec "}"))


(context 'MAIN)

(constant 'exit SYS:myexit)
(constant 'tk)

(define (tk-args)
        (parse (tk "args") " "))

(constant 'tk-args)

;;; eof ;;;
}

# push content buffer to newLISP for evaluation
# note, that NewlispEvaluate{} alone does'nt work here because it takes only
# one line for evaluation, this function takes a whole buffer with line feeds

proc NewlispEvaluateBuffer {content flag} {

    global Ide nlio
    
    puts $nlio {[cmd]} 
    if {$flag == 0} { puts $nlio "(silent)" }
    puts $nlio $content
    puts $nlio {[/cmd]}
    flush $nlio
    }

NewlispEvaluateBuffer $init_lsp 0

################## setup Tcl/Tk listener for handling requests from newLISP ###############

proc AcceptNewLISPrequest {newSock addr port} {

    global TCLTKsock

    fileevent $newSock readable TCLTKreader
    fconfigure $newSock -blocking 0 -translation lf

    set TCLTKsock $newSock 
}


proc TCLTKreader {} {

    global TCLTKsock tkCommand

    set result ""
    set line [read $TCLTKsock]
    append tkCommand $line

    if { [string match "*##EOT##" $line] == 1 } {
        if [catch {eval $tkCommand} result] { 
           tk_messageBox -message $result;
           set result "ERR-TK: $result"
           }
        puts $TCLTKsock $result
        flush $TCLTKsock
        puts $TCLTKsock "VOID-TK:"
        set tkCommand ""
        }

}


set TCLTKlisten [socket -server AcceptNewLISPrequest $Ide(TCLTKport)]
#tk_messageBox -message "TCLTKlisten $TCLTKlisten"
#focus -force $txt

# connect from newLISP to TCL/TK
NewlispEvaluate "(silent (set 'SYS:tk-sock (net-connect \"$Ide(TCLTKhost)\" $Ide(TCLTKport))))"	

# check for command line arguments
proc args {} {
        global argv

        return $argv
        }

foreach arg $argv {
	NewlispEvaluate "(silent (load \"$arg\"))"
	}

NewlispEvaluate "(silent (catch (load \"$Ide(newLISPapp)\") 'error))"

if { [file isdirectory $Ide(initialDir)] == 1 } { cd $Ide(initialDir) }

# eof

