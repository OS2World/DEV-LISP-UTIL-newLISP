;; @module zlisp.lsp
;; @version 1.1 - comments redone for automatic documentation
;; @author April 13th 2006, L.M.
;;
;; <h2>Functions for compression/decompression with zlib</h2> 
;; For this module a platform sepcific library
;; from @link http://www.zlib.net/ www.zib.net is needed.
;;
;; The module offers two types of compression/decompression support:
;; one for fast in memory compression/decopmpression, the other for
;; GZ compatible file compression and decompression.
;; 
;; Before using the module it must be loaded:
;; <pre>
;; (load "/usr/share/newlisp/zlib.lsp")
;; </pre>


(context 'zlib)

(if 
	;; LINUX and FreeBSD 
	(< (& 0xF (sys-info -1)) 3)	(set 'library "libz.so") 
	;; Mac OSX / Darwin
	(= (& 0xF (sys-info -1)) 3) (set 'library "libz.dylib")
	;; Solaris
	(= (& 0xF (sys-info -1)) 4)	(set 'library "libz.so") 
	;; Win32
	(> (& 0xF (sys-info -1)) 4) (set 'library "libz1.dll")
	true (println "Cannot load library, OS not supported"))

(import library "compress")
(import library "uncompress")
(import library "gzopen")
(import library "gzread")
(import library "gzclose")
(import library "gzwrite")

;; @syntax (zlib:squeeze <str-buffer>)
;; @return The string containing the compressed <str-buffer>.
;; @example
;; (set 'str-z  (zlib:squeeze str))

(define (squeeze src)
	(letn (	(srclen (length src)) 
			(destlen (int (add (mul 1.01 srclen) 12))) 
			(dest (dup "\000" destlen))
			(destlenp (pack "ld" destlen))
			)
	(compress dest destlenp src srclen)
	(set 'destlen (first (unpack "ld" destlenp)))
	(slice dest 0 destlen)))


;; @syntax (zlib:unsqueeze <str-buffer>)
;; @return The original uncompressed string from a compressed buffer in <str-buffer>
;; @example
;; (set 'str (zlib:unsqueeze str-z))

(define (unsqueeze src)
	(letn (	(srclen (length src)) 
			(destlen (* srclen 3)) 
			(dest (dup "\000" destlen))
			(destlenp (pack "ld" destlen))
			)
		(while (= -5 (uncompress dest destlenp src srclen))
			(set 'destlen (* 2 destlen))	
			(set 'dest (dup "\000" destlen))
			(set 'destlenp (pack "ld" destlen)))
		(set 'destlen (first (unpack "ld" destlenp)))
		(slice dest 0 destlen)))

;; @syntax (zlib:gz-read-file <str-file-name>)
;; @return A string buffer with the original contents.
;;
;; Uncompresses the GZ compressed file in <str-file-name>.
;; @example
;; (set 'buff (zlib:gz-read-file "myfile.gz"))

(define (gz-read-file file-name)
	(let ( (fno (gzopen file-name "rb"))
	       (buff (dup "\000" 0x1000))
	       (result ""))
		(if (!= fno 0)
			(begin
				(while (> (set 'bytes (gzread fno buff 0x1000)) 0)
					(write-buffer result buff bytes))
				(gzclose fno)
				result))))

;; @syntax (zlib:gz-write-file <str-file-name> <str-buffer>)
;; @return The number of bytes in <str-buffer>.
;;
;; Does a GZ compatible comrpression of a buffer in <str-buffer> and
;; writes it to the file in <str-file-name>.
;; @example
;; (zlib:gz-write-file "myfile.gz" buff) 

(define (gz-write-file file-name buff)
    (let ( (fno (gzopen file-name "wb"))
		   (result nil))
    	(if (!= fno 0)	
			(begin
				(set 'result (gzwrite fno buff (length buff)))
				(gzclose fno)
				result))))
	
(context MAIN) 

; eof

