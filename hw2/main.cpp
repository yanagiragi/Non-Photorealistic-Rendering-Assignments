#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GL/glut.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <cmath>

#define MAX_FRAME 4294697295

/*
	建立3層模型
	做Color Blending
	處理Wet on wet, wet on dry
*/

using namespace glm;

// Variables

struct Bristle{
	double ink; 		// 水的含量
	double wetpigment;
	double drypigment;
	vec4 color;
};

//vec3 layerRatio = vec3(0.8, 0.02, 0.18); // <水：濕顏料：乾顏料> 的比例
vec3 layerRatio = vec3(0.5, 0.20, 0.30); // <水：濕顏料：乾顏料> 的比例
double pigmentContrast = 5;		// 計算顏色時 用來讓透明度 增加 對比度的係數
double inkLossAmmount = 1; 		// 顏料隨著筆劃流下的常數 （數字愈大流下愈多）

double pigmentThreshold = 0.3; 	// 顏料儲存容量
double dryAmmount = 0.001; 		// 變乾的速度
double dryThreshold = 0.1; 		// 剩餘墨水不發散 的limit

double waterDiffuseAmmount = 0.8; 	// 墨水發散係數
double pigmentDiffuseAmmount = 1; 	// 墨水發散係數

unsigned short int dragPointSize = 1; // 繪畫時預覽用的筆刷大小


unsigned char colorBuffer[4];
unsigned int frame = 0;
bool isDrag = false;
std::vector<vec3> nowDragged;
std::vector<vec2> nowCanvas;

const int width = 200;
const int height = 200;

struct Bristle bristle[height][width];


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

// Events
void mouseDrags(int, int);
void mouseClicks(int, int, int, int);
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
	glutReshapeFunc(reshape);							// Register callback handler for window re-size event
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
void reshape(GLsizei w, GLsizei h) {  // GLsizei for non-negative integer
   glutReshapeWindow(width, height);
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

	if(frame % 180 == 0){
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
			/*if(bristle[h][w].ink > 0 && bristle[h][w].pigment < pigmentThreshold){
				bristle[h][w].ink -= dryAmmount;
				bristle[h][w].pigment += dryAmmount;
			}*/
			
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
		bristle[h][w].drypigment = 0.;
		bristle[h][w].wetpigment = 0.;
		bristle[h][w].color.x = 1.;
		bristle[h][w].color.y = 0.;
		bristle[h][w].color.z = 0.;
		bristle[h][w].color.w = 1.;
		printf("Add new Entry %d %d \n", w, h);
	}
}

