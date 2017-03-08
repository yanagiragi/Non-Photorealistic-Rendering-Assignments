#ifndef  _OPENGL_WINDOW_H
#define _OPENGL_WINDOW_H

#define RADIUSRANGE 10

#include <vector>
#include <cmath>
#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Button.H>

#include <stdio.h>

struct vector2{
	float x , y;
};

struct cells{
	float radius, ink;
	struct vector2 position;
	int count; // how many frames stays on it
};

class OpenGL_window : public Fl_Gl_Window { 
	static void Timer_CB(void *userdata)
	{
			OpenGL_window *pb = (OpenGL_window*)userdata;
			pb->redraw();
			Fl::repeat_timeout(1.0/24.0, Timer_CB, userdata);
	}

	void draw() ;
	void draw_overlay();
	int handle_mouse(int event, int button, int x, int y);

	public:
			OpenGL_window(int x,int y,int w,int h,const char *l=0) : Fl_Gl_Window(x,y,w,h,l) 
			{
					mode( FL_RGB | FL_ALPHA | FL_DOUBLE | FL_STENCIL );
					Fl::add_timeout(1.0/24.0, Timer_CB, (void*)this);
					frame  = 0;
					
					width = w;
					height = h;
					
					lastFrame.x = -1;
					lastFrame.y = -1;
					isDrag = false;

					glViewport(x,y,w,h);
					
			}
			
			void genRadius();
			int handle(int event);
			int frame, width, height;
			bool isDrag;
			
			struct vector2 lastFrame;
			std::vector<struct cells> tmpcells;
			std::vector<struct cells> drawcells;
};

#endif