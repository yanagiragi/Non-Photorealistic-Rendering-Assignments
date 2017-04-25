#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GL/glut.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <cmath>

#define MAX_FRAME 4294697294

using namespace glm;

// Variables

struct Bristle{
	double ink; 				// 水的含量
	double wetpigment;			// 乾的顏料的含量
	double drypigment;			// 濕的顏料的含量
	vec3 color;					// 顏色
};

typedef enum{
	CYAN,
	MAGENTA,
	YELLOW
} COLOR;

COLOR nowcolor = MAGENTA;

bool previewmode = true; 		// 要不要開始 擴散
bool mode = true; 				// true for wet on wet
bool debug = false;				// 顯示黑線
bool isDrag = false;

vec2 prevPixel = vec2(-1, -1);

double pigmentContrast = 1;		// 計算顏色時 用來讓透明度 增加 對比度的係數
double inkLossAmmount = 1; 		// 顏料隨著筆劃流下的常數 （數字愈大流下愈多）
double pigmentThreshold = 0.3; 	// 顏料儲存容量
double dryAmmount = 0.001; 		// 變乾的速度
double dryThreshold = 0.1; 		// 剩餘墨水不發散 的limit
double waterDiffuseAmmount = 1; 	// 墨水發散係數
double pigmentDiffuseAmmount = .8; 	// 顏料發散係數

unsigned short int dragPointSize = 1; 		// 繪畫時預覽用的筆刷大小
unsigned short int paintPointSize = 15;		// 繪畫時用的筆刷大小

unsigned int strokeID = 0;
unsigned int frame = 0;

//vec3 layerRatio = vec3(0.8, 0.02, 0.18); // <水：濕顏料：乾顏料> 的比例
//vec3 layerRatio = vec3(0.7, 0.05, 0.25); // <水：濕顏料：乾顏料> 的比例
vec3 layerRatio = vec3(0.3, 0.65, 0.05); // <水：濕顏料：乾顏料> 的比例

std::vector<vec3> nowDragged;
std::vector<vec3> nowCanvas;

const int width = 200;
const int height = 200;

struct Bristle bristle[height][width];

std::vector<vec2> HydraPoint;					// 存 交界點
std::vector<vec3> strokeColorContainercolor;    // 存Stroke ID 對應的顏色

// GLs
void initGL();
void display();
void reshape(GLsizei, GLsizei);
void idle();

void MainDisplay();
vec3 ColorBlend(vec3 color1, vec3 color2, double ratio);
void diffuseInk(int h, int w, int i, int id);
void diffuseInkMinor(int old_h,int old_w, int new_h, int new_w, int id);
bool BoundaryCheck(int h, int w);
void AddToCanvasIsNotExists(int h, int w, int id);

// Events
void mouseDrags(int, int);
void mouseClicks(int, int, int, int);
void keyboard(unsigned char key, int x, int y);