void diffuseInkMinor(int old_h,int old_w, int new_h, int new_w)
{
	bristle[old_h][old_w].ink -= (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);
	bristle[new_h][new_w].ink += (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);

	double pigmentDiffuseAmmount = waterDiffuseAmmount * ((layerRatio.y + layerRatio.z)  / layerRatio.x );

	bristle[old_h][old_w].wetpigment -= (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y / layerRatio.x);
	bristle[new_h][new_w].wetpigment += (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y/ layerRatio.x);

	// dryPigment comes for wetPigment in same bristle
	bristle[new_h][new_w].drypigment += (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
	bristle[new_h][new_w].wetpigment -= (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
}

void diffuseInk(int h, int w, int i)
{
	switch(i){
		case 0:
			if(BoundaryCheck(h, w + 1) && bristle[h][w].ink > bristle[h][w+1].ink){
				AddToCanvasIsNotExists(h, w + 1);
				diffuseInkMinor(h, w, h, w + 1);
				/*
				bristle[h][w].ink -= (bristle[h][w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);
				bristle[h][w + 1].ink += (bristle[h][w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);
				
				double pigmentDiffuseAmmount = waterDiffuseAmmount * ((layerRatio.y + layerRatio.z)  / layerRatio.x );

				bristle[h][w].wetpigment -= (bristle[h][w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y / layerRatio.x);
				bristle[h][w+1].wetpigment += (bristle[h][w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y/ layerRatio.x);

				// dryPigment comes for wetPigment in same bristle
				bristle[h][w+1].drypigment += (bristle[h][w+1].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
				bristle[h][w+1].wetpigment -= (bristle[h][w+1].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
				*/

			}
			break;

		case 1:
			if(BoundaryCheck(h, w - 1) && bristle[h][w].ink > bristle[h][w-1].ink){
				AddToCanvasIsNotExists(h, w - 1);
				diffuseInkMinor(h, w, h, w - 1);
			}
			break;
		case 2:
			if(BoundaryCheck(h + 1, w) && bristle[h][w].ink > bristle[h + 1][w].ink){
				AddToCanvasIsNotExists(h + 1, w);
				diffuseInkMinor(h, w, h + 1, w);
			}
			break;
		case 3:
			if(BoundaryCheck(h - 1, w) && bristle[h][w].ink > bristle[h - 1][w].ink){
				AddToCanvasIsNotExists(h - 1, w);
				diffuseInkMinor(h, w, h - 1, w);
			}
			break;
		/*case 4:
			if(BoundaryCheck(h + 1, w + 1) && bristle[h][w].ink > bristle[h + 1][w + 1].ink){
				AddToCanvasIsNotExists(h + 1, w + 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * waterDiffuseAmmount;
				bristle[h + 1][w + 1].ink += (bristle[h][w].ink / 8) * waterDiffuseAmmount;
			}
			break;
		case 5:
			if(BoundaryCheck(h - 1, w - 1) && bristle[h][w].ink > bristle[h - 1][w - 1].ink){
				AddToCanvasIsNotExists(h - 1, w - 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * waterDiffuseAmmount;
				bristle[h - 1][w - 1].ink += (bristle[h][w].ink / 8) * waterDiffuseAmmount;
			}
			break;
		case 6:
			if(BoundaryCheck(h + 1, w - 1) && bristle[h][w].ink > bristle[h + 1][w - 1].ink){
				AddToCanvasIsNotExists(h + 1, w - 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * waterDiffuseAmmount;
				bristle[h + 1][w - 1].ink += (bristle[h][w].ink / 8) * waterDiffuseAmmount;
			}
			break;
		case 7:
			if(BoundaryCheck(h - 1, w + 1) && bristle[h][w].ink > bristle[h - 1][w + 1].ink){
				AddToCanvasIsNotExists(h - 1, w + 1);
				bristle[h][w].ink -= (bristle[h][w].ink / 8) * waterDiffuseAmmount;
				bristle[h - 1][w + 1].ink += (bristle[h][w].ink / 8) * waterDiffuseAmmount;
			}
			break;*/
		default:
			break;
	}
}

void MainDisplay()
{
	// if now in drag node
	if(isDrag){
		glPointSize(dragPointSize);
		glColor4f(1,0,0,1); // preview color: red
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
	
	glPointSize(5);
	for(int i = 0; i < nowCanvas.size(); ++i){
		int w = nowCanvas[i].x;
		int h = nowCanvas[i].y;
		
		//printf("Draw: %f %f %f\n", nowCanvas[i].x, nowCanvas[i].y, bristle[h][w].wetpigment + bristle[h][w].drypigment);
		
		// TODO: Blend Color with dry & wet Color	
		/*glColor4f(
			bristle[h][w].color.x,
			bristle[h][w].color.y,
			bristle[h][w].color.z,
			(bristle[h][w].wetpigment + bristle[h][w].drypigment) * pigmentContrast
		);*/

		glColor4f(
			bristle[h][w].color.x,
			bristle[h][w].color.y,
			bristle[h][w].color.z,
			1//(bristle[h][w].wetpigment + bristle[h][w].drypigment) * pigmentContrast
		);
		
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
		
		int i;
		for(i = 0; i < nowDragged.size(); ++i){
			if(nowDragged[i].x == x && nowDragged[i].y == y)
				break;
		}

		if(i < nowDragged.size()) { // Found
			++nowDragged[i].z;
			// printf("%s\n", "Found");
		}
		else{
			nowDragged.push_back(vec3(x,y,1.));
		}

		glutPostRedisplay();
	}
}

void mouseClicks(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        printf("Left Click Down, AT (%d %d)\n", x, y);
        
        /*if(BoundaryCheck(y, x)){
			AddToCanvasIsNotExists(y, x);        	
        	bristle[y][x].ink += 1.;        	
			glutPostRedisplay();
		}*/
		
		isDrag = true;
    }
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        printf("Left Click Up, AT (%d %d)\n", x, y);
        
        isDrag = false;

        double sum = 0, nowcount = 0;
        
		for(int i = 0; i < nowDragged.size(); ++i){
        	sum += nowDragged[i].z;
        }

		for(int i = 0; i < nowDragged.size(); ++i){
        	int x = nowDragged[i].x;
        	int y = nowDragged[i].y;
        	double count = static_cast<double>(nowDragged[i].z);
			
			AddToCanvasIsNotExists(y, x);
			
			// In here Ink Contains Water, WetPigment, DryPigment
			bristle[y][x].ink += (1 - nowcount / sum) * inkLossAmmount;
			
			bristle[y][x].wetpigment = bristle[y][x].ink * layerRatio.y;
			bristle[y][x].drypigment = bristle[y][x].ink * layerRatio.z;
			bristle[y][x].ink = bristle[y][x].ink * layerRatio.x;

			nowcount += count;
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
