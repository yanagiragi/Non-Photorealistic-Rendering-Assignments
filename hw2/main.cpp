#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GL/glut.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <cmath>

#define MAX_FRAME 4294697295

using namespace glm;

// Bugs: Reshape will failed

struct Bristle{
	double ink;
	vec4 color;
};
unsigned char colorBuffer[4];
unsigned short int dragPointSize = 1;
double dryThreshold = 0.1;
double diffuseAmmount = 0.1;
unsigned int frame = 0;
bool isDrag = false;
const int width = 200;
const int height = 200;
struct Bristle bristle[height][width];
std::vector<vec2> nowDragged;
std::vector<vec2> nowCanvas;

// Mathmatics
float clamp(float, float, float);

// GLs
void initGL();
void display();
void reshape(GLsizei, GLsizei);
void idle();

void MainDisplay();
void diffuseInk(int h, int w, int i);
bool BoundaryCheck(int h, int w);
void AddToCanvasIsNotExists(int h, int w);
void mouseDrags(int, int);
void mouseClicks(int, int, int, int);

// Keyboard events
void postDealKey();
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);


/* Main function: GLUT runs as a console application starting at main() */
int main(int argc, char** argv) {

	for(int i = 0; i < height; ++i){
		for(int j = 0; j < width; ++j){
			//bristle[i][j].size = 0.;
			bristle[i][j].ink = 0.;
			bristle[i][j].color = vec4(1.,1.,1.,1.);
		}
	}

	glutInit(&argc, argv);								// Initialize GLUT
	glutInitDisplayMode(GLUT_DOUBLE);					// Enable double buffered mode
	glutInitWindowSize(width, height);					// Set the window's initial width & height
	glutInitWindowPosition(500, 500);						// Position the window's initial top-left corner
	glutCreateWindow("NPR hw2");						// Create window with the given title
	glutDisplayFunc(display);							// Register callback handler for window re-paint event
	//glutReshapeFunc(reshape);							// Register callback handler for window re-size event
	glutIdleFunc(idle);
	glutMouseFunc(mouseClicks);
	glutMotionFunc(mouseDrags); 
	glutKeyboardFunc(keyboard);							// Reigister callback handler for keyboard event
	glutKeyboardUpFunc(keyboardUp);						// Reigister callback handler for keyboardUp event

	glewExperimental=true;							 	// Needed in core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	initGL();											// Our own OpenGL initialization
	
	glutMainLoop();										// Enter the infinite event-processing loop

	return 0;
}

/* Initialize OpenGL Graphics */
void initGL()
{
	glClearColor(1, 1, 1, 0);
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	
	//glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_POINT_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION); 

	glViewport(0, 0, width, height);
	glLoadIdentity(); 
}

/* Handler for window-repaint event. Called back when the window first appears and
   whenever the window needs to be re-painted. */
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 	// Clear color and depth buffers
	glMatrixMode(GL_MODELVIEW);    							// To operate on model-view matrix
	glLoadIdentity();
	MainDisplay();
	glutSwapBuffers();  									// Swap the front and back frame buffers (double buffering)   
}
 
/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei width, GLsizei height) {  // GLsizei for non-negative integer
   // Compute aspect ratio of the new window
   if (height == 0) height = 1;                // To prevent divide by 0
   GLfloat aspect = (GLfloat)width / (GLfloat)height;
 
   // Set the viewport to cover the new window
   glViewport(0, 0, width, height);
}

float clamp(float v, float min, float max)
{
	return (v > max) ? max : ((v < min) ? min : v);
}


void idle()
{
	if(frame >= MAX_FRAME){
		frame = 0;
	}
	else{
		++frame;
	}

	if(frame % 180000 == 0){
		printf("Ding!\n");

		for(int i = 0; i < nowCanvas.size(); ++i){
			
			int w = nowCanvas[i].x;
			int h = nowCanvas[i].y;
			
			// TODO: Do Some Color Blend Stuff
			// glReadPixels(w, height - h, 1, 1 ,GL_RGB, GL_UNSIGNED_BYTE, colorBuffer);
			// printf("Now: %d %d : %f, dryThreshold = %f\n", h, w, bristle[h][w].ink, dryThreshold);
			if(bristle[h][w].ink > dryThreshold){
				printf("%d %d : %f\n", h, w, bristle[h][w].ink);
				for(int j = 0; j < 8 && bristle[h][w].ink > dryThreshold; ++j){
					diffuseInk(h, w, j);
				}
			}
			
		}

		glutPostRedisplay();
	}
	
	//printf("%u\n", frame);
}

bool BoundaryCheck(int h, int w)
{
	return h < height && h > 0 && w < width && w > 0;
}

void AddToCanvasIsNotExists(int h, int w)
{
	if(std::find(nowCanvas.begin(), nowCanvas.end() ,vec2(w, h)) == nowCanvas.end()){
		nowCanvas.push_back(vec2(w, h));
		bristle[h][w].ink = 0.;
		bristle[h][w].color.x = 0.;
		bristle[h][w].color.y = 0.;
		bristle[h][w].color.z = 0.;
		bristle[h][w].color.w = 1.;
		printf("Add new Entry %d %d \n", w, h);
	}
}

