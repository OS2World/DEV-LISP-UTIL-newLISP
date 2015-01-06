#!/usr/bin/newlisp

; demo client for non-blocking UDP communications
;
; start the server program udp-server.lsp first
;
; note, that net-listen in UDP mode only binds the socket
; to the local address, it does not 'listen' as in TCP/IP
; v.1.0

(set 'socket (net-listen 10002 "" "udp"))
(if (not socket) (println (net-error)))
(while (not (net-error))
	(print "->")
	(net-send-to "127.0.0.1" 10001 (read-line) socket)
	(net-receive socket 'buff 255)
	(println "=>" buff))

; eof

