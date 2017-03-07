#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>

#include "OpenGL_window.h"
#include <glut.h>
#include <stdio.h>
#include <time.h>


int main(int argc, char **argv) {
	
	Fl_Window window(1240 , 840, "NPR_hw1");  // Create a FLTK window. Resolution 600*400. 	
	
	OpenGL_window gl_win(20, 20, 1200 ,800);

	window.end();							 // End of FLTK windows setting. 
	window.show(argc,argv);        // Show the FLTK window

	gl_win.show();							// Show the OpenGL window
	gl_win.redraw_overlay();       // redraw 

	return Fl::run();
}