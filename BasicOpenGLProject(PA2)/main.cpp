#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <vector>
#include "shader.h"
#include "shaderprogram.h"

/*=================================================================================================
	DOMAIN
=================================================================================================*/

// Window dimensions
const int InitWindowWidth  = 800;
const int InitWindowHeight = 800;
int WindowWidth  = InitWindowWidth;
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

//More parameters for Torus
float outerRad = 0.4;
float innerRad = 0.2;
int Slices = 20;
int Loops = 20;
int centerX = 0;
int centerY = 0;
int centerZ = 0;


//Initializing torus items
GLuint torus_VAO;
GLuint torus_VBO[2];

int Size = 0;
std::vector<float> torusColors;
std::vector<float> torusVertices;


/*=================================================================================================
	SHADERS & TRANSFORMATIONS
=================================================================================================*/

ShaderProgram PassthroughShader;
ShaderProgram PerspectiveShader;

glm::mat4 PerspProjectionMatrix( 1.0f );
glm::mat4 PerspViewMatrix( 1.0f );
glm::mat4 PerspModelMatrix( 1.0f );

float perspZoom = 1.0f, perspSensitivity = 0.35f;
float perspRotationX = 0.0f, perspRotationY = 0.0f;

/*=================================================================================================
	OBJECTS
=================================================================================================*/

//VAO -> the object "as a whole", the collection of buffers that make up its data
//VBOs -> the individual buffers/arrays with data, for ex: one for coordinates, one for color, etc.

GLuint axis_VAO;
GLuint axis_VBO[2];

float axis_vertices[] = {
	//x axis
	-1.0f,  0.0f,  0.0f, 1.0f,
	1.0f,  0.0f,  0.0f, 1.0f,
	//y axis
	0.0f, -1.0f,  0.0f, 1.0f,
	0.0f,  1.0f,  0.0f, 1.0f,
	//z axis
	0.0f,  0.0f, -1.0f, 1.0f,
	0.0f,  0.0f,  1.0f, 1.0f
};

float axis_colors[] = {
	//x axis
	1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
	//y axis
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	//z axis
	0.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f
};



/*=================================================================================================
	HELPER FUNCTIONS
=================================================================================================*/

void window_to_scene( int wx, int wy, float& sx, float& sy )
{
	sx = ( 2.0f * (float)wx / WindowWidth ) - 1.0f;
	sy = 1.0f - ( 2.0f * (float)wy / WindowHeight );
}

/*=================================================================================================
	SHADERS
=================================================================================================*/

void CreateTransformationMatrices( void )
{
	// PROJECTION MATRIX
	PerspProjectionMatrix = glm::perspective<float>( glm::radians( 60.0f ), (float)WindowWidth / (float)WindowHeight, 0.01f, 1000.0f );

	// VIEW MATRIX
	glm::vec3 eye   ( 0.0, 0.0, 2.0 );
	glm::vec3 center( 0.0, 0.0, 0.0 );
	glm::vec3 up    ( 0.0, 1.0, 0.0 );

	PerspViewMatrix = glm::lookAt( eye, center, up );

	// MODEL MATRIX
	PerspModelMatrix = glm::mat4( 1.0 );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationX ), glm::vec3( 1.0, 0.0, 0.0 ) );
	PerspModelMatrix = glm::rotate( PerspModelMatrix, glm::radians( perspRotationY ), glm::vec3( 0.0, 1.0, 0.0 ) );
	PerspModelMatrix = glm::scale( PerspModelMatrix, glm::vec3( perspZoom ) );
}

void CreateShaders( void )
{
	// Renders without any transformations
	PassthroughShader.Create( "./shaders/simple.vert", "./shaders/simple.frag" );

	// Renders using perspective projection
	PerspectiveShader.Create( "./shaders/persp.vert", "./shaders/persp.frag" );

	//
	// Additional shaders would be defined here
	//
}

/*=================================================================================================
	BUFFERS
=================================================================================================*/

