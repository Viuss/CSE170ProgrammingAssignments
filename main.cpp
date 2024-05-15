#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <math.h>
#include <iostream>
#include "shader.h"
#include "shaderprogram.h"
#include <vector>

/*=================================================================================================
	DOMAIN
=================================================================================================*/

// Window dimensions
const int InitWindowWidth = 800;
const int InitWindowHeight = 800;
int WindowWidth = InitWindowWidth;
int WindowHeight = InitWindowHeight;

// Last mouse cursor position
int LastMousePosX = 0;
int LastMousePosY = 0;

// Arrays that track which keys are currently pressed
bool key_states[256];
bool key_special_states[256];
bool mouse_states[8];

// Other parameters
bool draw_wireframe = false;

/*=================================================================================================
	SHADERS & TRANSFORMATIONS
=================================================================================================*/

ShaderProgram PassthroughShader;
ShaderProgram PerspectiveShader;

glm::mat4 PerspProjectionMatrix(1.0f);
glm::mat4 PerspViewMatrix(1.0f);
glm::mat4 PerspModelMatrix(1.0f);

float perspZoom = 1.0f, perspSensitivity = 0.35f;
float perspRotationX = 0.0f, perspRotationY = 0.0f;

/*=================================================================================================
	OBJECTS
=================================================================================================*/

//VAO -> the object "as a whole", the collection of buffers that make up its data
//VBOs -> the individual buffers/arrays with data, for ex: one for coordinates, one for color, etc.

GLuint poly_VAO;
GLuint poly_VBO[2];
GLuint point_VAO;
GLuint point_VBO[2];
GLuint curve_VAO;
GLuint curve_VBO[2];

float lines_vertices[] = {
	-1.0f, 0.0f, 0.0f, 0.0f,
	-0.25f, 1.0f, 0.0f, 0.0f,
	0.25f, 1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f,
	0.25f, -1.0f, 0.0f, 0.0f,
	-0.25f, -1.0f, 0.0f, 0.0f,
};

float line_colors[] = {
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
};

int selectedPoint = 0;

float point_vertices[] = {
	-1.0f, 0.0f, 0.0f, 0.0f,
	-0.25f, 1.0f, 0.0f, 0.0f,
	0.25f, 1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f,
	0.25f, -1.0f, 0.0f, 0.0f,
	-0.25f, -1.0f, 0.0f, 0.0f,
};

float point_colors[] = {
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
};

std::vector<float> curve_vertices;
std::vector<float> curve_colors;
std::vector<float> point_parameters;

bool isBezier = true;
float numT = 6;

void createBezier(float);
void createBSpline(float);
float createBasis(int i, int k, float t);


/*=================================================================================================
	HELPER FUNCTIONS
=================================================================================================*/

void window_to_scene(int wx, int wy, float& sx, float& sy)
{
	sx = (2.0f * (float)wx / WindowWidth) - 1.0f;
	sy = 1.0f - (2.0f * (float)wy / WindowHeight);
}

/*=================================================================================================
	SHADERS
=================================================================================================*/

void CreateTransformationMatrices(void)
{
	// PROJECTION MATRIX
	PerspProjectionMatrix = glm::perspective<float>(glm::radians(60.0f), (float)WindowWidth / (float)WindowHeight, 0.01f, 1000.0f);

	// VIEW MATRIX
	glm::vec3 eye(0.0, 0.0, 2.0);
	glm::vec3 center(0.0, 0.0, 0.0);
	glm::vec3 up(0.0, 1.0, 0.0);

	PerspViewMatrix = glm::lookAt(eye, center, up);

	// MODEL MATRIX
	PerspModelMatrix = glm::mat4(1.0);
	PerspModelMatrix = glm::rotate(PerspModelMatrix, glm::radians(perspRotationX), glm::vec3(1.0, 0.0, 0.0));
	PerspModelMatrix = glm::rotate(PerspModelMatrix, glm::radians(perspRotationY), glm::vec3(0.0, 1.0, 0.0));
	PerspModelMatrix = glm::scale(PerspModelMatrix, glm::vec3(perspZoom));
}

void CreateShaders(void)
{
	// Renders without any transformations
	PassthroughShader.Create("./shaders/simple.vert", "./shaders/simple.frag");

	// Renders using perspective projection
	PerspectiveShader.Create("./shaders/persp.vert", "./shaders/persp.frag");

	//
	// Additional shaders would be defined here
	//
}

