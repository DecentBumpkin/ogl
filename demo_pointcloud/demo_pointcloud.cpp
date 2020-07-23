// Include standard headers
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h> // Include GLEW
#include <GLFW/glfw3.h> // Include GLFW
#include <glm/glm.hpp> // Include GLM
#include <glm/gtc/matrix_transform.hpp>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <playground/Control/camera.h>
#include <playground/Control/trackball.h>
#include <boost/bind.hpp>
using namespace glm;
typedef struct
{
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
} Point;

class Scene 
{
public:
	Scene(){}
	int init();
	void computeMatricesFromInputs();
	void computeMatricesFromInputsPro();
	inline glm::mat4 getViewMatrix() { return ViewMatrix; }
	inline glm::mat4 getProjectionMatrix() { return ProjectionMatrix; }
private:
	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;
	// Initial position : on +Z
	glm::vec3 position = glm::vec3( 10, 0, 0 ); 
	// Initial horizontal angle : toward -Z
	float horizontalAngle = 3.14f;
	// Initial vertical angle : none
	float verticalAngle = 0.0f;
	// Initial Field of View
	float initialFoV = 45.0f;

	float speed = 3.0f; // 3 units / second
	float mouseSpeed = 0.005f;
	GLFWwindow* window;
	static TrackBall	trackball_;
	static Camera		camera_;
	static double xpos_, ypos_;
	// static members
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

};

int main( void )
{
	Scene demo_scene;
	demo_scene.init();
	return EXIT_SUCCESS;
}

int Scene::init(){
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Pointcloud", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
    glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	// When sticky keys mode is enabled, the pollable state of a key will remain GLFW_PRESS
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    // glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");


	Point point_cloud_buffer[] = {
		{-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		{-2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 0.0f},
		{2.0f, -2.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		{0.0f,  2.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{-3.0f, -3.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		{3.0f, -3.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f,  3.0f, 0.0f, 0.0f, 1.0f, 0.0f}
	};
	printf("show size %lu\n",sizeof(Point));



	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(point_cloud_buffer), point_cloud_buffer, GL_DYNAMIC_DRAW);

	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// 1rst attribute buffer : vertices
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			6 * sizeof(GL_FLOAT), // stride
			(void*)0            // array buffer offset
		);
		glVertexAttribPointer(
			1,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			6 * sizeof(GL_FLOAT), // stride
			(void*)(3 * sizeof(GL_FLOAT))      // array buffer offset
		);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		// Draw the triangle !
		glPointSize(3);
		glDrawArrays(GL_POINTS, 0, 9); // 3 indices starting at 0 -> 1 triangle

		glDisableVertexAttribArray(0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}


void Scene::computeMatricesFromInputs(){
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, Scene::cursor_position_callback);
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	lastTime = currentTime;

    ProjectionMatrix = glm::perspective<float>(glm::radians(camera_.get_zoom()), 4.0f/3.0f, 0.1f, 100.0f);
    ViewMatrix = camera_.get_view_matrix() * trackball_.get_rotation_matrix();
}

// static members
void Scene::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if(button == GLFW_MOUSE_BUTTON_RIGHT){
		if(action == GLFW_PRESS){
			printf("moving camera...\n");
			camera_.process_mouse_right_click(xpos_, ypos_);
		}
		else if(action == GLFW_RELEASE){
			printf("release camera...\n");
			camera_.process_mouse_right_release(xpos_, ypos_);
		}
	}
	if(button == GLFW_MOUSE_BUTTON_LEFT){
		if(action == GLFW_PRESS){
			printf("moving trackball...\n");
			trackball_.process_mouse_left_click(xpos_, ypos_);
		}
		else if(action == GLFW_RELEASE){
			printf("release trackball...\n");
			trackball_.process_mouse_left_release(xpos_, ypos_);
		}
	}
}
void Scene::scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	// printf("Mouse wheel x: %5.2lf, y: %5.2lf\n", xoffset, yoffset);
	Point newPoint[] = {
		{-1.0f, 4.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{0.0f, 4.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 4.0f, 0.0f, 1.0f, 1.0f, 1.0f}
	};
	glBufferSubData(GL_ARRAY_BUFFER, 6*sizeof(Point), 3*sizeof(Point), newPoint);
	camera_.process_mouse_scroll(yoffset);
}
void Scene::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// printf("Cursor pos  x: %5.2lf, y: %5.2lf\n", xpos, ypos);
	camera_.process_mouse_right_movement(xpos, ypos);
	trackball_.process_mouse_left_movement(xpos, ypos);
	xpos_ = xpos;
	ypos_ = ypos;
}

TrackBall Scene::trackball_ = TrackBall(1024,768);
Camera Scene::camera_;
double Scene::xpos_;
double Scene::ypos_;