void CreateAxisBuffers( void )
{
	glGenVertexArrays( 1, &axis_VAO ); //generate 1 new VAO, its ID is returned in axis_VAO
	glBindVertexArray( axis_VAO ); //bind the VAO so the subsequent commands modify it

	glGenBuffers( 2, &axis_VBO[0] ); //generate 2 buffers for data, their IDs are returned to the axis_VBO array

	// first buffer: vertex coordinates
	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[0] ); //bind the first buffer using its ID
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_vertices ), axis_vertices, GL_STATIC_DRAW ); //send coordinate array to the GPU
	glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 ); //let GPU know this is attribute 0, made up of 4 floats
	glEnableVertexAttribArray( 0 );

	// second buffer: colors
	glBindBuffer( GL_ARRAY_BUFFER, axis_VBO[1] ); //bind the second buffer using its ID
	glBufferData( GL_ARRAY_BUFFER, sizeof( axis_colors ), axis_colors, GL_STATIC_DRAW ); //send color array to the GPU
	glVertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 ); //let GPU know this is attribute 1, made up of 4 floats
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( 0 ); //unbind when done

	//NOTE: You will probably not use an array for your own objects, as you will need to be
	//      able to dynamically resize the number of vertices. Remember that the sizeof()
	//      operator will not give an accurate answer on an entire vector. Instead, you will
	//      have to do a calculation such as sizeof(v[0]) * v.size().
}


void CreateTorusBuffers(void)
{
	// generate a vertex array object(VAO)
	glGenVertexArrays(1, &torus_VAO);

	// bind the VAO
	glBindVertexArray(torus_VAO);

	glGenBuffers(2, &torus_VBO[0]); //generate 2 buffers for data, their IDs are returned to the axis_VBO array

	// first buffer: vertex coordinates
	glBindBuffer(GL_ARRAY_BUFFER, torus_VBO[0]);
	// allocate storage and copy vertex coordinates into the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(torusVertices[0]) * torusVertices.size(), &torusVertices[0], GL_STATIC_DRAW);
	// specify the layout of the vertex data for the vertex shader
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	// enable the vertex attribute at index 0
	glEnableVertexAttribArray(0);

	// second buffer: colors
	glBindBuffer(GL_ARRAY_BUFFER, torus_VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(torusColors[0]) * torusColors.size(), &torusColors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);


	glBindVertexArray(0); //unbind when done
}


// generate the vertex and color data for a torus
void makeTorus(float rad1, float rad2, float slices, float loops) {
	// set size counter and clear existing data
	Size = 0;
	torusVertices.clear();
	torusColors.clear();

	// outer loop for vertical sections of the torus
	for (int j = 0; j <= loops; ++j) {
		float u1 = j / loops; //u1 representing the position along the loop section (from 0 to 1).
		float loopAngle1 = u1 * 2.0f * 3.14f;  // converts u1 to an angle in radians, representing the rotation around the torus.
		float cosLoops1 = cos(loopAngle1);  // computes the cosine of the rotation angle.	
		float sinLoops1 = sin(loopAngle1); // computes the sine of the rotation angle.

		float u2 = (j + 1) / loops; //u2 representing the position along the loop section (from 0 to 1) for the next vertex.
		float loopAngle2 = u2 * 2.0f * 3.14f; 
		float cosLoops2 = cos(loopAngle2);
		float sinLoops2 = sin(loopAngle2);

		// loop for slices in each vertical section
		for (int i = 0; i <= slices; ++i) {
			float v1 = i / slices; // v1 representing the position along the slice (from 0 to 1).
			float sliceAngle1 = v1 * 2.0f * 3.14f; // converts v1 to an angle in radians
			float cosSlices1 = cos(sliceAngle1); // computes the cosine of the rotation angle within the slice.
			float sinSlices1 = sin(sliceAngle1); // computes the sine of the rotation angle within the slice.
			float sliceRadius1 = rad1 + rad2 * cosSlices1; // calculates the radius of the slice based on the major and minor radii.

			float v2 = (i + 1) / slices; // v2 representing the position along the slice (from 0 to 1) for the next vertex.
			float sliceAngle2 = v2 * 2.0f * 3.14f; // converts v2 to an angle in radians, representing the rotation around the slice for the next vertex.
			float cosSlices2 = cos(sliceAngle2); // computes the cosine of the rotation angle within the slice for the next vertex.
			float sinSlices2 = sin(sliceAngle2); // computes the sine of the rotation angle within the slice for the next vertex.
			float sliceRadius2 = rad1 + rad2 * cosSlices2; // calculates the radius of the slice for the next vertex based on the major and minor radii.

			float ax = sliceRadius1 * cosLoops1;
			float ay = sliceRadius1 * sinLoops1;
			float az = sinSlices1 * rad2;

			float ax1 = sliceRadius1 * cosLoops2;
			float ay1 = sliceRadius1 * sinLoops2;
			float az1 = sinSlices1 * rad2;

			float bx = sliceRadius2 * cosLoops1;
			float by = sliceRadius2 * sinLoops1;
			float bz = sinSlices2 * rad2;

			float bx1 = sliceRadius2 * cosLoops2;
			float by1 = sliceRadius2 * sinLoops2;
			float bz1 = sinSlices2 * rad2;

			//order matters because it needs to be built counterclockwise

			glm::vec3 vec_flat_1 = { bx - ax1, by - ay1, bz - az1 }; // calculates a vector in the plane of the torus slice from a1 to b.
			glm::vec3 vec_flat_2 = { ax1 - ax, ay1 - ay, az1 - az }; // calculates a vector in the plane of the torus slice from a to a1.


			// add the coordinates for vertex a
			torusVertices.push_back(ax);
			torusVertices.push_back(ay);
			torusVertices.push_back(az);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex a.
			Size++;

			// add the coordinates for vertex a1.
			torusVertices.push_back(ax1);
			torusVertices.push_back(ay1);
			torusVertices.push_back(az1);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex a1.
			Size++;

			// add the coordinates for vertex b.
			torusVertices.push_back(bx);
			torusVertices.push_back(by);
			torusVertices.push_back(bz);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex b.
			Size++;

			// repeat coordinates to complete first triangle.
			torusVertices.push_back(bx);
			torusVertices.push_back(by);
			torusVertices.push_back(bz);
			torusVertices.push_back(1.0f);
			Size++;

			// repeat a1 to complete 2nd triangle.
			torusVertices.push_back(ax1);
			torusVertices.push_back(ay1);
			torusVertices.push_back(az1);
			torusVertices.push_back(1.0f);
			Size++;

			// add the coordinates for vertex b1.
			torusVertices.push_back(bx1);
			torusVertices.push_back(by1);
			torusVertices.push_back(bz1);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex b1.
			Size++;



			// run a color for each of our 6 points.
			for (int k = 0; k < 6; ++k) {
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
			}
		}
	}
}

