#include "OpenGL_window.h"

void OpenGL_window :: draw()  // a.k.a. RenderFunc()
{
		// the valid() property may be used to avoid reinitializing your GL transformation for each redraw:
		if (!valid()) {
			valid(1);
			glLoadIdentity();
			glViewport(0,0,w(),h());
			glEnable(GL_POINT_SMOOTH);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			
		}
		
		// Draw White Canvas
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBegin(GL_QUADS);
			glColor3f(1, 1, 1);glVertex2f(-1, -1);
			glColor3f(1, 1, 1);glVertex2f(-1,  1);
			glColor3f(1, 1, 1);glVertex2f( 1,  1);
			glColor3f(1, 1, 1);glVertex2f( 1, -1);
		glEnd();
		
		// draw picture
		for (unsigned int i = 0; i < drawcells.size(); ++i) {

			glPointSize(drawcells[i].radius);	 // Radius

			if (rapidMode) {
				glReadPixels(drawcells[i].position.x, height - drawcells[i].position.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colorPick);

				if ((float)colorPick[0] != 255.0) { // if pixel already has color
					float tmpcolor = abs((colorPick[0] / 255.0) - drawcells[i].ink);
					glColor3f(
						tmpcolor,
						tmpcolor,
						tmpcolor
					); // Ink
				}
				else {
					glColor3f(
						drawcells[i].ink,
						drawcells[i].ink,
						drawcells[i].ink
					); // Ink
				}
			}
			else {
				glColor3f(
					drawcells[i].ink,
					drawcells[i].ink,
					drawcells[i].ink
				); // Ink
			}			

			glBegin( GL_POINTS );
				glVertex2f(
						(drawcells[i].position.x - width/2)/width *2, 
						(drawcells[i].position.y - height/2)/height * -2
				);
			glEnd();
		}
		
		// draw drag curve
		if(tmpcells.size() > 0 && isDrag){
			for(unsigned int i = 0; i  < tmpcells.size(); ++i){
				if(isDrag){
					glPointSize(5);	 // tmp value	
					glColor3f(1,0,0);
					glBegin( GL_POINTS );
						glVertex2f(
									(tmpcells[i].position.x - width/2)/width *2, 
									(tmpcells[i].position.y - height/2)/height * -2
							);
					glEnd();
				}				
			}
		}

		//glFlush();
		++frame;
}

void OpenGL_window :: genRadius()
{
	for (auto& n : tmpcells)
		sum += n.count;
	
	float mean = sum / 2;
	float nowcount = 0;
	
	for (auto& n : tmpcells){
		nowcount += (float)n.count;
		n.radius = RADIUSRANGE - abs(nowcount - mean) / mean * RADIUSRANGE;
	}	
	
	return;
}

void OpenGL_window::genColor()
{
	float nowcount = 0;
	float incre = (sum / tmpcells.size() / 255.0);
	
	for (auto& n : tmpcells) {
		++nowcount;
		n.ink = nowcount *  incre * (float)n.count; // bigger value means brighter
	}

	sum = 0;

	return;
}

void OpenGL_window :: draw_overlay()
{
		// Draw overlay function. 
		// the valid() property may be used to avoid reinitializing your GL transformation for each redraw:
		if ( !valid() ) {
			valid(1);
			glLoadIdentity();
			glViewport(0,0,w(),h());
		}
}

int OpenGL_window :: handle_mouse(int event, int button, float x, float y)
{
		if(lastFrame.x == x && lastFrame.y == y){
			++ tmpcells.back().count;
			lastFrame.x = x;
			lastFrame.y = y;
			return 1;
		}
		
		lastFrame.x = x;
		lastFrame.y = y;
		
		temppos.x = x;
		temppos.y = y;
		tempcells.position = temppos;
		tempcells.count = 1;
		tmpcells.push_back(tempcells);	
			
		return 1;
}

int  OpenGL_window :: handle(int event)
{
		switch (event) {
				case FL_PUSH:
					return handle_mouse(event, Fl::event_button(), (float)Fl::event_x(), (float)Fl::event_y());
					return 1;
				
				case FL_DRAG:
						if(!isDrag) // if not true, set to true
							isDrag = true;
						return handle_mouse(event,Fl::event_button(), (float)Fl::event_x(), (float)Fl::event_y());

				case FL_RELEASE:
						genRadius();
						genColor();
						
						for (int i = 0; i < tmpcells.size(); ++i) {
							drawcells.push_back(tmpcells[i]);
						}
							
						tmpcells.clear();
						
						lastFrame.x = lastFrame.y = -1;
						isDrag = false;

				default:
						return Fl_Window::handle(event);
		}
}