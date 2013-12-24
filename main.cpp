// LightEngine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "le/LightSystem.h"
#include "le/leOpenGLRender.h"
#include <gl/glut.h>

LightSystem light_system;
int mainWindow;

float n = 0;
float x , y ;
float MouseX,MouseY;
float Num;


float an = 0;


//===============World loading=======
void ReadLine(FILE * file,char *string)
{
	
	do
	{
		fgets(string,255,file);
	}while ((string[0] == '/') || (string[0] == '\n'));
	return;
}
void LoadWorld()
{
	FILE *file;
	char oneline[255];
	int Polygon_count = 0;
	float X = 0.0f,Y = 0.0f;

	file = fopen("Data/map.txt","rt");

	ReadLine(file,oneline);
	sscanf(oneline,"Polygons %d",&Polygon_count);
	printf("Polygons: %d \n",Polygon_count);
	for (int p_num = 0; p_num < Polygon_count;p_num++)
	{
		light_system.AddObject(lePoly2D());
		int Vertex_count;
		ReadLine(file,oneline);
		sscanf(oneline,"Polygon %d",&Vertex_count);
		printf("Polygon: %d \n",p_num);
		for (int v_num = 0; v_num < Vertex_count; v_num++)
		{
			ReadLine(file,oneline);
			sscanf(oneline,"%f %f",&X,&Y);
			light_system.Polygons[p_num].AddVertex(leVec2f(X,Y));
			printf("Polygon: %d Vertex: %d X: %f Y: %f \n",p_num,v_num,X,Y);
		}
	}
	Polygon_count = 0;
	X = 0.0f;Y = 0.0f;
	float Radius = 0.0f;
	
	printf("========CIRCLE=========== \n");

	ReadLine(file,oneline);
	sscanf(oneline,"Circle %d",&Polygon_count);
	printf("Circles: %d \n",Polygon_count);
	for (int c_num = 0 ; c_num < Polygon_count; c_num++)
	{
		ReadLine(file,oneline);
		sscanf(oneline,"%f %f %f",&X,&Y,&Radius);
		light_system.AddObject(leEllipse2D(leVec2f(X,Y),Radius));
		printf("Circle: %d X: %f Y: %f Radius: %f \n",c_num,X,Y,Radius);
	}


	Polygon_count = 0;
	X = 0.0f;Y = 0.0f;
	Radius = 0.0f;
	float Inten = 0.0f;
	printf("========LIGHT=========== \n");

	ReadLine(file,oneline);
	sscanf(oneline,"Lights %d",&Polygon_count);
	printf("Lights: %d \n",Polygon_count);
	for (int l_num = 0; l_num < Polygon_count; l_num++)
	{
		ReadLine(file,oneline);
		sscanf(oneline,"Light %f %f %f %f",&X,&Y,&Radius,&Inten);
		light_system.AddLight(leLight(leVec2f(X,Y),Radius,Inten));
		printf("Light: %d X %f Y %f Radius %f Inten %f \n",Polygon_count,X,Y,Radius,Inten);
	}
	fclose(file);

	light_system.AddLight(leLight(leVec2f(15,15),15,45,15));
	light_system.AddLight(leLight(leVec2f(32,15),10,110,45));
}



//===================================
void DebugDraw()
{
	for (int p_num = 0; p_num <(int) light_system.Polygons.size();p_num++)
	{
		glColor3f(0.0f,1.0,0.0f);
		DrawGLLinePolygon(light_system.Polygons[p_num]);
	}
	for ( int c_num = 0 ; c_num <(int) light_system.Ellipses.size();c_num++ )
	{
		DragGLLineCircle(light_system.Ellipses[c_num].Position.x,light_system.Ellipses[c_num].Position.y,
			light_system.Ellipses[c_num].Radius);
	}
	for (int l_num = 0; l_num < (int) light_system.Lights.size();l_num++)
	{
		if (light_system.Lights[l_num].type == le_llight)
		{
			glColor3f(1.0f,0.0f,0.0f);
			DrawGLPoint(light_system.Lights[l_num].Position,4);
			DragGLLineCircle(light_system.Lights[l_num].Position.x,
			light_system.Lights[l_num].Position.y,light_system.Lights[l_num].Radius);
		}
		if (light_system.Lights[l_num].type == le_flight)
		{
			glColor3f(1.0,0.0,0.0);
			DrawGLLine(light_system.Lights[l_num].Position,light_system.Lights[l_num].v[1]);
			DrawGLLine(light_system.Lights[l_num].Position,light_system.Lights[l_num].v[2]);
			DrawGLPoint(light_system.Lights[l_num].Position,4);

		}


		for (int i = 0; i < (int)light_system.Edges.size(); i++)
		{
			glColor3f(0.0f,0.0f,0.0f);
			DrawGLPoint(light_system.Edges[i].v1,4);
			DrawGLPoint(light_system.Edges[i].v2,4);
			DrawGLLine(light_system.Edges[i].v1,light_system.Edges[i].LightVec1);
			DrawGLLine(light_system.Edges[i].v2,light_system.Edges[i].LightVec2);
		}
	}
}
void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |GL_STENCIL_BUFFER_BIT);



	
	DebugDraw();

	

	glutSwapBuffers();
}

void Resize(int W,int H)
{
	glViewport(0,0,W,H);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 50, 0, 50, -10.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}
float n1;


void Timer(int)
{
	glutSetWindow(mainWindow);
	glutPostRedisplay();
	glutTimerFunc(15, Timer, 0);


	n += 0.01;
	n1 += 0.01;

	x = 26 + sin(n) * 15;
	y = 16 + cos(n) * 15;

	light_system.Lights[0].Move(MouseX,MouseY);

	an += 0.1f;
	light_system.Lights[1].Move(15,15,an,25);





	light_system.CheckLightPosition();
	light_system.FindEdges();
}

void MMF(int x,int y)
{
	float n1 = 50.0f / 700.0f;

	MouseX = (float) x * n1;
	MouseY = 50.0f - (float) y * n1;


}

void InitGL()
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	LoadWorld();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);



	
}

int _tmain(int argc, char* argv[])
{
	glutInit(&argc,argv);	
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(700,700); 
    glutInitWindowPosition(100, 100);
	mainWindow = glutCreateWindow("LightEngine");
	glutDisplayFunc(Display);                 
    glutReshapeFunc(Resize);
	InitGL();
	glutMotionFunc(MMF);
	glutTimerFunc(100, Timer, 0);
	

	glutMainLoop();
	return 0;
}

