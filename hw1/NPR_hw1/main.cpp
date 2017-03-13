#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Check_Button.H>

#include "OpenGL_window.h"
#include <GL/glut.h>
#include <stdio.h>
#include <time.h>

void Check_CB(Fl_Widget* o, void *window) {
	((OpenGL_window *)(window))->rapidMode = !((OpenGL_window *)(window))->rapidMode;
}

int main(int argc, char **argv) {
	
	Fl_Window window(640 , 640, "NPR_hw1");  // Create a FLTK window. Resolution 600*400. 	
	
	OpenGL_window gl_win(30, 30, 580 ,580);

	/*Fl_Widget *checkbox;
	checkbox = new Fl_Check_Button(640 - 130, 6, 110, 18, "Overlap Calc");
	checkbox->callback(Check_CB, &gl_win);*/

	window.end();					// End of FLTK windows setting. 
	window.show(argc,argv);			// Show the FLTK window

	gl_win.show();					// Show the OpenGL window
	gl_win.redraw_overlay();		// redraw 

	return Fl::run();
}