/* Main function: GLUT runs as a console application starting at main() */
int main(int argc, char** argv)
{
	//Initialize
	for(int i = 0; i < height; ++i){
		for(int j = 0; j < width; ++j){
			bristle[i][j].ink = 0.;
			bristle[i][j].color = vec3(1.,1.,1.);
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

vec3 ColorBlend(vec3 color1, vec3 color2, double ratio){
	printf("Blending <%f,%f,%f> with <%f,%f,%f> , ratio = %f\n", color1.x, color1.y, color1.z, color2.x, color2.y, color2.z, ratio );
	vec3 tmp;
	tmp.x = color1.x * ratio + color2.x * (1 - ratio);
	tmp.y = color1.y * ratio + color2.y * (1 - ratio);
	tmp.z = color1.z * ratio + color2.z * (1 - ratio);
	return tmp;
}

void idle()
{
	if(frame >= MAX_FRAME){
		frame = 0;
	}
	else{
		++frame;
	}
	if(!previewmode){
		if(frame % 1800 == 0){
			printf("Ding!\n");

			for(int i = 0; i < nowCanvas.size(); ++i){
				
				int w = nowCanvas[i].x;
				int h = nowCanvas[i].y;
				int id = nowCanvas[i].z;
				
				for(int j = 0; j < 8; ++j){
					if((bristle[h][w].ink + bristle[h][w].wetpigment) > dryThreshold){
						diffuseInk(h, w, j, id);
					}
				}
			}
			glutPostRedisplay();
		}
	}
}

bool BoundaryCheck(int h, int w)
{
	return h < height && h > 0 && w < width && w > 0;
}

void AddToCanvasIsNotExists(int h, int w, int id = -1)
{
	if(id == -1)
		nowCanvas.push_back(vec3(w, h, static_cast<double>(strokeID - 1)));
	else
		nowCanvas.push_back(vec3(w, h, static_cast<double>(id)));

	bristle[h][w].ink = 0.;
	bristle[h][w].drypigment = 0.;
	bristle[h][w].wetpigment = 0.;
}

void diffuseInkMinor(int old_h,int old_w, int new_h, int new_w, int id)
{
	double blendRatio = .5;
	vec3 tmpcolor = vec3(1,1,1);
	
	if(!mode){

		switch(nowcolor){
			case CYAN:
				tmpcolor.x = 0.;
				tmpcolor.y = 1.;
				tmpcolor.z = 1.;
				break;
			case MAGENTA:
				tmpcolor.x = 1.;
				tmpcolor.y = 0.;
				tmpcolor.z = 1.;
				break;
			default:
				tmpcolor.x = 1.;
				tmpcolor.y = 1.;
				tmpcolor.z = 0.;
				break;
		};

		tmpcolor = strokeColorContainercolor[id];
		
		// Find HydraPoint
		for(int tmpindex = 0, Collisionid = -1; tmpindex < HydraPoint.size(); ++tmpindex){
			if(HydraPoint[tmpindex].x == id){
				Collisionid = HydraPoint[tmpindex].y;
				printf("Collision with stroke id %d\n", Collisionid);
				tmpcolor = strokeColorContainercolor[Collisionid];//ColorBlend(tmpcolor, strokeColorContainercolor[Collisionid], 0.5);
				//strokeColorContainercolor[Collisionid] = ColorBlend(strokeColorContainercolor[id], strokeColorContainercolor[Collisionid], 0.5);
				
			}
			else if(HydraPoint[tmpindex].y == id){
				Collisionid = HydraPoint[tmpindex].x;
				printf("Collision with stroke id %d\n", Collisionid);
				tmpcolor = strokeColorContainercolor[Collisionid];//ColorBlend(tmpcolor, strokeColorContainercolor[Collisionid], 0.5);
				//strokeColorContainercolor[Collisionid] = ColorBlend(strokeColorContainercolor[id], strokeColorContainercolor[Collisionid], 0.5);
			}
		}
	}
	else{

		tmpcolor = ColorBlend(vec3(1,1,1), bristle[old_h][old_w].color, 0.5);
	}

	bristle[new_h][new_w].color = tmpcolor;

	bristle[old_h][old_w].ink -= (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);
	bristle[new_h][new_w].ink += (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);	

	double pigmentDiffuseAmmount = waterDiffuseAmmount * ((layerRatio.y + layerRatio.z)  / layerRatio.x );

	bristle[old_h][old_w].wetpigment -= (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y / layerRatio.x);
	bristle[new_h][new_w].wetpigment += (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y/ layerRatio.x);

	// dryPigment comes for wetPigment in same bristle
	bristle[new_h][new_w].drypigment += (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
	bristle[new_h][new_w].wetpigment -= (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);	
}

void diffuseInk(int h, int w, int i,int id)
{
	switch(i){
		case 0:
			if(BoundaryCheck(h, w + 1) && bristle[h][w].ink > bristle[h][w+1].ink){
				AddToCanvasIsNotExists(h, w + 1, id);
				diffuseInkMinor(h, w, h, w + 1, id);
			}
			break;

		case 1:
			if(BoundaryCheck(h, w - 1) && bristle[h][w].ink > bristle[h][w-1].ink){
				AddToCanvasIsNotExists(h, w - 1, id);
				diffuseInkMinor(h, w, h, w - 1, id);
			}
			break;
		case 2:
			if(BoundaryCheck(h + 1, w) && bristle[h][w].ink > bristle[h + 1][w].ink){
				AddToCanvasIsNotExists(h + 1, w,id);
				diffuseInkMinor(h, w, h + 1, w, id);
			}
			break;
		case 3:
			if(BoundaryCheck(h - 1, w) && bristle[h][w].ink > bristle[h - 1][w].ink){
				AddToCanvasIsNotExists(h - 1, w, id);
				diffuseInkMinor(h, w, h - 1, w, id);
			}
			break;
		case 4:
			if(BoundaryCheck(h + 1, w + 1) && bristle[h][w].ink > bristle[h + 1][w + 1].ink){
				AddToCanvasIsNotExists(h + 1, w + 1, id);
				diffuseInkMinor(h, w, h + 1, w + 1, id);
			}
			break;
		case 5:
			if(BoundaryCheck(h - 1, w - 1) && bristle[h][w].ink > bristle[h - 1][w - 1].ink){
				AddToCanvasIsNotExists(h - 1, w - 1, id);
				diffuseInkMinor(h, w, h - 1, w - 1, id);
			}
			break;
		case 6:
			if(BoundaryCheck(h + 1, w - 1) && bristle[h][w].ink > bristle[h + 1][w - 1].ink){
				AddToCanvasIsNotExists(h + 1, w - 1, id);
				diffuseInkMinor(h, w, h + 1, w - 1, id);
			}
			break;
		case 7:
			if(BoundaryCheck(h - 1, w + 1) && bristle[h][w].ink > bristle[h - 1][w + 1].ink){
				AddToCanvasIsNotExists(h - 1, w + 1, id);
				diffuseInkMinor(h, w, h - 1, w + 1, id);
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
	
	glPointSize(paintPointSize);
	for(int i = 0; i < nowCanvas.size(); ++i){
		int w = nowCanvas[i].x;
		int h = nowCanvas[i].y;
		
		glColor4f(
			bristle[h][w].color.x,
			bristle[h][w].color.y,
			bristle[h][w].color.z,
			(bristle[h][w].wetpigment + bristle[h][w].drypigment + bristle[h][w].ink) * pigmentContrast
			
		);
		
		glBegin(GL_POINTS);
			glVertex2f(
				(w - width/2) / (float)width * 2.0, 
				(h - height/2) / (float)height * -2.0
			);
		glEnd();
	}


	if(debug){
		// For Debug
		glPointSize(1);
		for(int i = 0; i < nowCanvas.size(); ++i){
			int w = nowCanvas[i].x;
			int h = nowCanvas[i].y;
			glColor4f(0,0,0,1);		
			glBegin(GL_POINTS);
				glVertex2f(
					(w - width/2) / (float)width * 2.0, 
					(h - height/2) / (float)height * -2.0
				);
			glEnd();
		}
	}
}

void mouseDrags(int old_x, int old_y)
{
	for(int j = 1; j < paintPointSize / 2; ++j){
		int x = old_x;
		int y = old_y;
		if(BoundaryCheck(y, x)){
			int i;
			for(i = 0; i < nowDragged.size(); ++i){
				if(nowDragged[i].x == x && nowDragged[i].y == y)
					break;
			}

			if(i < nowDragged.size()) { // Found
				++(nowDragged[i].z);
			}
			else{
				nowDragged.push_back(vec3(x,y, 1.));
				if(prevPixel.x == -1 || prevPixel.y == -1){
					prevPixel = vec2(x,y);
				}
				else{
					vec2 Disvector;
					Disvector.x = prevPixel.x - x;
					Disvector.y = prevPixel.y - y;

					int incre = Disvector.y / Disvector.x;

					for(int tmp = prevPixel.x; tmp < x; ++tmp){
						if(BoundaryCheck( (int) (prevPixel.y + incre * (tmp - x)), (int)tmp)){
							nowDragged.push_back(vec3((int)tmp, (int) (prevPixel.y + incre * (tmp - x)), 1.));
						}
					}
					prevPixel = vec2(x,y);
				}
			}

			glutPostRedisplay();
		}
	}
}

void mouseClicks(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        printf("Left Click Down, AT (%d %d)\n", x, y);
        isDrag = true;
    }
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        printf("Left Click Up, AT (%d %d), strokeID = %d\n", x, y, strokeID);

        
        isDrag = false;

        double sum = 0, nowcount = 0;
        
		for(int i = 0; i < nowDragged.size(); ++i){
        	sum += nowDragged[i].z;
        }

		vec3 tmpcolor;
		switch(nowcolor){
			case CYAN:
				tmpcolor.x = 0.;
				tmpcolor.y = 1.;
				tmpcolor.z = 1.;
				break;
			case MAGENTA:
				tmpcolor.x = 1.;
				tmpcolor.y = 0.;
				tmpcolor.z = 1.;
				break;
			default:
				tmpcolor.x = 1.;
				tmpcolor.y = 1.;
				tmpcolor.z = 0.;
				break;
		}

		strokeColorContainercolor.push_back(tmpcolor);
		++strokeID;
        
		for(int i = 0; i < nowDragged.size(); ++i){
			
			bristle[y][x].color = tmpcolor;

			for(int j = 0; j < nowCanvas.size(); ++j){
				if(nowDragged[i].x == nowCanvas[j].x && nowDragged[i].y == nowCanvas[j].y){
					printf("All Hail Hydra\n");
					printf("Add new HydraPair<%d, %f>\n",strokeID - 1, nowCanvas[j].z);
					HydraPoint.push_back(vec2(static_cast<double>(strokeID - 1), nowCanvas[j].z));

				}
			}
		}


		for(int i = 0; i < nowDragged.size(); ++i){
        	int x = nowDragged[i].x;
        	int y = nowDragged[i].y;
        	double count = static_cast<double>(nowDragged[i].z);
			
			AddToCanvasIsNotExists(y, x);
			
			bristle[y][x].color = tmpcolor;
			// In here Ink Contains Water, WetPigment, DryPigment
			double tmpInk = (1 - nowcount / sum) * inkLossAmmount;
			bristle[y][x].ink += tmpInk * layerRatio.x;
			bristle[y][x].wetpigment += tmpInk * layerRatio.y;
			bristle[y][x].drypigment += tmpInk * layerRatio.z;

			nowcount += count;
        }

        // Clear Buffer that Stored points in last drag
        nowDragged.clear();
		prevPixel.x = prevPixel.y = -1;
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key){
		case 'c':
			nowcolor = CYAN;
			break;
		case 'm':
			nowcolor = MAGENTA;
			break;
		case 'y':
			nowcolor = YELLOW;
			break;
		case 'd':
			printf("============================\nSwitch to %s Mode\n============================\n", (previewmode) ? "Debug" : "Normal");
			debug = !debug;
			break;
		case 'a':
			printf("============================\nClear Canvas\n============================\n");
			prevPixel.x = prevPixel.y = -1;
			nowCanvas.clear();
			break;
		case 'z':
			printf("============================\nSwitch to %s Mode\n============================\n", (previewmode) ? "Draw" : "Preview");
			previewmode = !previewmode;
			break;
		case 'x':
			printf("============================\nSwitch to %s Mode\n============================\n", (mode) ? "Diffuse" : "Wet");
			mode = !mode;
			break;
		default : break;
	}
}