// generate at specific coordinates
void generateTorusAt(float centerX, float centerY, float centerZ, float rad1, float rad2, float slices, float loops) {
	// set size counter and clear existing data
	Size = 0;
	torusVertices.clear();
	torusColors.clear();

	// outer loop for vertical sections of the torus
	for (int j = 0; j <= loops; ++j) {
		float u1 = j / loops; //u1 representing the position along the loop section (from 0 to 1).
		float loopAngle1 = u1 * 2.0f * 3.14f;  // converts u1 to an angle in radians, representing the rotation around the torus.
		float cosLoops1 = cos(loopAngle1);  // computes the cosine of the rotation angle.	
		float sinLoops1 = sin(loopAngle1); // computes the sine of the rotation angle.

		float u2 = (j + 1) / loops; //u2 representing the position along the loop section (from 0 to 1) for the next vertex.
		float loopAngle2 = u2 * 2.0f * 3.14f;
		float cosLoops2 = cos(loopAngle2);
		float sinLoops2 = sin(loopAngle2);

		// loop for slices in each vertical section
		for (int i = 0; i <= slices; ++i) {
			float v1 = i / slices; // v1 representing the position along the slice (from 0 to 1).
			float sliceAngle1 = v1 * 2.0f * 3.14f; // converts v1 to an angle in radians
			float cosSlices1 = cos(sliceAngle1); // computes the cosine of the rotation angle within the slice.
			float sinSlices1 = sin(sliceAngle1); // computes the sine of the rotation angle within the slice.
			float sliceRadius1 = rad1 + rad2 * cosSlices1; // calculates the radius of the slice based on the major and minor radii.

			float v2 = (i + 1) / slices; // v2 representing the position along the slice (from 0 to 1) for the next vertex.
			float sliceAngle2 = v2 * 2.0f * 3.14f; // converts v2 to an angle in radians, representing the rotation around the slice for the next vertex.
			float cosSlices2 = cos(sliceAngle2); // computes the cosine of the rotation angle within the slice for the next vertex.
			float sinSlices2 = sin(sliceAngle2); // computes the sine of the rotation angle within the slice for the next vertex.
			float sliceRadius2 = rad1 + rad2 * cosSlices2; // calculates the radius of the slice for the next vertex based on the major and minor radii.

			float ax = sliceRadius1 * cosLoops1 + centerX;
			float ay = sliceRadius1 * sinLoops1 + centerY;
			float az = sinSlices1 * rad2 + centerZ;

			float ax1 = sliceRadius1 * cosLoops2 + centerX;
			float ay1 = sliceRadius1 * sinLoops2 + centerY;
			float az1 = sinSlices1 * rad2 + centerZ;

			float bx = sliceRadius2 * cosLoops1 + centerX;
			float by = sliceRadius2 * sinLoops1 + centerY;
			float bz = sinSlices2 * rad2 + centerZ;

			float bx1 = sliceRadius2 * cosLoops2 + centerX;
			float by1 = sliceRadius2 * sinLoops2 + centerY;
			float bz1 = sinSlices2 * rad2 + centerZ;


			glm::vec3 vec_flat_1 = { bx - ax1, by - ay1, bz - az1 }; // calculates a vector in the plane of the torus slice from a1 to b.
			glm::vec3 vec_flat_2 = { ax1 - ax, ay1 - ay, az1 - az }; // calculates a vector in the plane of the torus slice from a to a1.


			// add the coordinates for vertex a
			torusVertices.push_back(ax);
			torusVertices.push_back(ay);
			torusVertices.push_back(az);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex a.
			Size++;

			// add the coordinates for vertex a1.
			torusVertices.push_back(ax1);
			torusVertices.push_back(ay1);
			torusVertices.push_back(az1);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex a1.
			Size++;

			// add the coordinates for vertex b.
			torusVertices.push_back(bx);
			torusVertices.push_back(by);
			torusVertices.push_back(bz);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex b.
			Size++;

			// repeat coordinates to complete first triangle.
			torusVertices.push_back(bx);
			torusVertices.push_back(by);
			torusVertices.push_back(bz);
			torusVertices.push_back(1.0f);
			Size++;

			// repeat a1 to complete 2nd triangle.
			torusVertices.push_back(ax1);
			torusVertices.push_back(ay1);
			torusVertices.push_back(az1);
			torusVertices.push_back(1.0f);
			Size++;

			// add the coordinates for vertex b1.
			torusVertices.push_back(bx1);
			torusVertices.push_back(by1);
			torusVertices.push_back(bz1);
			torusVertices.push_back(1.0f); // homogeneous coordinate w for vertex b1.
			Size++;



			// run a color for each of our 6 points.
			for (int k = 0; k < 6; ++k) {
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
				torusColors.push_back(0.5);
			}
		
		}
	}
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

void reshape_func( int width, int height )
{
	WindowWidth  = width;
	WindowHeight = height;

	glViewport( 0, 0, width, height );
	glutPostRedisplay();
}

void keyboard_func( unsigned char key, int x, int y )
{
	key_states[ key ] = true;

	switch( key )
	{
		case 'g':
		{
			draw_wireframe = !draw_wireframe;
			if( draw_wireframe == true )
				std::cout << "Wireframes on.\n";
			else
				std::cout << "Wireframes off.\n";
			break;
		}

		case 'q':
		{
			Slices++;
			Loops++;
			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case 'a':
		{
			Slices--;
			Loops--;

			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case 'w':
		{
			innerRad += 0.1;
			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case 's':
		{
			innerRad -= 0.1;
			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case 'e':
		{
			outerRad += 0.1;
			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case 'd':
		{
			outerRad -= 0.1;
			makeTorus(outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '1':
		{
			centerX += 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '2':
		{
			centerX -= 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '3':
		{
			centerY += 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '4':
		{
			centerY -= 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '5':
		{
			centerZ += 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		case '6':
		{
			centerZ -= 1;
			generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
			CreateTorusBuffers();

			break;
		}

		// Exit on escape key press
		case '\x1B':
		{
			exit( EXIT_SUCCESS );
			break;
		}
	}
}

void key_released( unsigned char key, int x, int y )
{
	key_states[ key ] = false;
}

void key_special_pressed( int key, int x, int y )
{
	key_special_states[ key ] = true;
}

void key_special_released( int key, int x, int y )
{
	key_special_states[ key ] = false;
}

void mouse_func( int button, int state, int x, int y )
{
	// Key 0: left button
	// Key 1: middle button
	// Key 2: right button
	// Key 3: scroll up
	// Key 4: scroll down

	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( button == 3 )
	{
		perspZoom += 0.03f;
	}
	else if( button == 4 )
	{
		if( perspZoom - 0.03f > 0.0f )
			perspZoom -= 0.03f;
	}

	mouse_states[ button ] = ( state == GLUT_DOWN );

	LastMousePosX = x;
	LastMousePosY = y;
}

void passive_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	LastMousePosX = x;
	LastMousePosY = y;
}

void active_motion_func( int x, int y )
{
	if( x < 0 || x > WindowWidth || y < 0 || y > WindowHeight )
		return;

	float px, py;
	window_to_scene( x, y, px, py );

	if( mouse_states[0] == true )
	{
		perspRotationY += ( x - LastMousePosX ) * perspSensitivity;
		perspRotationX += ( y - LastMousePosY ) * perspSensitivity;
	}

	LastMousePosX = x;
	LastMousePosY = y;
}



/*=================================================================================================
	RENDERING
=================================================================================================*/

void display_func( void )
{
	// Clear the contents of the back buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Update transformation matrices
	CreateTransformationMatrices();

	// Choose which shader to user, and send the transformation matrix information to it
	PerspectiveShader.Use();
	PerspectiveShader.SetUniform( "projectionMatrix", glm::value_ptr( PerspProjectionMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "viewMatrix", glm::value_ptr( PerspViewMatrix ), 4, GL_FALSE, 1 );
	PerspectiveShader.SetUniform( "modelMatrix", glm::value_ptr( PerspModelMatrix ), 4, GL_FALSE, 1 );

	// Drawing in wireframe?
	if( draw_wireframe == true )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	else
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	// Bind the axis Vertex Array Object created earlier, and draw it
	glBindVertexArray( axis_VAO );
	glDrawArrays( GL_LINES, 0, 6 ); // 6 = number of vertices in the object

	//
	glBindVertexArray(torus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, Size); // vertices appended based on size.
	//

	// Unbind when done
	glBindVertexArray( 0 );

	// Swap the front and back buffers
	glutSwapBuffers();
}

/*=================================================================================================
	INIT
=================================================================================================*/

void init( void )
{
	// Print some info
	std::cout << "Vendor:         " << glGetString( GL_VENDOR   ) << "\n";
	std::cout << "Renderer:       " << glGetString( GL_RENDERER ) << "\n";
	std::cout << "OpenGL Version: " << glGetString( GL_VERSION  ) << "\n";
	std::cout << "GLSL Version:   " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n\n";

	// Set OpenGL settings
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ); // background color
	glEnable( GL_DEPTH_TEST ); // enable depth test
	glEnable( GL_CULL_FACE ); // enable back-face culling

	// Create shaders
	CreateShaders();

	// Create axis buffers
	CreateAxisBuffers();

	//
	makeTorus(outerRad, innerRad, Slices, Loops);
	generateTorusAt(centerX, centerY, centerZ, outerRad, innerRad, Slices, Loops);
	CreateTorusBuffers();

	//

	std::cout << "Finished initializing...\n\n";
}

/*=================================================================================================
	MAIN
=================================================================================================*/

int main( int argc, char** argv )
{
	// Create and initialize the OpenGL context
	glutInit( &argc, argv );

	glutInitWindowPosition( 100, 100 );
	glutInitWindowSize( InitWindowWidth, InitWindowHeight );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );

	glutCreateWindow( "CSE-170 Computer Graphics" );

	// Initialize GLEW
	GLenum ret = glewInit();
	if( ret != GLEW_OK ) {
		std::cerr << "GLEW initialization error." << std::endl;
		glewGetErrorString( ret );
		return -1;
	}

	// Register callback functions
	glutDisplayFunc( display_func );
	glutIdleFunc( idle_func );
	glutReshapeFunc( reshape_func );
	glutKeyboardFunc( keyboard_func );
	glutKeyboardUpFunc( key_released );
	glutSpecialFunc( key_special_pressed );
	glutSpecialUpFunc( key_special_released );
	glutMouseFunc( mouse_func );
	glutMotionFunc( active_motion_func );
	glutPassiveMotionFunc( passive_motion_func );

	// Do program initialization
	init();

	// Enter the main loop
	glutMainLoop();



	return EXIT_SUCCESS;
}
