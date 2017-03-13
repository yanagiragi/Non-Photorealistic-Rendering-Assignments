#include "OpenGL_window.h"

void OpenGL_window :: draw()  // a.k.a. RenderFunc()
{
		// the valid() property may be used to avoid reinitializing your GL transformation for each redraw:
		if (!valid()) {
			valid(1);
			glLoadIdentity();
			glViewport(0,0,w(),h());
			glEnable(GL_POINT_SMOOTH);
			glEnable(GL_BLEND);
			glClearColor(1, 1, 1, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			
		}
		
		// Draw White Canvas
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// draw picture
		for (unsigned int i = 0; i < drawcells.size(); ++i) {

			glPointSize(drawcells[i].radius);	 // Radius

			glColor4f(
				drawcells[i].ink,
				drawcells[i].ink,
				drawcells[i].ink,
				1- drawcells[i].ink
			); // Ink
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			// set old to 1- drawcells[i].ink, new to drawcells[i].ink
						

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
				case FL_KEYUP:
					//if(event)
					if (Fl::event_key() == 99)
						drawcells.clear();
					return 1;

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

					updateContainer();

					tmpcells.clear();

					lastFrame.x = lastFrame.y = -1;
					isDrag = false;
				
				default:
						return Fl_Window::handle(event);
		}
}

void OpenGL_window::updateContainer()
{
	for (int i = 0; i < tmpcells.size(); ++i) {
		if (hashTable.find(tmpcells[i].position) == hashTable.end() ) { // not found
			hashTable.insert(std::pair<struct vector2, int>(tmpcells[i].position, drawcells.size()));
			drawcells.push_back(tmpcells[i]);
		}
		else {
			float maxer = tmpcells[i].count > drawcells[hashTable[tmpcells[i].position]].count ? tmpcells[i].count : drawcells[hashTable[tmpcells[i].position]].count;
			drawcells[hashTable[tmpcells[i].position]].count = maxer;

			drawcells[hashTable[tmpcells[i].position]].ink = abs(drawcells[hashTable[tmpcells[i].position]].ink + drawcells[i].ink) / 2.0;
		}
	}
	
	return;
}