/*=================================================================================================
	BUFFERS
=================================================================================================*/

void CreatePolyBuffers(void)
{
	glGenVertexArrays(1, &poly_VAO); //generate 1 new VAO, its ID is returned in axis_VAO
	glBindVertexArray(poly_VAO); //bind the VAO so the subsequent commands modify it

	glGenBuffers(2, &poly_VBO[0]); //generate 2 buffers for data, their IDs are returned to the axis_VBO array

	// first buffer: vertex coordinates
	glBindBuffer(GL_ARRAY_BUFFER, poly_VBO[0]); //bind the first buffer using its ID
	glBufferData(GL_ARRAY_BUFFER, sizeof(lines_vertices), lines_vertices, GL_STATIC_DRAW); //send coordinate array to the GPU
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); //let GPU know this is attribute 0, made up of 4 floats
	glEnableVertexAttribArray(0);

	// second buffer: colors
	glBindBuffer(GL_ARRAY_BUFFER, poly_VBO[1]); //bind the second buffer using its ID
	glBufferData(GL_ARRAY_BUFFER, sizeof(line_colors), line_colors, GL_STATIC_DRAW); //send color array to the GPU
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); //let GPU know this is attribute 1, made up of 4 floats
	glEnableVertexAttribArray(1);

	glBindVertexArray(0); //unbind when done
}

void CreatePointBuffers(void)
{
	glGenVertexArrays(1, &point_VAO);
	glBindVertexArray(point_VAO);

	glGenBuffers(2, &point_VBO[0]);

	// First buffer: vertex coordinates
	glBindBuffer(GL_ARRAY_BUFFER, point_VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point_vertices), point_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Second buffer: colors
	glBindBuffer(GL_ARRAY_BUFFER, point_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point_colors), point_colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void CreateCurveBuffers(void)
{
	glGenVertexArrays(1, &curve_VAO);
	glBindVertexArray(curve_VAO);

	glGenBuffers(2, &curve_VBO[0]);

	// First buffer: vertex coordinates
	glBindBuffer(GL_ARRAY_BUFFER, curve_VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, curve_vertices.size() * sizeof(float), curve_vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Second buffer: colors
	glBindBuffer(GL_ARRAY_BUFFER, curve_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, curve_colors.size() * sizeof(float), curve_colors.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

/*=================================================================================================
	CALLBACKS
=================================================================================================*/

//-----------------------------------------------------------------------------
// CALLBACK DOCUMENTATION
// https://www.opengl.org/resources/libraries/glut/spec3/node45.html
// http://freeglut.sourceforge.net/docs/api.php#WindowCallback
//-----------------------------------------------------------------------------

void idle_func()
{
	//uncomment below to repeatedly draw new frames
	glutPostRedisplay();
}

void reshape_func(int width, int height)
{
	WindowWidth = width;
	WindowHeight = height;

	glViewport(0, 0, width, height);
	glutPostRedisplay();
}

void keyboard_func(unsigned char key, int x, int y)
{
	key_states[key] = true;

	switch (key)
	{
		// Control point up
	case 'w':
	{
		// Increment y-coordinate of the selected control point
		point_vertices[selectedPoint * 4 + 1] += 0.05f;

		// Also the corresponding line vertex
		lines_vertices[selectedPoint * 4 + 1] += 0.05f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		CreatePolyBuffers();
		break;
	}

	//  Control point down
	case 's':
	{
		// Decrement y-coordinate of the selected control point
		point_vertices[selectedPoint * 4 + 1] -= 0.05f;

		// Also move the corresponding line vertex
		lines_vertices[selectedPoint * 4 + 1] -= 0.05f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		CreatePolyBuffers();
		break;
	}

	//  Control point left
	case 'a':
	{
		// Decrement x-coordinate of the selected control point
		point_vertices[selectedPoint * 4] -= 0.05f;

		// Also move the corresponding line vertex
		lines_vertices[selectedPoint * 4] -= 0.05f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		CreatePolyBuffers();
		break;
	}

	//  Control point right
	case 'd':
	{
		// Increment x-coordinate of the selected control point
		point_vertices[selectedPoint * 4] += 0.05f;

		// Also move the corresponding line vertex
		lines_vertices[selectedPoint * 4] += 0.05f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		CreatePolyBuffers();
		break;
	}

	// Next control point
	case 'e':
	{
		// Chaning all colors back
		for (int i = 0; i < 6; i++)
		{
			point_colors[i * 4] = 0.0f;
			point_colors[i * 4 + 1] = 1.0f;
			point_colors[i * 4 + 2] = 0.0f;
			point_colors[i * 4 + 3] = 1.0f;
		}

		// Increment selected control point, or loop back
		selectedPoint++;
		if (selectedPoint > 5)
			selectedPoint = 0;

		// Set current point to blue to distinguish
		point_colors[selectedPoint * 4 + 2] = 1.0f;
		point_colors[selectedPoint * 4 + 1] = 0.0f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		break;
	}

	// Previous control point
	case 'q':
	{
		// Changing all colors back
		for (int i = 0; i < 6; i++)
		{
			point_colors[i * 4] = 0.0f;
			point_colors[i * 4 + 1] = 1.0f;
			point_colors[i * 4 + 2] = 0.0f;
			point_colors[i * 4 + 3] = 1.0f;
		}

		// Decrement selected control point, or loop back
		selectedPoint--;
		if (selectedPoint < 0)
			selectedPoint = 5;

		// Set current point to blue to distinguish
		point_colors[selectedPoint * 4 + 2] = 1.0f;
		point_colors[selectedPoint * 4 + 1] = 0.0f;

		// Redraw selected curve
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);

		// Recreate buffers
		CreatePointBuffers();
		break;
	}

	// Increase resolution
	case 'z':
	{
		// Increment res by 4
		numT += 4.0f;

		// Recreate buffers
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);
		break;
	}

	// Decrease resolution
	case 'x':
	{
		// Decrement res by 4 but not to below 4
		if (numT > 4)
			numT -= 4.0f;

		// Recreate buffers
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);
		break;
	}

	// Reset resolution
	case 'c':
	{
		// Reset granularity to 4
		numT = 4.0f;

		// Recreate buffers
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);
		break;
	}

	// Toggle between Bezier and B-spline
	case 'f':
	{
		// Toggle bool
		isBezier = !isBezier;

		// Recreate buffers
		if (isBezier)
			createBezier(numT);
		else
			createBSpline(numT);
		break;
	}

	// Exit the application on Escape key press
	case '\x1B':
	{
		exit(EXIT_SUCCESS);
		break;
	}
	}
}


