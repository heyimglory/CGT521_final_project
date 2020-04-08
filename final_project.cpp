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

static const std::string vertex_shader1("vertex_shader1.glsl");
static const std::string fragment_shader1("fragment_shader1.glsl");

static const std::string vertex_shader2("vertex_shader2.glsl");
static const std::string geometry_shader2("geometry_shader2.glsl");
static const std::string fragment_shader2("fragment_shader2.glsl");

static const float PI = 3.1415926535f;

GLuint shader_program1 = -1;
GLuint shader_program2 = -1;

GLuint quad_vao = -1;
GLuint quad_vbo = -1;

GLuint fbo_id = -1;       // Framebuffer object,
GLuint rbo_id = -1;       // and Renderbuffer (for depth buffering)
GLuint original_render_texture = -1;  // Texture rendered into.
GLuint original_depth_texture = -1;  // Texture rendered into.

static const std::string house_mesh_name = "Alpine_chalet.obj";
static const std::string house_d_texture_name = "Alpine_chalet_textures/Diffuse_map.png";
static const std::string house_n_texture_name = "Alpine_chalet_textures/Normal_map.png";
static const std::string house_r_texture_name = "Alpine_chalet_textures/Roughness_map.png";
static const std::string house_m_texture_name = "Alpine_chalet_textures/Metallic_map.png";

GLuint house_d_texture_id = -1; //Texture map
GLuint house_n_texture_id = -1;
GLuint house_r_texture_id = -1;
GLuint house_m_texture_id = -1;

MeshData house_mesh_data1;
MeshData house_mesh_data2;
float time_sec = 0.0f;

glm::vec2 cur_mouse_pos = glm::vec2(0.0f, 0.0f);
glm::vec2 prev_mouse_pos = glm::vec2(0.0f, 0.0f);
glm::vec2 cam_angle = glm::vec2(0.0f, 0.0f);
float cam_dist = 2.0f;
bool dragging = false;

bool check_framebuffer_status();

bool recording = false;

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

	ImGui::Image((void*)original_render_texture, ImVec2(102, 76));
	ImGui::SameLine(150);
	ImGui::Image((void*)original_depth_texture, ImVec2(102, 76));

	ImGui::Render();
}

