#!/usr/bin/newlisp
;
; This demo shows how to write Tcl/Tk GUIs
; controlled from newLISP
;
; newlisp-tk is not required, only newlisp
; and a Tcl/Tk installation
;

; setup communications
;
(map set '(myin tcout) (pipe))
(map set '(tcin myout) (pipe))
(process "wish" tcin tcout)

; make GUI
;
(write-buffer myout 
[text]
wm geometry . 250x90
wm title . "Tcl/Tk and newLISP"

button .one -text {red}
button .two -text {green}
button .three -text {blue}
label .colorlabel -width 25

grid .one .two .three -padx 8 -row 0
grid .colorlabel -column 0 -columnspan 3 -pady 6

.one config -command {puts {(call-back "red")}}
.two config -command {puts {(call-back "green")}}
.three config -command {puts {(call-back "blue")}}

bind . <Destroy> {puts {(exit)}}
[/text])

(define (call-back color)
	(write-line (append ".colorlabel config -background " color) myout)
	)


; run event loop
;
(while (read-line myin)
	(eval-string (current-line)))

;; eof