void key_released(unsigned char key, int x, int y)
{
	key_states[key] = false;
}

void key_special_pressed(int key, int x, int y)
{
	key_special_states[key] = true;
}

void key_special_released(int key, int x, int y)
{
	key_special_states[key] = false;
}

void mouse_func(int button, int state, int x, int y)
{
	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if (x < 0 || x > WindowWidth || y < 0 || y > WindowHeight)
		return;

	float px, py;
	window_to_scene(x, y, px, py);

	if (button == 3)
	{
		perspZoom += 0.03f;
	}
	else if (button == 4)
	{
		if (perspZoom - 0.03f > 0.0f)
			perspZoom -= 0.03f;
	}

	mouse_states[button] = (state == GLUT_DOWN);

	LastMousePosX = x;
	LastMousePosY = y;
}

void passive_motion_func(int x, int y)
{
	if (x < 0 || x > WindowWidth || y < 0 || y > WindowHeight)
		return;

	float px, py;
	window_to_scene(x, y, px, py);

	LastMousePosX = x;
	LastMousePosY = y;
}

void active_motion_func(int x, int y)
{
	if (x < 0 || x > WindowWidth || y < 0 || y > WindowHeight)
		return;

	float px, py;
	window_to_scene(x, y, px, py);
	/*
	if( mouse_states[0] == true )
	{
		perspRotationY += ( x - LastMousePosX ) * perspSensitivity;
		perspRotationX += ( y - LastMousePosY ) * perspSensitivity;
	}
	*/
	LastMousePosX = x;
	LastMousePosY = y;
}