void diffuseInk(int h, int w, int i)
{
	switch(i){
		case 0:
			if(BoundaryCheck(h, w + 1) && bristle[h][w].ink > bristle[h][w+1].ink){
				AddToCanvasIsNotExists(h, w + 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h][w + 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;

		case 1:
			if(BoundaryCheck(h, w - 1) && bristle[h][w].ink > bristle[h][w-1].ink){
				AddToCanvasIsNotExists(h, w - 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h][w - 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 2:
			if(BoundaryCheck(h + 1, w) && bristle[h][w].ink > bristle[h + 1][w].ink){
				AddToCanvasIsNotExists(h + 1, w);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h + 1][w].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 3:
			if(BoundaryCheck(h - 1, w) && bristle[h][w].ink > bristle[h - 1][w].ink){
				AddToCanvasIsNotExists(h - 1, w);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h - 1][w].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 4:
			if(BoundaryCheck(h + 1, w + 1) && bristle[h][w].ink > bristle[h + 1][w + 1].ink){
				AddToCanvasIsNotExists(h + 1, w + 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h + 1][w + 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 5:
			if(BoundaryCheck(h - 1, w - 1) && bristle[h][w].ink > bristle[h - 1][w - 1].ink){
				AddToCanvasIsNotExists(h - 1, w - 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h - 1][w - 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 6:
			if(BoundaryCheck(h + 1, w - 1) && bristle[h][w].ink > bristle[h + 1][w - 1].ink){
				AddToCanvasIsNotExists(h + 1, w - 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h + 1][w - 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		case 7:
			if(BoundaryCheck(h - 1, w + 1) && bristle[h][w].ink > bristle[h - 1][w + 1].ink){
				AddToCanvasIsNotExists(h - 1, w + 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * diffuseAmmount;
				bristle[h - 1][w + 1].ink += (bristle[h][w].ink / 8) * diffuseAmmount;
			}
			break;
		default:
			break;
	}
}

void MainDisplay()
{
	// if now in drag node
	if(isDrag){
		glPointSize(dragPointSize);
		glColor4f(1,0,0,1);
		for(int i = 0; i < nowDragged.size(); ++i){
			int w = nowDragged[i].x;
			int h = nowDragged[i].y;
			glBegin(GL_POINTS);
				glVertex2f(
					(w - width/2) / (float)width * 2.0, 
					(h - height/2) / (float)height * -2.0
				);
			glEnd();
		}
	}
	
	glPointSize(1);
	for(int i = 0; i < nowCanvas.size(); ++i){
		int w = nowCanvas[i].x;
		int h = nowCanvas[i].y;
		//printf("Draw: %f %f\n", nowCanvas[i].x, nowCanvas[i].y);
			
		glColor4f(bristle[h][w].color.x, bristle[h][w].color.y, bristle[h][w].color.z, 1 - bristle[h][w].ink);
		glBegin(GL_POINTS);
			glVertex2f(
				(w - width/2) / (float)width * 2.0, 
				(h - height/2) / (float)height * -2.0
			);
		glEnd();
	}
}

void mouseDrags(int x, int y)
{
	if(BoundaryCheck(y, x)){
		nowDragged.push_back(vec2(x,y));
		glutPostRedisplay();	
	}
}

void mouseClicks(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        printf("Left Click Down, AT (%d %d)\n", x, y);
        
        if(BoundaryCheck(y, x)){
			if(find(nowCanvas.begin(), nowCanvas.end(), vec2(x, y)) == nowCanvas.end()){
        		bristle[y][x].ink = 1.;
				bristle[y][x].color.x = 0.;
				bristle[y][x].color.y = 0.;
				bristle[y][x].color.z = 0.;
				bristle[y][x].color.w = 1.;
				nowCanvas.push_back(vec2(x, y));	
        	}
        	else{
        		bristle[y][x].ink += 1.;
        	}
			glutPostRedisplay();
		}
		
		isDrag = true;
        
    }
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        printf("Left Click Up, AT (%d %d)\n", x, y);
        isDrag = false;

        for(int i = 0; i < nowDragged.size(); ++i){
        	int x = nowDragged[i].x;
        	int y = nowDragged[i].y;


        	if(find(nowCanvas.begin(), nowCanvas.end(), vec2(x, y)) == nowCanvas.end()){
        		bristle[y][x].ink = 1.;
				bristle[y][x].color.x = 0.;
				bristle[y][x].color.y = 0.;
				bristle[y][x].color.z = 0.;
				bristle[y][x].color.w = 1.;
				nowCanvas.push_back(vec2(x, y));	
        	}
        	else{
        		bristle[y][x].ink += 1.;
        	}    		
        }

        // Clear Buffer that Stored points in last drag
        nowDragged.clear();
    }

    glutPostRedisplay();
}

void postDealKey(){

	int key = -1;
	
	switch(key){
			case 'w':
				break;
			case 's':
				break;
			case 'a':
				break;
			case 'd':
				break;
			case 'q':
			default : break;
		}
	
}

void keyboard(unsigned char key, int x, int y)
{
	postDealKey();
}

void keyboardUp(unsigned char key, int x, int y)
{
	postDealKey();
}
