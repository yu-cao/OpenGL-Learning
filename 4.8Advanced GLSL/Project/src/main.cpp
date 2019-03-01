#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/MyShader.h"
#include "../include/Camera.h"
#include "../include/Model.h"
#include "init.cpp"

#include <iostream>

unsigned int loadCubemap(std::vector<std::string> faces);

int main()
{
	int errorCode = 0;
	GLFWwindow *window = init(errorCode);
	if(errorCode == -1)
		return -1;

	// 配置OpenGL的全局状态设置
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	//---------------------------------------

    // build and compile our shader program
    // ------------------------------------
    Shader shader("shader/object.vert", "shader/object.frag");
    Shader skyboxShader("shader/skybox.vert","shader/skybox.frag");
    Shader box("shader/box.vert","shader/box.frag");

	// cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glBindVertexArray(0);

	//cube new
	unsigned int NewCubeVAO, NewCubeVBO;
	glGenVertexArrays(1, &NewCubeVAO);
	glGenBuffers(1, &NewCubeVBO);
	glBindVertexArray(NewCubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, NewCubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
	glBindVertexArray(0);

	// skybox VAO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	//Uniform缓冲
	unsigned int uniformBlockIndexObj = glGetUniformBlockIndex(shader.ID,"Matrices");
	unsigned int uniformBlockIndexBox = glGetUniformBlockIndex(box.ID,"Matrices");

	glUniformBlockBinding(shader.ID,uniformBlockIndexObj,0);
	glUniformBlockBinding(box.ID,uniformBlockIndexBox,0);

	//创建一个Uniform缓冲对象本身，绑定到绑定点0
	unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

	//投影矩阵：这里锁定了视角的缩放，fov为45度
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));//在渲染循环开始前将投影矩阵写入缓冲
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//观察矩阵
	glm::mat4 view = camera.GetViewMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));//偏移了一个mat4给前面的投影矩阵
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//--------------------------------

	// load textures
    unsigned int cubeTexture = loadTexture("texture/container.jpg");

	std::vector<std::string> faces
	{
		"texture/skybox/right.jpg",
		"texture/skybox/left.jpg",
		"texture/skybox/top.jpg",
		"texture/skybox/bottom.jpg",
		"texture/skybox/front.jpg",
		"texture/skybox/back.jpg"
	};
	unsigned int cubemapTexture = loadCubemap(faces);

	// shader configuration
	// --------------------
	shader.use();
	shader.setInt("texture1", 0);

	skyboxShader.use();
	skyboxShader.setInt("skybox",0);

	box.use();
	box.setInt("skybox",0);

	// render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // make sure the camera move speed is same in any computer
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// set the view and projection matrix in the uniform block - we only have to do this once per loop iteration.
		glm::mat4 view = camera.GetViewMatrix();
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// cubes
		shader.use();
		glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.75f, 0.75f, 0.0f));
        shader.setMat4("model", model);
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		//new cube
        box.use();
        model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.75f, 0.75f, 0.0f));
        box.setMat4("model",model);
        box.setVec3("cameraPos",camera.Position);
		glBindVertexArray(NewCubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP,cubemapTexture);
		glDrawArrays(GL_TRIANGLES,0,36);
        glBindVertexArray(0);

		// draw skybox as last
		glDepthFunc(GL_LEQUAL);//使得当深度缓冲相等时也能通过测试（因为原来除了箱子Z较小，其他都为1，与背景Z值相等）
		skyboxShader.use();

		//这里的view矩阵要调整，所以不加入Uniform Buffer中
		glm::mat4 view1 = glm::mat4(glm::mat3(camera.GetViewMatrix()));//从视角矩阵中删除变换操作
		skyboxShader.setMat4("view", view1);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f,
												100.0f);
		skyboxShader.setMat4("projection", projection);

		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteVertexArrays(1,&NewCubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &skyboxVAO);
	glDeleteBuffers(1,&NewCubeVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}