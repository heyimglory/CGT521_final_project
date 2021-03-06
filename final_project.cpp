#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <ctime>

#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"
#include "VideoMux.h"
#include "imgui_impl_glut.h"

#define BUFFER_OFFSET(i)    ((char*)NULL + (i))

static const std::string vertex_shader1("vertex_shader1.glsl");
static const std::string fragment_shader1("fragment_shader1.glsl");

static const std::string vertex_shader2("vertex_shader2.glsl");
static const std::string tess_control_shader2("tess_tc_shader2.glsl");
static const std::string tess_eval_shader2("tesse_te_shader2.glsl");
static const std::string geometry_shader2("geometry_shader2.glsl");
static const std::string fragment_shader2("fragment_shader2.glsl");

static const float PI = 3.1415926535f;
static glm::vec2 window_size = glm::vec2(1920, 1024);

GLuint shader_program1 = -1;
GLuint shader_program2 = -1;

GLuint fbo_id = -1;       // Framebuffer object,
GLuint rbo_id = -1;       // and Renderbuffer (for depth buffering)
GLuint original_render_texture = -1;  // Texture rendered into.
GLuint original_depth_texture = -1;  // Texture rendered into.

static const std::string house_mesh_name = "Alpine_chalet.obj";
static const std::string house_d_texture_name = "Alpine_chalet_textures/Diffuse_map.png";
static const std::string house_n_texture_name = "Alpine_chalet_textures/Normal_map.png";
static const std::string house_r_texture_name = "Alpine_chalet_textures/Roughness_map.png";
static const std::string house_m_texture_name = "Alpine_chalet_textures/Metallic_map.png";
static const std::string stroke_texture_name = "stroke_tex.png";
static const std::string bg_texture_name = "background.png";

GLuint house_d_texture_id = -1; //Texture map
GLuint house_n_texture_id = -1;
GLuint house_r_texture_id = -1;
GLuint house_m_texture_id = -1;
GLuint stroke_texture_id = -1;
GLuint bg_texture_id = -1;

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

MeshData house_mesh_data1;
MeshData house_mesh_data2;
float time_sec = 0.0f;

glm::vec2 cur_mouse_pos = glm::vec2(0.0f, 0.0f);
glm::vec2 prev_mouse_pos = glm::vec2(0.0f, 0.0f);
glm::vec2 cam_angle = glm::vec2(0.0f, 0.0f);
glm::vec2 scene_offset = glm::vec2(0.0f, 0.0f);
glm::vec3 cam_pos;
float cam_dist = 2.0f;
bool turning = false;
bool dragging = false;

bool check_framebuffer_status();

bool recording = false;
float stroke_width = 0.04;
float stroke_inter = 0.04;

void draw_gui()
{
	ImGui_ImplGlut_NewFrame();

	const int filename_len = 256;
	static char video_filename[filename_len] = "capture.mp4";

	ImGui::InputText("Video filename", video_filename, filename_len);
	ImGui::SameLine();
	if (recording == false)
	{
		if (ImGui::Button("Start Recording"))
		{
			const int w = glutGet(GLUT_WINDOW_WIDTH);
			const int h = glutGet(GLUT_WINDOW_HEIGHT);
			recording = true;
			start_encoding(video_filename, w, h); //Uses ffmpeg
		}
	}
	else
	{
		if (ImGui::Button("Stop Recording"))
		{
			recording = false;
			finish_encoding(); //Uses ffmpeg
		}
	}

	ImGui::SliderFloat("Stroke width", &stroke_width, 0.01, 0.07);
	ImGui::SliderFloat("Stroke interval", &stroke_inter, 0.01, 0.08);
	ImGui::Text("Rotate: Drag with right button     Move: Drag with wheel     Zoom: Scroll");
	ImGui::Image((void*)original_render_texture, ImVec2(102, 76));
	ImGui::SameLine(150);
	ImGui::Image((void*)original_depth_texture, ImVec2(102, 76));

	ImGui::Render();
}