void draw_pass_1()
{
   glm::mat4 M = glm::rotate(cam_angle.x * 180.0f / PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(house_mesh_data1.mScaleFactor));
   glm::mat4 V = glm::lookAt(glm::vec3(0.0f, cam_dist * sin(cam_angle.y), cam_dist * cos(cam_angle.y)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
      glm::mat4 PVM = P*V*M;
      glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }

   int d_tex_loc = glGetUniformLocation(shader_program1, "d_texture");
   if(d_tex_loc != -1)
   {
      glUniform1i(d_tex_loc, 0); // we bound our texture to texture unit 0
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

   glBindVertexArray(house_mesh_data1.mVao);
   glDrawElements(GL_TRIANGLES, house_mesh_data1.mNumIndices, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);

}

void draw_pass_2()
{
   glm::mat4 M = glm::rotate(cam_angle.x * 180.0f / PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(house_mesh_data2.mScaleFactor));
   glm::mat4 V = glm::lookAt(glm::vec3(0.0f, cam_dist * sin(cam_angle.y), cam_dist * cos(cam_angle.y)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(40.0f, 1.0f, 0.1f, 100.0f);

   int PVM_loc = glGetUniformLocation(shader_program2, "PVM");
   if (PVM_loc != -1)
   {
	   glm::mat4 PVM = P * V*M;
	   glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }

   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, original_render_texture);

   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, original_depth_texture);

   int col_tex_loc = glGetUniformLocation(shader_program2, "color_texture");
   if (col_tex_loc != -1)
   {
	   glUniform1i(col_tex_loc, 4);
   }

   int dep_tex_loc = glGetUniformLocation(shader_program2, "depth_texture");
   if (dep_tex_loc != -1)
   {
	   glUniform1i(dep_tex_loc, 5);
   }

   glBindVertexArray(house_mesh_data2.mVao);
   glDrawElements(GL_TRIANGLES, house_mesh_data2.mNumIndices, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
	// ===== pass 1 =====
   glUseProgram(shader_program1);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id); // Render to FBO.
   GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
   glDrawBuffers(2, draw_buffers);

   //Make the viewport match the FBO texture size.
   int tex_w, tex_h;
   glBindTexture(GL_TEXTURE_2D, original_render_texture);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_w);
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_h);
   glViewport(0, 0, tex_w, tex_h);

   //Clear the FBO.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Lab assignment: don't forget to also clear depth
   draw_pass_1();

   // ===== pass 2 =====
   glBindFramebuffer(GL_FRAMEBUFFER, 0); // Do not render the next pass to FBO.
   glDrawBuffer(GL_BACK); // Render to back buffer.

   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   glViewport(0, 0, w, h); //Render to the full viewport
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear the back buffer
   // draw depth buffer
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
   /*glm::mat4 M = glm::rotate(cam_angle.x * 180.0f / PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(house_mesh_data1.mScaleFactor));
   //M = glm::translate(M, glm::vec3(0.0f, 0.0f, 0.1f));
   glm::mat4 V = glm::lookAt(glm::vec3(0.0f, cam_dist * sin(cam_angle.y), cam_dist * cos(cam_angle.y)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(40.0f, 1.0f, 0.1f, 100.0f);
   int PVM_loc = glGetUniformLocation(shader_program1, "PVM");
   if (PVM_loc != -1)
   {
	   glm::mat4 PVM = P * V*M;
	   glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }*/
   glBindVertexArray(house_mesh_data1.mVao);
   glDrawElements(GL_TRIANGLES, house_mesh_data1.mNumIndices, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

   glUseProgram(shader_program2);
   glDepthMask(GL_FALSE);
   //glEnable(GL_POLYGON_OFFSET_FILL);
   //glPolygonOffset(-2.0, -0.5);
   glDisable(GL_CULL_FACE);
   draw_pass_2();
   glEnable(GL_CULL_FACE);
   //glDisable(GL_POLYGON_OFFSET_FILL);
   glDepthMask(GL_TRUE);
   
   draw_gui();

   /*if (recording == true)
   {
	   glFinish();

	   glReadBuffer(GL_BACK);
	   read_frame_to_encode(&rgb, &pixels, w, h);
	   encode_frame(rgb);
   }*/

   glutSwapBuffers();
}

void idle()
{
	glutPostRedisplay();

   /*const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;
   int time_loc = glGetUniformLocation(shader_program1, "time");
   if (time_loc != -1)
   {
	   glUniform1f(time_loc, time_sec);
   }

   int mouse_pos_loc1 = glGetUniformLocation(shader_program1, "mouse_pos");
   if (mouse_pos_loc1 != -1)
   {
	   glUniform2f(mouse_pos_loc1, cur_mouse_pos.x, cur_mouse_pos.y);
   }

   int mouse_pos_loc2 = glGetUniformLocation(shader_program2, "mouse_pos");
   if (mouse_pos_loc2 != -1)
   {
	   glUniform2f(mouse_pos_loc2, cur_mouse_pos.x, cur_mouse_pos.y);
   }*/
}

void reload_shader()
{
   GLuint new_shader1 = InitShader(vertex_shader1.c_str(), fragment_shader1.c_str());
   GLuint new_shader2 = InitShader(vertex_shader2.c_str(), geometry_shader2.c_str(), fragment_shader2.c_str());

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

      /*if(house_mesh_data1.mVao != -1)
      {
         //BufferIndexedVerts(house_mesh_data1);
		  glUseProgram(shader_program1);
		  house_mesh_data1 = LoadMesh(house_mesh_name);
      }

	  if (house_mesh_data2.mVao != -1)
	  {
		  //BufferIndexedVerts(house_mesh_data2);
		  glUseProgram(shader_program2);
		  house_mesh_data2 = LoadMesh(house_mesh_name);
	  }*/
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
	if (dragging)
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
}

void mouse(int button, int state, int x, int y)
{
	ImGui_ImplGlut_MouseButtonCallback(button, state);
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		dragging = true;
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
		dragging = false;
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

   //mesh and texture for pass 1
   glUseProgram(shader_program1);
   house_mesh_data1 = LoadMesh(house_mesh_name);
   house_d_texture_id = LoadTexture(house_d_texture_name.c_str());
   house_n_texture_id = LoadTexture(house_n_texture_name.c_str());
   house_r_texture_id = LoadTexture(house_r_texture_name.c_str());
   house_m_texture_id = LoadTexture(house_m_texture_name.c_str());


   glUseProgram(shader_program2);
   house_mesh_data2 = LoadMesh(house_mesh_name);

   //mesh for pass 2 (full screen quadrilateral)
   glGenVertexArrays(1, &quad_vao);
   glBindVertexArray(quad_vao);

   float vertices[] = {1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f};

   //create vertex buffers for vertex coords
   glGenBuffers(1, &quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   int pos_loc = glGetAttribLocation(shader_program2, "pos_attrib");
   if(pos_loc >= 0)
   {
      glEnableVertexAttribArray(pos_loc);
	   glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);
   }
  
   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   //Create texture to render pass 1 into.
   //Lab assignment: make the texture width and height be the window width and height.
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

   //Lab assignment: Create renderbuffer for depth.
   GLuint depth_buffer_id;
   glGenRenderbuffers(1, &depth_buffer_id);
   glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer_id);
   glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);

   //Create the framebuffer object
   glGenFramebuffers(1, &fbo_id);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, original_render_texture, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, original_depth_texture, 0);

   //Lab assignment: attach depth renderbuffer to FBO
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
   glutInitWindowSize (1024, 768);
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


