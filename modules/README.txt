To generate HTML documentation for the modules in this directory, simply
execute the following statement inside the newlisp-x.x.x/modules directory:

   newlispdoc -s *.lsp

or on Win32

   newlisp -s newlispdoc *.lsp

This will genereate an index page index.html and one html file each
for each module of the form name.lsp.html, where name is the name of the module.
For additional conversion options see newlisp-x.x.x/doc/newLISPdoc.html .

The newlispdoc utility can be found in newlisp-x.x.x/examples/newlispdoc .

Documentation on how to format newLISP source files for this program can be found 
in newlisp-x.x.x/doc/newLISPdoc.html or in /usr/share/doc/newlisp/newLISPdoc.html.


