" Vim syntax file
" Language:    newLISP
" Maintainers: David S. de Lis <ocarina@tierramedia.org> and L.M.
" Last Change: 2007-15-01 L.M.
" Version:     1.0.40
"              (based on lisp.vim version 13,
"              http://www.erols.com/astronaut/vim/index.html#vimlinks_syntax)
" URL: http://www.geocities.com/excaliborus/newlisp/newlisp.vim

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if version >= 600
 setlocal iskeyword=42,43,45,47-58,60-62,64-90,97-122,_
else
 set iskeyword=42,43,45,47-58,60-62,64-90,97-122,_
endif

" Clusters
syn cluster       newlispAtomCluster      contains=newlispAtomBarSymbol,newlispAtomList,newlispAtomNmbr0,newlispComment,newlispDecl,newlispFunc,newlispLeadWhite
syn cluster       newlispListCluster      contains=newlispAtom,newlispAtomBarSymbol,newlispAtomMark,newlispBQList,newlispBarSymbol,newlispComment,newlispConcat,newlispDecl,newlispFunc,newlispKey,newlispList,newlispNumber,newlispSpecial,newlispSymbol,newlispVar,newlispLeadWhite

" Lists
syn match       newlispSymbol        contained         ![^()'{}"; \t]\+!
syn match       newlispBarSymbol     contained         !|..-|!
syn region       newlispList        matchgroup=Delimiter start="(" skip="|.\{-}|"          matchgroup=Delimiter end=")" contains=@newlispListCluster,newlispString
syn region       newlispString        matchgroup=Delimiter start="{" skip="|.\{-}|"          matchgroup=Delimiter end="}" contains=@newlispListCluster,newlispString
syn region       newlispStringBig        matchgroup=Delimiter start="\[text\]" skip="|.\{-}|"          matchgroup=Delimiter end="\[/text\]" contains=@newlispListCluster,newlispString
syn region       newlispBQList        matchgroup=PreProc   start="`("  skip="|.\{-}|"        matchgroup=PreProc   end=")" contains=@newlispListCluster,newlispString
" Atoms
syn match       newlispAtomMark        "'"
syn match       newlispAtom        "'("me=e-1         contains=newlispAtomMark      nextgroup=newlispAtomList
syn match       newlispAtom        "'[^ \t()]\+"         contains=newlispAtomMark
syn match       newlispAtomBarSymbol      !'|..\-|!         contains=newlispAtomMark
syn region       newlispAtom        start=+'"+         skip=+\\"+ end=+"+
syn region       newlispAtom        start=+'{+         skip=+\\}+ end=+}+
syn region       newlispAtomList        contained         matchgroup=Special start="("      skip="|.\{-}|" matchgroup=Special end=")"            contains=@newlispAtomCluster,newlispString
syn match       newlispAtomNmbr        contained         "\<\d\+"
syn match       newlispLeadWhite        contained         "^\s\+"

" Standard newlisp Functions and Macros
" lists, flow and integers
syn keyword newlispFunc + - * / % < > = <= >= != and append apply
syn keyword newlispFunc args assoc begin case catch collect cond cons
syn keyword newlispFunc constant count define define-macro def-new difference
syn keyword newlispFunc doargs dolist dotimes dotree dup ends-with eval first filter clean
syn keyword newlispFunc find flat for if index intersect last length let letex letn
syn keyword newlispFunc list lookup map match mem member name not nth nth-set
syn keyword newlispFunc or pop push quote ref ref-all rest replace replace-assoc
syn keyword newlispFunc reverse rotate select set setq set! set-nth
syn keyword newlispFunc silent slice sort starts-with swap unique unless
syn keyword newlispFunc until while do-until do-while lambda fn expand
" floats and special math
syn keyword newlispFunc amb abs acos acosk add asin asinh atan atanh atan2 
syn keyword newlispFunc beta beta1 binomial ceil crc32 crit-z cos cosh dec div exp
syn keyword newlispFunc factor fft floor flt gammai gcd atan ifft inc log max min mod mul normal
syn keyword newlispFunc pow prob-chi2 prob-z round sequence series sin sinh rand random
syn keyword newlispFunc randomize seed sqrt sub tan tanh uuid
syn keyword newlispFunc bayes-query bayes-train unify
" bits
syn keyword newlispFunc << >> & \| ^ ~
" matrices
syn keyword newlispFunc det invert mat multiply transpose
" arrays
syn keyword newlispFunc array array-list array? nth-set set-nth
" financial
syn keyword newlispFunc fv irr nper npv pv pmt
" date and time
syn keyword newlispFunc date date-value now time time-of-day
" strings and conversions
syn keyword newlispFunc append char chop collect ends-with encrypt
syn keyword newlispFunc eval-string explode find first float format
syn keyword newlispFunc get-char get-float get-int get-long get-string integer
syn keyword newlispFunc int get-int join last lower-case match nth nth-set pack
syn keyword newlispFunc parse regex replace rest reverse select set-nth
syn keyword newlispFunc slice source starts-with string sym symbol title-case trim 
syn keyword newlispFunc unicode unpack upper-case utf8 utf8len xml-error xml-parse 
syn keyword newlispFunc xml-type-tags
" IO and files
syn keyword newlispFunc append-file close command-line current-line device exec
syn keyword newlispFunc load open peek print println 
syn keyword newlispFunc read-buffer read-char read-file read-key read-line save
syn keyword newlispFunc search seek write-buffer write-char write-file
syn keyword newlispFunc write-line
" directories
syn keyword newlispFunc change-dir copy-file delete-file directory file-info
syn keyword newlispFunc make-dir real-path remove-dir rename-file
" processes and threads
syn keyword newlispfunc ! exec fork process semaphore share timer wait-pid
" system and predicates
syn keyword newlispFunc address atom? callback catch context context? debug default delete
syn keyword newlispFunc directory? empty? env environ error-event error-number
syn keyword newlispFunc error-test exit file? float? getenv global import
syn keyword newlispFunc integer? lambda? legal? list? macro? main-args NaN? new
syn keyword newlispFunc nil nil? null? ostype pipe pretty-print primitive? putenv quote?
syn keyword newlispFunc reset signal set-locale sleep string? symbol? symbols 
syn keyword newlispFunc sys-info throw trace trace-highlight true true?
syn keyword newlispFunc throw-error number? zero?
" HTTP network API
syn keyword newlispFunc base64-dec base64-enc delete-url get-url  post-url put-url
" network TCP/IP API
syn keyword newlispFunc net-accept net-close net-connect net-error net-eval net-listen
syn keyword newlispFunc net-local net-lookup net-peer net-peek net-ping net-receive
syn keyword newlispFunc net-receive-from net-receive-udp net-select net-send 
syn keyword newlispFunc net-send-to net-send-udp net-service net-sessions
" newLISP internals API
syn keyword newlispFunc cpymem dump


" boolean values
syn keyword newlispBoolean true false

" Strings
syn region       newlispString        start=+"+ skip=+\\\\\|\\"+ end=+"+
syn region       newlispString        start=+{+ skip=+\\\\\|\\}+ end=+}+
syn region       newlispString        start=+\[text\]+ skip=+\\\\\|\\]+ end=+\[/text\]+

