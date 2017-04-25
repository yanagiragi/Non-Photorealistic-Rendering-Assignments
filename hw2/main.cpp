#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GL/glut.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <cmath>

#define MAX_FRAME 4294697294

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
	vec3 color;
	std::vector<vec3> strokeColor; // 這個 點 過去畫上stroke的顏色
};

std::vector<int> strokeColorContainer;

typedef enum{
	CYAN,
	MAGENTA,
	YELLOW
} COLOR;

COLOR nowcolor = MAGENTA;
bool previewmode = true; // true for wet on wet
bool mode = true; // true for wet on wet
double loss = 0.01;

//vec3 layerRatio = vec3(0.8, 0.02, 0.18); // <水：濕顏料：乾顏料> 的比例
//vec3 layerRatio = vec3(0.7, 0.05, 0.25); // <水：濕顏料：乾顏料> 的比例
vec3 layerRatio = vec3(0.3, 0.65, 0.05); // <水：濕顏料：乾顏料> 的比例
double pigmentContrast = 1;		// 計算顏色時 用來讓透明度 增加 對比度的係數
double inkLossAmmount = 1; 		// 顏料隨著筆劃流下的常數 （數字愈大流下愈多）

double pigmentThreshold = 0.3; 	// 顏料儲存容量
double dryAmmount = 0.001; 		// 變乾的速度
double dryThreshold = 0.1; 		// 剩餘墨水不發散 的limit

double waterDiffuseAmmount = 1; 	// 墨水發散係數
double pigmentDiffuseAmmount = .8; 	// 墨水發散係數

unsigned short int dragPointSize = 1; // 繪畫時預覽用的筆刷大小
unsigned short int paintPointSize = 15;

unsigned int strokeID = 0;

unsigned char colorBuffer[5000][3];
//unsigned char colorBuffer[3];
unsigned int frame = 0;
bool isDrag = false;
std::vector<vec3> nowDragged;
std::vector<vec3> nowCanvas;

const int width = 200;
const int height = 200;

struct Bristle bristle[height][width];
struct Bristle An_bristle[height][width];

std::vector<vec2> HedryaPoint;
std::vector<vec3> strokeColorContainercolor;

// Mathmatics
float clamp(float, float, float);

// GLs
void initGL();
void display();
void reshape(GLsizei, GLsizei);
void idle();

void MainDisplay();
void diffuseInk(int h, int w, int i, int id);
bool BoundaryCheck(int h, int w);
void AddToCanvasIsNotExists(int h, int w, int);

// Events
void mouseDrags(int, int);
void mouseClicks(int, int, int, int);
void keyboard(unsigned char key, int x, int y);

