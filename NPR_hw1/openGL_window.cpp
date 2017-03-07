#include "OpenGL_window.h"

void OpenGL_window :: draw()  // a.k.a. RenderFunc()
{
		// the valid() property may be used to avoid reinitializing your GL transformation for each redraw:
		if (!valid()) {
			valid(1);
			glLoadIdentity();
			glViewport(0,0,w(),h());
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
		for(unsigned int i = 0; i  < drawcells.size(); ++i){
				
				glPointSize(5);	 // tmp value	
				glColor3f(0,0,0); // black
			
				glBegin( GL_POINTS );
					glVertex2f(
							(drawcells[i].position.x - width/2)/width *2, 
							(drawcells[i].position.y - height/2)/height * -2
					);
				glEnd();
		}
		
		// draw drag curve
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

		glFlush();
		++frame;
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

int OpenGL_window :: handle_mouse(int event, int button, int x, int y)
{
		struct cells tempcells; // since points push back immediately, we don't use OpenGL_window:tempcell
		struct vector2 temppos;
		temppos.x = x;
		temppos.y = y;
		tempcells.position = temppos;
		tmpcells.push_back(tempcells);	
			
		return 1;
}

int  OpenGL_window :: handle(int event)
{
		switch (event) {
				case FL_PUSH:
					if(callback() && (when() & FL_WHEN_CHANGED))
                        do_callback();
                    return 1;
				
				case FL_DRAG:
						if(!isDrag) // if not true, set to true
							isDrag = true;
						return handle_mouse(event,Fl::event_button(), Fl::event_x(),Fl::event_y());

				case FL_RELEASE:
						printf("Release %i", tmpcells.size());
						for(int i = 0; i < tmpcells.size(); ++i)
							drawcells.push_back(tmpcells.at(i));
						tmpcells.clear();
						isDrag = false;

				default:
						return Fl_Window::handle(event);
		}
}