void createBezier(float tSize) {
	// Clear vecs
	curve_vertices.clear();
	curve_colors.clear();

	// Calculate step size for t based on tSize
	float t = 1.0f / tSize;

	// Iterate through values 0 to 1 in steps of t
	for (float i = 0; i <= 1; i += t) {
		// Calculate x and y coordinates for Bezier curve using de Casteljau's

		// The de Casteljau algorithm calculates points on the Bezier
		// Recursively interpolating points between control points.

		// For each i, calculate x-coordinate of the point on the curve.

		float x = 
			// Calculate the x-coordinate using the formula for Bezier interpolation.
			// Each term in the sum represents the contribution of a control point to the curve at parameter value i (6 control points)
			powf(1 - i, 5.0) * point_vertices[0] +
			5 * i * powf(1.0f - i, 4) * point_vertices[4] +
			10 * powf(i, 2) * powf(1 - i, 3) * point_vertices[8] +
			10 * powf(i, 3) * powf(1 - i, 2) * point_vertices[12] +
			5 * powf(i, 4) * (1 - i) * point_vertices[16] +
			powf(i, 5) * point_vertices[20];

		float y = 
			// Calculate the y-coordinate using the formula for Bezier interpolation.
			// Similar to the x-coordinate calculation, each term represents the contribution of a control point.
			powf(1 - i, 5.0) * point_vertices[1] +
			5 * i * powf(1.0f - i, 4) * point_vertices[5] +
			10 * powf(i, 2) * powf(1 - i, 3) * point_vertices[9] +
			10 * powf(i, 3) * powf(1 - i, 2) * point_vertices[13] +
			5 * powf(i, 4) * (1 - i) * point_vertices[17] +
			powf(i, 5) * point_vertices[21];

		// Add vertex to vertex vector
		curve_vertices.push_back(x);
		curve_vertices.push_back(y);
		curve_vertices.push_back(0.0f); 
		curve_vertices.push_back(0.0f); // Padding (not used)

		// Color (red)
		curve_colors.push_back(1.0);
		curve_colors.push_back(0.0);
		curve_colors.push_back(0.0);
		curve_colors.push_back(1.0);
	}

	// Add the last control point to ensure the curve passes through it
	curve_vertices.push_back(point_vertices[20]);
	curve_vertices.push_back(point_vertices[21]);
	curve_vertices.push_back(0.0f);
	curve_vertices.push_back(0.0f);
	curve_colors.push_back(1.0);
	curve_colors.push_back(0.0);
	curve_colors.push_back(0.0);
	curve_colors.push_back(1.0);

	// Update buffers
	CreateCurveBuffers();
}

void createBSpline(float tSize)
{
	int n = 6; // Control points

	// Clear vectors
	curve_vertices.clear();
	curve_colors.clear();

	// Calculate step size for t based on tSize
	float tStep = 2.0f / tSize; // Quadratic B-spline has domain [2, 4]

	// Iterate from 2 to 4 in steps of tStep
	for (float t = 2; t <= 4; t += tStep)
	{
		float x = 0;
		float y = 0;

		// Compute x and y coordinates of B-spline curve at parameter t
		for (int i = 0; i < n; i++)
		{
			// Calculate B-spline basis function value for i-th control point
			float basis = createBasis(i, 2, t); // Using quadratic basis function
			x += basis * point_vertices[i * 4];     // Multiply basis by x-coordinate of control point
			y += basis * point_vertices[i * 4 + 1]; // Multiply basis by y-coordinate of control point
		}

		// Computed vertex to vertices vector
		curve_vertices.push_back(x);
		curve_vertices.push_back(y);
		curve_vertices.push_back(0.0f);
		curve_vertices.push_back(0.0f); // Padding (not used)

		// Color (red)
		curve_colors.push_back(1.0);
		curve_colors.push_back(0.0);
		curve_colors.push_back(0.0);
		curve_colors.push_back(1.0);
	}

	// Update buffers 
	CreateCurveBuffers(); 
}


