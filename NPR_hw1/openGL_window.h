#ifndef  _OPENGL_WINDOW_H
#define _OPENGL_WINDOW_H

#define RADIUSRANGE 10

#include <vector>
#include <map>
#include <cmath>
#include <iostream>
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

struct CmpFunction
{
	bool operator() (const struct vector2 s1, const struct vector2 s2) const
	{
		return (
			s1.x == s2.x ? (
				s1.y > s2.y
				) : s1.x > s2.x
			);
	}
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
	int handle_mouse(int event, int button, float x, float y);

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
					rapidMode = isDrag = false;
					sum = 0;
					glViewport(x,y,w,h);

					tmpcells.reserve(sizeof(struct cells) * w * h);
					drawcells.reserve(sizeof(struct cells) * w * h * 2);
			}
			
			void genRadius();
			void genColor();
			void updateContainer();
			int handle(int event);
			int frame, width, height;
			unsigned char colorPick[3];
			float sum;

			bool isDrag, rapidMode;
			
			struct vector2 lastFrame;
			struct cells tempcells; 
			struct vector2 temppos;

			std::map<struct vector2, int, CmpFunction> hashTable;

			std::vector<struct cells> tmpcells;
			std::vector<struct cells> drawcells;
};

#endif