" Declarations, Macros, Functions
syn keyword newlispDecl     define  define-macro

" Numbers: supporting integers and floating point numbers
syn match newlispNumber     "-\=\(\.\d\+\|\d\+\(\.\d*\)\=\)\(e[-+]\=\d\+\)\="
syn match	newlispNumber	    "[-#+0-9.][-#+0-9a-feEx]*"

" syn match newlispSpecial     "\*[a-zA-Z_][a-zA-Z_0-9-]*\*"
" syn match newlispSpecial     !#|[^()'`,"; \t]\+|#!
" syn match newlispSpecial     !#x[0-9a-fA-F]\+!
" syn match newlispSpecial     !#o[0-7]\+!
" syn match newlispSpecial     !#b[01]\+!
" syn match newlispSpecial     !#\\[ -\~]!
" syn match newlispSpecial     !#[':][^()'`,"; \t]\+!
" syn match newlispSpecial     !#([^()'`,"; \t]\+)!

" syn match newlispConcat     "\s\.\s"
syn match newlispParenError   ")"

" Comments
syn cluster newlispCommentGroup   contains=newlispTodo
syn match   newlispComment     ";.*$"          contains=@newlispCommentGroup
syn match   newlispComment     "#.*$"          contains=@newlispCommentGroup
"syn region  newlispCommentRegion   start="#|" end="|#"      contains=newlispCommentRegion,@newlispCommentGroup

" synchronization
syn sync lines=100

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_newlisp_syntax_inits")
  if version < 508
    let did_newlisp_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink newlispCommentRegion   newlispComment
  HiLink newlispAtomNmbr     newlispNumber
  HiLink newlispAtomMark     newlispMark
  HiLink newlispStringBig   newlispString
  HiLink newlispStringOpt   newlispString

  HiLink newlispAtom     Identifier
  HiLink newlispAtomBarSymbol   Special
  HiLink newlispBarSymbol     Special
  HiLink newlispBoolean     Special
  HiLink newlispComment     Comment
  HiLink newlispConcat     Statement
  HiLink newlispDecl     Statement
  HiLink newlispFunc     Statement
  HiLink newlispKey     Type
  HiLink newlispMark     Delimiter
  HiLink newlispNumber     Number
  HiLink newlispParenError     Error
  HiLink newlispSpecial     Type
  HiLink newlispString     String
  HiLink newlispTodo     Todo
  HiLink newlispVar     Statement

  delcommand HiLink
endif

let b:current_syntax = "newlisp"

" vim: ts=2
