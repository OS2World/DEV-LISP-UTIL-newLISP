#!/usr/bin/newlisp

# OpenGL and GLUT demo - opengl-demo.lsp
# currently tested working only on Win32
# Linux is untested and Mac OS X not working yet

# WIn32
# opengl32.dll - should be already on you WindowsXP installation or at www.opengl.org  
# glut32.dll -  available here: http://www.opengl.org/resources/libraries/glut/
# 
# Linux/UNIX - not tested
# libGLU.so - should be on your Linux/UNIX installation or at www.opengl.org
# glut-3.7.so - already on your system or at:
#     http://www.opengl.org/resources/libraries/glut/
#
# Mac OS X - cannot work on PPC, not tested yet on x86 Intel based OS X
# libGL.dylib - installed in /System/Library/Frameworks/OpenGL.Framework
# GLUT - installed in /System/Library/Frameworks/GLUT.Framework
#
# for a complete function reference for OpenGL look here:
#     http://rush3d.com/reference/opengl-bluebook-1.0/index.html
# linked from:
#     http://www.opengl.org/documentation/
#
# for descriptions and refence to GLU see here:
#     http://www.opengl.org/resources/libraries/glut/
#
# Calling patterns for OpenGL functions:
# Functions are described in the manual with the following types:
#
# All the following integer types are passed as is or with (int ...):
# GLenum GLboolean GLbitfield GLbyte GLshort GLint GLsizei GLubyte GLushort GLuint
# and types ending in: 3b, si, 3s
#
# The following double float types are passed as is or with (float ...):
# GLdouble GLclampd
# and types ending in: 3d
#
# The following small float types are must with (flt ...) conversion:
# GLfloat GLclampf
# and all types ending 3f:
# 


(if 
  (= ostype "OSX") ;; drawings will be visible only on x86 based OS X (not tested)
  (begin
    (set 'GL_LIB "/System/Library/Frameworks/OpenGL.Framework/libraries/libGL.dylib")
    (set 'GLUT_LIB "/System/Library/Frameworks/GLUT.Framework/GLUT")
  )
  (= ostype "Win32") ;; everything works
  (begin
    (set 'GL_LIB "opengl32.dll")
    (set 'GLUT_LIB "glut32.dll"))
  (= ostype "Linux") ;; not tested
  (begin
    (set 'GL_LIB "libGL.so")
    (set 'GLUT_LIB "libglut.so"))
)

(import GL_LIB "glClear")
(import GL_LIB "glClearColor")
(import GL_LIB "glEnable")
(import GL_LIB "glColor3d")
(import GL_LIB "glBegin")
(import GL_LIB "glEnd")
(import GL_LIB "glVertex3d")
(import GL_LIB "glFlush")
(import GL_LIB "glFinish")
(import GL_LIB "glRotated")

(import GLUT_LIB "glutInit")
(import GLUT_LIB "glutDisplayFunc")
(import GLUT_LIB "glutInitWindowSize")
(import GLUT_LIB "glutInitDisplayMode")
(import GLUT_LIB "glutInitWindowPosition")
(import GLUT_LIB "glutDisplayFunc")
(import GLUT_LIB "glutKeyboardFunc")
(import GLUT_LIB "glutMouseFunc")
(import GLUT_LIB "glutMotionFunc")
(import GLUT_LIB "glutIdleFunc")
(import GLUT_LIB "glutCreateWindow")
(import GLUT_LIB "glutCreateSubWindow")
(import GLUT_LIB "glutMainLoop")
(import GLUT_LIB "glutSwapBuffers")
(import GLUT_LIB "glutPostRedisplay")

(import GLUT_LIB "glutSolidCube")
(import GLUT_LIB "glutSolidTeapot")
(import GLUT_LIB "glutWireCube")
(import GLUT_LIB "glutWireTeapot")

(constant 'GL_COLOR_BUFFER_BIT  0x00004000)
(constant 'GL_TRIANGLES         0x0004)

(constant 'GLUT_DOUBLE 0x0002)
(constant 'GLUT_SINGLE 0x0000)
(constant 'GLUT_RGB 0x0000)
(constant 'GLUT_RGBA 0x0000)

(set 'argc 0)
(set 'argv1 "")
(set 'argv (pack "lu" argv1))
(set 'rotx 0.0)
(set 'roty 0.0)
(set 'PI (mul (acos 0) 2))

(define (draw) 
	(glClear GL_COLOR_BUFFER_BIT )
	(glRotated rotx 0.0 1.0 0.0)
	(glRotated roty 1.0 0.0 0.0)
	(glutWireTeapot 0.5)
	(glutSwapBuffers)
)

(define (drawObject)
	(glClear GL_COLOR_BUFFER_BIT )
	(glColor3d 1.0 0.85 0.35)
	(glBegin GL_TRIANGLES)
	(glVertex3d 0.0 0.6 0.0)
	(glVertex3d -0.2 -0.3 0.0)
	(glVertex3d 0.2 -0.3 0.0)
	(glEnd)
)

(define (rotation)
	(set 'rotx (mod (sub rotx .01) PI))
        (set 'roty (mod (sub roty .012) PI))
	(sleep 30)
	(glutPostRedisplay)
)

(define (keyboard key x y)
	(if (= (& key 0xFF) 27) (exit)) ; 0xFF mask necessary in Win32
	(println "key:" (& key 0xFF) " x:" x  " y:" y))

(define (mouse button state x y)
	(if (= state 0)
		(glutIdleFunc 0) ; auto-motion off on button press
		(glutIdleFunc (callback 4 'rotation))) ; auto-motinn on
	(println "button: " button " state:" state " x:" x " y:" y))

(define (motion x y)
	(set 'rotx (mul (div 200 x) PI))
	(set 'roty (mul (div 150 y) PI))
	(glutPostRedisplay)
	(println "x:" x  " y:" y))

(glutInit (address argc) (address argv))
(glutInitDisplayMode (| GLUT_RGB GLUT_DOUBLE ))
(glutInitWindowSize 800 600)
(glutInitWindowPosition 80 80)
(set 'id (glutCreateWindow "OpenGL and newLISP - ESC to exit"))

(glClearColor (flt 0.5) (flt 0.3) (flt 0.3) (flt 0.0))
(glColor3d 1.0 0.85 0.35)

(glutDisplayFunc (callback 0 'draw))
;(glutDisplayFunc (callback 0 'drawObject))
(glutKeyboardFunc (callback 1 'keyboard))
(glutMouseFunc (callback 2 'mouse))
(glutMotionFunc (callback 3 'motion))
(glutIdleFunc (callback 4 'rotation))
(glutMainLoop)

;; eof