void draw_scene()
{
	// draw background
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
	glDrawBuffer(GL_BACK); // Render to back buffer.

	const int w = glutGet(GLUT_WINDOW_WIDTH);
	const int h = glutGet(GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h); //Render to the full viewport
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer

	int pass_loc = glGetUniformLocation(shader_program1, "pass");
	if (pass_loc != -1)
	{
		glUniform1i(pass_loc, 1);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bg_texture_id);
	int bg_tex_loc = glGetUniformLocation(shader_program1, "bg_texture");
	if (bg_tex_loc != -1)
	{
		glUniform1i(bg_tex_loc, 0);
	}

	glBindVertexArray(quad_vao);
	glDepthMask(GL_FALSE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDepthMask(GL_TRUE);

	// draw the house to texture
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); // Render to FBO.
	GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, draw_buffers);

	//Make the viewport match the FBO texture size.
	int tex_w, tex_h;
	glBindTexture(GL_TEXTURE_2D, original_render_texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);
	glViewport(0, 0, tex_w, tex_h);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pass_loc = glGetUniformLocation(shader_program1, "pass");
	if (pass_loc != -1)
	{
		glUniform1i(pass_loc, 2);
	}

	glm::vec4 cam_up = glm::rotate(cam_angle.y * 180.0f / PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, scene_offset.y, 0.0f, 0.0f);
	cam_pos = glm::vec3(scene_offset.x, cam_dist * sin(cam_angle.y), cam_dist * cos(cam_angle.y));
	glm::mat4 M = glm::translate(glm::vec3(0.0, -cam_up.y, cam_up.z)) * glm::rotate(cam_angle.x * 180.0f / PI,
		glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(house_mesh_data2.mScaleFactor));
	glm::mat4 V = glm::lookAt(cam_pos, glm::vec3(scene_offset.x, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 P = glm::perspective(40.0f, 1.0f, 0.1f, 100.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, house_d_texture_id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, house_n_texture_id);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, house_r_texture_id);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, house_m_texture_id);

	int PVM_loc = glGetUniformLocation(shader_program1, "PVM");
	if(PVM_loc != -1)
	{
		glm::mat4 PVM = P * V * M;
		glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	}

	int M_loc = glGetUniformLocation(shader_program1, "M");
	if (M_loc != -1)
	{
		glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));
	}

	int d_tex_loc = glGetUniformLocation(shader_program1, "d_texture");
	if(d_tex_loc != -1)
	{
		glUniform1i(d_tex_loc, 0);
	}

	int n_tex_loc = glGetUniformLocation(shader_program1, "n_texture");
	if (n_tex_loc != -1)
	{
		glUniform1i(n_tex_loc, 1);
	}

	int r_tex_loc = glGetUniformLocation(shader_program1, "r_texture");
	if (r_tex_loc != -1)
	{
		glUniform1i(r_tex_loc, 2);
	}

	int m_tex_loc = glGetUniformLocation(shader_program1, "m_texture");
	if (m_tex_loc != -1)
	{
		glUniform1i(m_tex_loc, 3);
	}

	int cam_pos_loc = glGetUniformLocation(shader_program1, "cam_pos");
	if (cam_pos_loc != -1)
	{
		glUniform3fv(cam_pos_loc, 1, glm::value_ptr(cam_pos));
	}

	glBindVertexArray(house_mesh_data1.mVao);
	glDrawElements(GL_TRIANGLES, house_mesh_data1.mNumIndices, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void draw_strokes()
{
	glm::vec4 cam_up = glm::rotate(cam_angle.y * 180.0f / PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, scene_offset.y, 0.0f, 0.0f);
	cam_pos = glm::vec3(scene_offset.x, cam_dist * sin(cam_angle.y), cam_dist * cos(cam_angle.y));
	glm::mat4 M = glm::translate(glm::vec3(0.0, -cam_up.y, cam_up.z)) * glm::rotate(cam_angle.x * 180.0f / PI,
		glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(house_mesh_data2.mScaleFactor));
	glm::mat4 V = glm::lookAt(cam_pos, glm::vec3(scene_offset.x, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 P = glm::perspective(40.0f, 1.0f, 0.1f, 100.0f);

	int PVM_loc = glGetUniformLocation(shader_program2, "PVM");
	if (PVM_loc != -1)
	{
		glm::mat4 PVM = P * V * M;
		glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, original_render_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, original_depth_texture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, stroke_texture_id);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bg_texture_id);

	int col_tex_loc = glGetUniformLocation(shader_program2, "color_texture");
	if (col_tex_loc != -1)
	{
		glUniform1i(col_tex_loc, 0);
	}

	int dep_tex_loc = glGetUniformLocation(shader_program2, "depth_texture");
	if (dep_tex_loc != -1)
	{
		glUniform1i(dep_tex_loc, 1);
	}

	int stroke_tex_loc = glGetUniformLocation(shader_program2, "stroke_texture");
	if (stroke_tex_loc != -1)
	{
		glUniform1i(stroke_tex_loc, 2);
	}

	int bg_tex_loc = glGetUniformLocation(shader_program2, "bg_texture");
	if (bg_tex_loc != -1)
	{
		glUniform1i(bg_tex_loc, 3);
	}

	int stroke_width_loc = glGetUniformLocation(shader_program2, "stroke_width");
	if (stroke_width_loc != -1)
	{
		glUniform1f(stroke_width_loc, stroke_width);
	}

	int stroke_inter_loc = glGetUniformLocation(shader_program2, "stroke_inter");
	if (stroke_inter_loc != -1)
	{
	glUniform1f(stroke_inter_loc, stroke_inter);
	}

	int win_size_loc = glGetUniformLocation(shader_program2, "win_size");
	if (win_size_loc != -1)
	{
		glUniform2fv(win_size_loc, 1, glm::value_ptr(window_size));
	}

	glBindVertexArray(house_mesh_data2.mVao);
	glPatchParameteri(GL_PATCH_VERTICES, 3); //number of input verts to the tess. control shader per patch.
	glDrawElements(GL_PATCHES, house_mesh_data2.mNumIndices, GL_UNSIGNED_INT, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
	// ===== draw the scene =====
	glUseProgram(shader_program1);

	draw_scene();

	// ===== add strokes =====
	glUseProgram(shader_program2);

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
	glDrawBuffer(GL_BACK); // Render to back buffer.

	const int w = glutGet(GLUT_WINDOW_WIDTH);
	const int h = glutGet(GLUT_WINDOW_HEIGHT);
	glViewport(0, 0, w, h); //Render to the full viewport

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw_strokes();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
   
	draw_gui();

	glReadBuffer(GL_BACK);
	if (recording == true)
	{
		glFinish();

		glReadBuffer(GL_BACK);
		read_frame_to_encode(&rgb, &pixels, w, h);
		encode_frame(rgb);
	}

	glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();
}

void reload_shader()
{
	GLuint new_shader1 = InitShader(vertex_shader1.c_str(), fragment_shader1.c_str());
	GLuint new_shader2 = InitShader(vertex_shader2.c_str(), tess_control_shader2.c_str(), tess_eval_shader2.c_str(), geometry_shader2.c_str(), fragment_shader2.c_str());

	if(new_shader1 == -1 || new_shader2 == -1) // loading failed
	{
		glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		glClearColor(1.0f, 0.98f, 0.8f, 0.0f);

		if(shader_program1 != -1)
		{
			glDeleteProgram(shader_program1);
		}
		shader_program1 = new_shader1;

		if (shader_program2 != -1)
		{
			glDeleteProgram(shader_program2);
		}
		shader_program2 = new_shader2;
	}
}


// glut keyboard callback function.
// This function gets called when an ASCII key is pressed
void keyboard(unsigned char key, int x, int y)
{
	std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

	switch(key)
	{
		case 'r':
		case 'R':
			reload_shader();     
		break;
	}
}

void motion(int x, int y)
{
	ImGui_ImplGlut_MouseMotionCallback(x, y);

	prev_mouse_pos = cur_mouse_pos;
	cur_mouse_pos.x = x;
	cur_mouse_pos.y = y;
	if (turning)
	{
		cam_angle += 0.02f * (cur_mouse_pos - prev_mouse_pos);
		if (cam_angle.x > 2.0 * PI)
			cam_angle.x -= 2.0 * PI;
		else if (cam_angle.x < -2.0f * PI)
			cam_angle.x += 2.0 * PI;
		if (cam_angle.y > 0.4999f * PI)
			cam_angle.y = 0.4999f * PI;
		else if (cam_angle.y < -0.4999f * PI)
			cam_angle.y = -0.4999f * PI;
	}
	if (dragging)
	{
		scene_offset += 0.002f * glm::vec2(prev_mouse_pos.x - cur_mouse_pos.x, cur_mouse_pos.y - prev_mouse_pos.y);
	}
}

void mouse(int button, int state, int x, int y)
{
	ImGui_ImplGlut_MouseButtonCallback(button, state);
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		turning = true;
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
		turning = false;
	if (button == 3)
	{
		if (cam_dist > 0.7f)
			cam_dist -= 0.1f;
	}
	if (button == 4)
	{
		if (cam_dist < 3.0f)
			cam_dist += 0.1f;
	}
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
		dragging = true;
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP)
		dragging = false;
}

void printGlInfo()
{
	std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
	std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
	std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
	std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;
}

void initOpenGl()
{
	glewInit();

	glEnable(GL_DEPTH_TEST);

	reload_shader();

	// mesh and texture for shader 1
	glUseProgram(shader_program1);
	house_mesh_data1 = LoadMesh(house_mesh_name);
	house_d_texture_id = LoadTexture(house_d_texture_name.c_str());
	house_n_texture_id = LoadTexture(house_n_texture_name.c_str());
	house_r_texture_id = LoadTexture(house_r_texture_name.c_str());
	house_m_texture_id = LoadTexture(house_m_texture_name.c_str());
	stroke_texture_id = LoadTexture(stroke_texture_name.c_str());
	bg_texture_id = LoadTexture(bg_texture_name.c_str());

	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	float quad_data[] = { 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, // pos
		1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // tex coord

	//create vertex buffers for vertex coords
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

	int pos_loc = glGetAttribLocation(shader_program1, "pos_attrib");
	if (pos_loc >= 0)
	{
		glEnableVertexAttribArray(pos_loc);
		glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
	}

	int tex_coord_loc = glGetAttribLocation(shader_program1, "tex_coord_attrib");
	if (tex_coord_loc >= 0)
	{
		glEnableVertexAttribArray(tex_coord_loc);
		glVertexAttribPointer(tex_coord_loc, 2, GL_FLOAT, false, 0, (void*)(12 * sizeof(float)));
	}

	// mesh and texture for shader 2
	glUseProgram(shader_program2);
	house_mesh_data2 = LoadMesh(house_mesh_name);
  
	const int w = glutGet(GLUT_WINDOW_WIDTH);
	const int h = glutGet(GLUT_WINDOW_HEIGHT);
	//Create texture to render pass 1 into.
	//Make the texture width and height be the window width and height.
	glGenTextures(1, &original_render_texture);
	glBindTexture(GL_TEXTURE_2D, original_render_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &original_depth_texture);
	glBindTexture(GL_TEXTURE_2D, original_depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Create renderbuffer for depth.
	GLuint depth_buffer_id;
	glGenRenderbuffers(1, &depth_buffer_id);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);

	//Create the framebuffer object
	glGenFramebuffers(1, &fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, original_render_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, original_depth_texture, 0);

	//Attach depth renderbuffer to FBO
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer_id);

	check_framebuffer_status();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main (int argc, char **argv)
{
	//Configure initial window state
	glutInit(&argc, argv); 
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition (5, 5);
	glutInitWindowSize(window_size.x, window_size.y);
	int win = glutCreateWindow ("Final Project");

	printGlInfo();

	//Register callback functions with glut. 
	glutDisplayFunc(display); 
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(motion);
	glutIdleFunc(idle);

	initOpenGl();
	ImGui_ImplGlut_Init(); // initialize the imgui system

	//Enter the glut event loop.
	glutMainLoop();
	glutDestroyWindow(win);
	return 0;
}

bool check_framebuffer_status() 
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			printf("Framebuffer incomplete, incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("Framebuffer incomplete, missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			printf("Framebuffer incomplete, missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("Framebuffer incomplete, missing read buffer\n");
            return false;
    }
	return false;
}