/* Main function: GLUT runs as a console application starting at main() */
int main(int argc, char** argv) {

	for(int i = 0; i < height; ++i){
		for(int j = 0; j < width; ++j){
			//bristle[i][j].size = 0.;
			bristle[i][j].ink = 0.;
			bristle[i][j].color = vec3(1.,1.,1.);

			An_bristle[i][j].ink = 0.;
			An_bristle[i][j].color = vec3(1.,1.,1.);
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
	if(!previewmode){
		if(frame % 1800 == 0){
			printf("Ding!\n");

			for(int i = 0; i < nowCanvas.size(); ++i){
				
				int w = nowCanvas[i].x;
				int h = nowCanvas[i].y;
				int id = nowCanvas[i].z;
				// printf("Now: %d %d : %f, dryThreshold = %f\n", h, w, bristle[h][w].ink, dryThreshold);
				
				for(int j = 0; j < 8; ++j){
					if((bristle[h][w].ink + bristle[h][w].wetpigment) > dryThreshold){
						//printf("%d %d : %f\n", h, w, (bristle[h][w].ink + bristle[h][w].wetpigment));
						diffuseInk(h, w, j, id);
					}
				}
				
				/*if(bristle[h][w].ink > 0 && bristle[h][w].pigment < pigmentThreshold){
					bristle[h][w].ink -= dryAmmount;
					bristle[h][w].pigment += dryAmmount;
				}*/
				
			}
			//printf("Done\n");
			glutPostRedisplay();
		}
	}

	
	
	//printf("%u\n", frame);
}

bool BoundaryCheck(int h, int w)
{
	return h < height && h > 0 && w < width && w > 0;
}

void AddToCanvasIsNotExists(int h, int w, int id = -1)
{
	//if(std::find(nowCanvas.begin(), nowCanvas.end() ,vec2(w, h)) == nowCanvas.end()){
		//printf("|%f|%f|\n",static_cast<double>(strokeID - 1), static_cast<double>(id));
		if(id == -1)
			nowCanvas.push_back(vec3(w, h, static_cast<double>(strokeID - 1)));
		else
			nowCanvas.push_back(vec3(w, h, static_cast<double>(id)));
		bristle[h][w].ink = 0.;
		bristle[h][w].drypigment = 0.;
		bristle[h][w].wetpigment = 0.;
		
		/*bristle[h][w].color.x = 1.;
		bristle[h][w].color.y = 0.;
		bristle[h][w].color.z = 0.;*/
		/*if(mode){
			if(bristle[h][w].ink > 1){
				bristle[h][w].color.x = 1.;
				bristle[h][w].color.y = 0.;
				bristle[h][w].color.z = 0.;
				return;
			}
		}*/
		
		
		/*switch(nowcolor){
			case CYAN:
				bristle[h][w].color.x = 0.;
				bristle[h][w].color.y = 1.;
				bristle[h][w].color.z = 1.;
				break;
			case MAGENTA:
				bristle[h][w].color.x = 1.;
				bristle[h][w].color.y = 0.;
				bristle[h][w].color.z = 1.;
				break;
			default:
				bristle[h][w].color.x = 1.;
				bristle[h][w].color.y = 1.;
				bristle[h][w].color.z = 0.;
				break;
		}*/

	
		

		//bristle[h][w].color.w = 1.;
		//printf("Add new Entry %d %d \n", w, h);
	//}
}

void diffuseInkMinor(int old_h,int old_w, int new_h, int new_w, int id)
{
	/*if(bristle[old_h][old_w].ink >= 1){
		switch(nowcolor){
			case CYAN:
				bristle[new_h][new_w].color.x = 0.;
				bristle[new_h][new_w].color.y = 1.;
				bristle[new_h][new_w].color.z = 1.;
				break;
			case MAGENTA:
				bristle[new_h][new_w].color.x = 1.;
				bristle[new_h][new_w].color.y = 0.;
				bristle[new_h][new_w].color.z = 1.;
				break;
			default:
				bristle[new_h][new_w].color.x = 1.;
				bristle[new_h][new_w].color.y = 1.;
				bristle[new_h][new_w].color.z = 0.;
				break;
		}
				bristle[new_h][new_w].color.x = 1.;
				bristle[new_h][new_w].color.y = 1.;
				bristle[new_h][new_w].color.z = 1.;
	}
	else{
				bristle[new_h][new_w].color.x = 0.;
				bristle[new_h][new_w].color.y = 0.;
				bristle[new_h][new_w].color.z = 0.;
	}*/

	/*bristle[new_h][new_w].color.x = bristle[old_h][old_w].color.x;
	bristle[new_h][new_w].color.y = bristle[old_h][old_w].color.y;
	bristle[new_h][new_w].color.z = bristle[old_h][old_w].color.z;*/
	/*bristle[new_h][new_w].color.x = An_bristle[new_h][new_w].color.x;
	bristle[new_h][new_w].color.y = An_bristle[new_h][new_w].color.y;
	bristle[new_h][new_w].color.z = An_bristle[new_h][new_w].color.z;*/
	//return;	

		double blendRatio = .5;
		vec3 tmpcolor = vec3(1,1,1);
		int h = new_h;
		int w = new_w;

		if(!mode){



		/*if(An_bristle[new_h][new_w].strokeColor.size() >= 1){
			for(int i = 0; i < bristle[old_h][old_w].strokeColor.size(); ++i){
				if(i == 0){
					tmpcolor.x = An_bristle[new_h][new_w].strokeColor[i].x;
					tmpcolor.y = An_bristle[new_h][new_w].strokeColor[i].y;
					tmpcolor.z = An_bristle[new_h][new_w].strokeColor[i].z;	
				}
				else{
					tmpcolor.x = tmpcolor.x * blendRatio + An_bristle[new_h][new_w].strokeColor[i].x * (1 - blendRatio);
					tmpcolor.y = tmpcolor.y * blendRatio + An_bristle[new_h][new_w].strokeColor[i].y * (1 - blendRatio);
					tmpcolor.z = tmpcolor.z * blendRatio + An_bristle[new_h][new_w].strokeColor[i].z * (1 - blendRatio);
				}			
			}
		}
		else{*/
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
			//tmpcolor.x = tmpcolor.y = tmpcolor.z = 0;
			printf("Drop down\n");
			//tmpcolor = bristle[old_h][old_w].color;
			tmpcolor = strokeColorContainercolor[id];
			printf("tmpcolor = %f %f %f, id = %d\n", tmpcolor[0], tmpcolor[1], tmpcolor[2], id);

		//}

		// Find HedryaPoint
		for(int tmpindex = 0, Collisionid = -1; tmpindex < HedryaPoint.size(); ++tmpindex){
			if(HedryaPoint[tmpindex].x == id){
				Collisionid = HedryaPoint[tmpindex].y;
				printf("%f %f\n", id, HedryaPoint[tmpindex].y);
				printf("Collision with stroke id %d\n", Collisionid);
				tmpcolor = strokeColorContainercolor[Collisionid];//ColorBlend(tmpcolor, strokeColorContainercolor[Collisionid], 0.5);
				
			}
			else if(HedryaPoint[tmpindex].y == id){
				Collisionid = HedryaPoint[tmpindex].x;
				printf("Collision with stroke id %d\n", Collisionid);
				tmpcolor = strokeColorContainercolor[Collisionid];//ColorBlend(tmpcolor, strokeColorContainercolor[Collisionid], 0.5);
			}
		}


		std::vector<vec3> tmpColorCountBuffer;
		for(int tmpindex = 0; tmpindex < nowCanvas.size(); ++tmpindex){
			if(nowCanvas[tmpindex].x == new_w && nowCanvas[tmpindex].y == new_h){
				//printf("Collision with stroke id %f\n", nowCanvas[tmpindex].z);
				//tmpcolor = ColorBlend(tmpcolor, strokeColorContainercolor[nowCanvas[tmpindex].z], 0.5);
				//tmpcolor = strokeColorContainercolor[nowCanvas[tmpindex].z];
			}
		}

	}
	else{
		tmpcolor = ColorBlend(vec3(1,1,1), bristle[old_h][old_w].color, 0.5);
		//tmpcolor = vec3(0,0,0);
	}

	bristle[new_h][new_w].color = tmpcolor;

	//printf("bristle[h][w].color.x = %f %f %f\n", bristle[h][w].color.x, bristle[h][w].color.y, bristle[h][w].color.z);
	//printf("Diffuse Color = %f %f %f\n", tmpcolor.x, tmpcolor.y, tmpcolor.z);


	bristle[old_h][old_w].ink -= (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);
	bristle[new_h][new_w].ink += (bristle[old_h][old_w].ink / 8.) * waterDiffuseAmmount * (layerRatio.x);

	

	double pigmentDiffuseAmmount = waterDiffuseAmmount * ((layerRatio.y + layerRatio.z)  / layerRatio.x );

	bristle[old_h][old_w].wetpigment -= (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y / layerRatio.x);
	bristle[new_h][new_w].wetpigment += (bristle[old_h][old_w].wetpigment / 8.) * pigmentDiffuseAmmount * (layerRatio.y/ layerRatio.x);

	// dryPigment comes for wetPigment in same bristle
	bristle[new_h][new_w].drypigment += (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);
	bristle[new_h][new_w].wetpigment -= (bristle[new_h][new_w].wetpigment) * pigmentDiffuseAmmount * (layerRatio.z / layerRatio.y);


	// Blend Color When Diffuse
	
	/*glReadPixels(old_w, height - old_h, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colorBuffer);

	if(
		//(colorBuffer[0] + colorBuffer[1] + colorBuffer[2]) < 255 * 3
		//bristle[old_]
		
		true
	){
		printf("sum = %f\n", (bristle[old_h][old_w].ink));
		bristle[new_h][new_w].color.x  = bristle[old_h][old_w].color.x;	
		bristle[new_h][new_w].color.y = bristle[new_h][new_w].color.z = 0;
		//bristle[old_h][old_w].color.x = bristle[old_h][old_w].color.y = bristle[old_h][old_w].color.z = 0;
		double ratio = 1.0;
		//bristle[new_h][new_w].color.x  = bristle[old_h][old_w].color.x * ratio + bristle[new_h][new_w].color.x * (1 - ratio);
		//bristle[new_h][new_w].color.y  = bristle[old_h][old_w].color.y * ratio + bristle[new_h][new_w].color.y * (1 - ratio);
		//bristle[new_h][new_w].color.z  = bristle[old_h][old_w].color.z * ratio + bristle[new_h][new_w].color.z * (1 - ratio);

	}*/

	
}

void updateStrokeColorContainer(int old_h, int old_w, int new_h, int new_w){
	for(int i = 0; i < bristle[old_h][old_w].strokeColor.size(); ++i){
		bristle[new_h][new_w].strokeColor.push_back(bristle[old_h][old_w].strokeColor[i]);
	}
}

void diffuseInk(int h, int w, int i,int id)
{
	switch(i){
		case 0:
			if(BoundaryCheck(h, w + 1) && bristle[h][w].ink > bristle[h][w+1].ink){
				AddToCanvasIsNotExists(h, w + 1, id);
				updateStrokeColorContainer(h, w, h, w + 1);
				diffuseInkMinor(h, w, h, w + 1, id);
			}
			break;

		case 1:
			if(BoundaryCheck(h, w - 1) && bristle[h][w].ink > bristle[h][w-1].ink){
				AddToCanvasIsNotExists(h, w - 1, id);
				updateStrokeColorContainer(h, w, h, w - 1);
				diffuseInkMinor(h, w, h, w - 1, id);
			}
			break;
		case 2:
			if(BoundaryCheck(h + 1, w) && bristle[h][w].ink > bristle[h + 1][w].ink){
				AddToCanvasIsNotExists(h + 1, w,id);
				updateStrokeColorContainer(h, w, h + 1, w);
				diffuseInkMinor(h, w, h + 1, w, id);
			}
			break;
		case 3:
			if(BoundaryCheck(h - 1, w) && bristle[h][w].ink > bristle[h - 1][w].ink){
				AddToCanvasIsNotExists(h - 1, w, id);
				updateStrokeColorContainer(h, w, h - 1, w);
				diffuseInkMinor(h, w, h - 1, w, id);
			}
			break;
		case 4:
			if(BoundaryCheck(h + 1, w + 1) && bristle[h][w].ink > bristle[h + 1][w + 1].ink){
				AddToCanvasIsNotExists(h + 1, w + 1, id);
				updateStrokeColorContainer(h, w, h + 1, w + 1);
				diffuseInkMinor(h, w, h + 1, w + 1, id);
			}
			break;
		case 5:
			if(BoundaryCheck(h - 1, w - 1) && bristle[h][w].ink > bristle[h - 1][w - 1].ink){
				AddToCanvasIsNotExists(h - 1, w - 1, id);
				updateStrokeColorContainer(h, w, h - 1, w - 1);
				diffuseInkMinor(h, w, h - 1, w - 1, id);
			}
			break;
		case 6:
			if(BoundaryCheck(h + 1, w - 1) && bristle[h][w].ink > bristle[h + 1][w - 1].ink){
				AddToCanvasIsNotExists(h + 1, w - 1, id);
				updateStrokeColorContainer(h, w, h + 1, w - 1);
				diffuseInkMinor(h, w, h + 1, w - 1, id);
			}
			break;
		case 7:
			if(BoundaryCheck(h - 1, w + 1) && bristle[h][w].ink > bristle[h - 1][w + 1].ink){
				AddToCanvasIsNotExists(h - 1, w + 1, id);
				updateStrokeColorContainer(h, w, h - 1, w + 1);
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
	//glPointSize(1);
	for(int i = 0; i < nowCanvas.size(); ++i){
		int w = nowCanvas[i].x;
		int h = nowCanvas[i].y;
		
		//printf("Draw: %f %f %f\n", nowCanvas[i].x, nowCanvas[i].y, (bristle[h][w].wetpigment + bristle[h][w].drypigment) * pigmentContrast);

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


	// For Debug
	/*glPointSize(1);
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
	}*/
}

void paintPointWithPointSize(int old_x, int old_y, int increX, int increY)
{
	for(int j = 1; j < paintPointSize / 2; ++j){
		int x = old_x - j * increX;
		int y = old_y - j * increY;
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
				nowDragged.push_back(vec3(x,y,1.));
			}

			glutPostRedisplay();
		}
	}
}

void mouseDrags(int old_x, int old_y)
{
	/*for(int i = -1 * paintPointSize / 2; i < paintPointSize / 2; ++i){
		for(int j = -1 * paintPointSize / 2; j < paintPointSize / 2; ++j){
			paintPointWithPointSize(old_x, old_y, i, j);
		}
	}*/

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
			}

			glutPostRedisplay();
		}
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
					printf("All Hail Hedrya\n");
					printf("%f %f %f\n", 
						bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color.x,
						bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color.y,
						bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color.z
					);
					
					/*bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].strokeColor.push_back(
						bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color
					);*/

					// Find which two stroke collisions
					// nowDragged[i].z
					// strokeID - 1
					printf("Add new HedryaPair<%d, %f>\n",strokeID - 1, nowCanvas[j].z);
					HedryaPoint.push_back(vec2(static_cast<double>(strokeID - 1), nowCanvas[j].z));

					for(int k = 0; k < nowDragged.size(); ++k){
						bristle[(int)nowDragged[k].y][(int)nowDragged[k].x].strokeColor.push_back(
							bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color
						);
					}

				}
			}
			bristle[y][x].strokeColor.push_back(bristle[y][x].color);
		}


		for(int i = 0; i < nowDragged.size(); ++i){
        	int x = nowDragged[i].x;
        	int y = nowDragged[i].y;
        	double count = static_cast<double>(nowDragged[i].z);
			
			AddToCanvasIsNotExists(y, x);
			//printf("Add pair -> %d\n", strokeID - 1);

			switch(nowcolor){
				case CYAN:
					bristle[y][x].color.x = 0.;
					bristle[y][x].color.y = 1.;
					bristle[y][x].color.z = 1.;
					break;
				case MAGENTA:
					bristle[y][x].color.x = 1.;
					bristle[y][x].color.y = 0.;
					bristle[y][x].color.z = 1.;
					break;
				default:
					bristle[y][x].color.x = 1.;
					bristle[y][x].color.y = 1.;
					bristle[y][x].color.z = 0.;
					break;
			};
			
			

			// In here Ink Contains Water, WetPigment, DryPigment
			double tmpInk = (1 - nowcount / sum) * inkLossAmmount;
			bristle[y][x].ink += tmpInk * layerRatio.x;
			bristle[y][x].wetpigment += tmpInk * layerRatio.y;
			bristle[y][x].drypigment += tmpInk * layerRatio.z;

			for(int tmph = paintPointSize / 2 * -1.; tmph < paintPointSize / 2.; ++tmph){
				for(int tmpw = paintPointSize / 2 * -1.; tmpw < paintPointSize / 2; ++ tmpw){
					int tmpx = x + tmpw;
					int tmpy = y + tmph;
					if(BoundaryCheck(tmpy, tmpx)){						
						double tmpInk = (1 - nowcount / sum) * inkLossAmmount;
						An_bristle[tmpy][tmpx].ink += tmpInk * layerRatio.x;
						An_bristle[tmpy][tmpx].wetpigment += tmpInk * layerRatio.y;
						An_bristle[tmpy][tmpx].drypigment += tmpInk * layerRatio.z;
						
						An_bristle[tmpy][tmpx].color.x = An_bristle[tmpy][tmpx].color.x * 0.5 + tmpcolor.x * 0.5;
						An_bristle[tmpy][tmpx].color.y = An_bristle[tmpy][tmpx].color.y * 0.5 + tmpcolor.y * 0.5;
						An_bristle[tmpy][tmpx].color.z = An_bristle[tmpy][tmpx].color.z * 0.5 + tmpcolor.z * 0.5;
					}
				}
			}

			/*for(int j = 0; j < nowCanvas.size(); ++j){
				if(nowDragged[i].x == nowCanvas[j].x && nowDragged[i].y == nowCanvas[j].y ){					
					for(int tmph = paintPointSize / 2 * -1.; tmph < paintPointSize / 2.; ++tmph){
						for(int tmpw = paintPointSize / 2 * -1.; tmpw < paintPointSize / 2; ++ tmpw){
							int tmpx = x + tmpw;
							int tmpy = y + tmph;
							if(BoundaryCheck(tmpy, tmpx)){						
								An_bristle[tmpy][tmpx].color.x = An_bristle[tmpy][tmpx].color.x * 0.5 + tmpcolor.x * 0.5;
								An_bristle[tmpy][tmpx].color.y = An_bristle[tmpy][tmpx].color.y * 0.5 + tmpcolor.y * 0.5;
								An_bristle[tmpy][tmpx].color.z = An_bristle[tmpy][tmpx].color.z * 0.5 + tmpcolor.z * 0.5;
								An_bristle[tmph][tmpw].strokeColor.push_back(
									bristle[(int)nowCanvas[j].y][(int)nowCanvas[j].x].color
								);
							}
						}
					}
				}
			}*/

			/*glReadPixels(x, height - y, paintPointSize, paintPointSize, GL_RGB, GL_UNSIGNED_BYTE, colorBuffer);
			int tmph;
			for(tmph = 0; tmph < paintPointSize *paintPointSize; ++tmph){
				
					if(
						(
							colorBuffer[tmph][0] + 
							colorBuffer[tmph][1] + 
							colorBuffer[tmph][2]
						) != 3 * 255
					){
						printf("%d %d %d\n",
						colorBuffer[tmph][0],
							colorBuffer[tmph][1],
							colorBuffer[tmph][2]
						);
						tmph = paintPointSize + 1;
						break;
					}
				
			}
			
			if(tmph == paintPointSize + 1){
				bristle[y][x].color.x = 0;
				bristle[y][x].color.y = 0;
				bristle[y][x].color.z = 0;
			}
			else *//*if((bristle[y][x].color.x + bristle[y][x].color.y + bristle[y][x].color.z) == 3){
				bristle[y][x].color.x = tmpcolor.x;
				bristle[y][x].color.y = tmpcolor.y;
				bristle[y][x].color.z = tmpcolor.z;
			}			
				bristle[y][x].color.x = tmpcolor.x;
				bristle[y][x].color.y = tmpcolor.y;
				bristle[y][x].color.z = tmpcolor.z;
			/*else{
				printf("Found!");
				//int a;
				//scanf("%d", &a);
			}*/

			//bristle[y][x].color.x = bristle[y][x].color.y = bristle[y][x].color.z = 0;
			

			// For Debug
			//tmpcolor.x = tmpcolor.y = tmpcolor.z = 0;

			

			//printf("colorBuffer = %f %f %f\n",(double)colorBuffer[0], (double)colorBuffer[1], (double)colorBuffer[2]);

			// Blend Color if that bristle already has color
			/*if(bristle[y][x].ink != 0){
				printf("hh\n");
				double ratioPigment = .5;//(tmpInk * layerRatio.x + tmpInk * layerRatio.z) / (bristle[y][x].wetpigment + bristle[y][x].drypigment);
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

				/*bristle[y][x].color = vec4(
					bristle[y][x].color.r * ratioPigment + tmpcolor.x * (1 - ratioPigment),
					bristle[y][x].color.g * ratioPigment + tmpcolor.y * (1 - ratioPigment),
					bristle[y][x].color.b * ratioPigment + tmpcolor.z * (1 - ratioPigment),
					bristle[y][x].color.w
				);*/
				
				/*bristle[y][x].color = vec4(
					0.1* bristle[y][x].color.r + tmpcolor.x,
					0.1* bristle[y][x].color.g + tmpcolor.y,
					0.1* bristle[y][x].color.b + tmpcolor.z,
					bristle[y][x].color.w
				);

			}*/
			
			

			nowcount += count;
        }

        // Clear Buffer that Stored points in last drag
        nowDragged.clear();
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
		case 'a':
			printf("============================\nClear Canvas\n============================\n");
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