// Compute the basis function for a given index i, degree k, and parameter t
float createBasis(int i, int k, float t)
{
	// Check for out-of-bounds
	if (i < 0 || i + k - 1 >= point_parameters.size() || i >= point_parameters.size()) {
		// Handle out-of-bounds indices by returning 0.0f
		return 0.0f;
	}
	else
	{
		// Recursive - Cox-de Boor recursion

		// Initial left and right basis values
		float basis_left = 0.0f;
		float basis_right = 0.0f;

		// Compute left basis value if denominator non-zero
		if (point_parameters[i + k - 1] != point_parameters[i]) {
			// Compute the left basis recursively
			basis_left = (t - point_parameters[i]) / (point_parameters[i + k - 1] - point_parameters[i]) * createBasis(i, k - 1, t);
		}

		// Compute right basis value if denominator is non-zero
		if (point_parameters[i + k] != point_parameters[i + 1]) {
			// Compute the right basis recursively
			basis_right = (point_parameters[i + k] - t) / (point_parameters[i + k] - point_parameters[i + 1]) * createBasis(i + 1, k - 1, t);
		}

		// Sum of left and right basis values
		return basis_left + basis_right;
	}
}



/*=================================================================================================
	RENDERING
=================================================================================================*/

void display_func(void)
{
	// Clear the contents of the back buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Update transformation matrices
	CreateTransformationMatrices();

	// Choose which shader to use, and send the transformation matrix information to it
	PerspectiveShader.Use();
	PerspectiveShader.SetUniform("projectionMatrix", glm::value_ptr(PerspProjectionMatrix), 4, GL_FALSE, 1);
	PerspectiveShader.SetUniform("viewMatrix", glm::value_ptr(PerspViewMatrix), 4, GL_FALSE, 1);
	PerspectiveShader.SetUniform("modelMatrix", glm::value_ptr(PerspModelMatrix), 4, GL_FALSE, 1);

	// Drawing in wireframe?
	if (draw_wireframe == true)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Bind the point VAO and draw control points
	glBindVertexArray(point_VAO);
	glDrawArrays(GL_POINTS, 0, 6);

	// Bind the curve VAO and draw the B-spline curve
	glBindVertexArray(curve_VAO);
	glDrawArrays(GL_LINE_STRIP, 0, curve_vertices.size() / 4);

	// Bind the polygon VAO and draw the polygon
	glBindVertexArray(poly_VAO);
	glDrawArrays(GL_LINE_STRIP, 0, 6);

	// Unbind VAO when done
	glBindVertexArray(0);

	// Swap the front and back buffers
	glutSwapBuffers();
}


/*=================================================================================================
	INIT
=================================================================================================*/

void init(void)
{
	// Print some info
	std::cout << "Vendor:         " << glGetString(GL_VENDOR) << "\n";
	std::cout << "Renderer:       " << glGetString(GL_RENDERER) << "\n";
	std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
	std::cout << "GLSL Version:   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n\n";

	// Set OpenGL settings
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // background color
	glEnable(GL_DEPTH_TEST); // enable depth test
	glEnable(GL_CULL_FACE); // enable back-face culling
	glPointSize(10.0);

	// Create shaders
	CreateShaders();

	// Create axis buffers
	CreatePolyBuffers();
	CreatePointBuffers();	

	// Initialize the curve VAO
	glGenVertexArrays(1, &curve_VAO);
	glBindVertexArray(curve_VAO);

	// Bind the curve VBO
	glBindBuffer(GL_ARRAY_BUFFER, curve_VBO[0]);

	// Specify the layout of the vertex data
	// Assuming each vertex consists of 4 floats (x, y, z, padding)
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the curve VAO
	glBindVertexArray(0);

	std::cout << "Finished initializing...\n\n";

}

/*=================================================================================================
	MAIN
=================================================================================================*/

int main(int argc, char** argv)
{
	// Create and initialize the OpenGL context
	glutInit(&argc, argv);

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(InitWindowWidth, InitWindowHeight);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	glutCreateWindow("CSE-170 Computer Graphics");

	// Initialize GLEW
	GLenum ret = glewInit();
	if (ret != GLEW_OK) {
		std::cerr << "GLEW initialization error." << std::endl;
		glewGetErrorString(ret);
		return -1;
	}

	// Register callback functions
	glutDisplayFunc(display_func);
	glutIdleFunc(idle_func);
	glutReshapeFunc(reshape_func);
	glutKeyboardFunc(keyboard_func);
	glutKeyboardUpFunc(key_released);
	glutSpecialFunc(key_special_pressed);
	glutSpecialUpFunc(key_special_released);
	glutMouseFunc(mouse_func);
	glutMotionFunc(active_motion_func);
	glutPassiveMotionFunc(passive_motion_func);

	// Do program initialization
	init();

	// Enter the main loop
	glutMainLoop();

	return EXIT_SUCCESS;
}



