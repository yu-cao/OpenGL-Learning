#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// first line is the version statement: 330 -> OpenGL 3.3
// the Keyword "in" means state all Input Vertex Attribute
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\0";
// the Keyword "out" mean output values of final color
// uniform: global values
const char *fragmentShaderSource = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "uniform vec4 ourColor;"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = ourColor;\n"
                                   "}\n\0";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);// We don't want the old OpenGL

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Initialize GLEW
    glfwMakeContextCurrent(window);

    // Fit the window size change
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);//second para means the number of source string
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    //check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    int shaderProgram = glCreateProgram();// the object Problem
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    // Set the vertices data
    float vertices[] = {
            0.5f, -0.5f, 0.0f,  // bottom right
            -0.5f, -0.5f, 0.0f,  // bottom left
            0.0f,  0.5f, 0.0f   // top
    };
//    unsigned int indices[] = {  // note that we start from 0!
//            0, 1, 3,  // first Triangle
//            1, 2, 3   // second Triangle
//    };

    // Vertex Array Object(VAO) is like array of pointer point to the VBO's different attributes
    // the relation can see the "VAO and VBO relation.png"
    // if you want draw more than one object, you need generate and config all VAO(and necessary VBO and attribute pointer)
    // then when we draw, use VAO, binding it; after draw, unbinding it.
    unsigned int VBO, VAO/*, EBO*/;
    glGenVertexArrays(1, &VAO);

    // first para is the number of buffer object name to be generated
    // second para specifies an array in which the generated buffer object names are stored
    glGenBuffers(1, &VBO);// use the Vertex Buffer Object(VBO) to manager the memory which store the vertices in GPU

    //glGenBuffers(1, &EBO);// like VBO

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    //---------------------
    glBindVertexArray(VAO);// OpenGL order us to use the VAO and OpenGL will know how to handle our vertex input, if binding error OpenGL won't draw anything

    // the vertices' type of buffer is GL_ARRAY_BUFFER
    // now the VBO can control the BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // store the vertices data into the memory of GPU
    // First para: the type of target buffer
    // Second para: the size of data transport, the better is to use "sizeof"
    // Third para: the real data we want to send
    // Fourth para: how to manager these data in GPU
    // GL_STATIC_DRAW: Data won't or hardly be changed; DYNAMIC:change a lot; STREAM: change in each draw
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);// we use static because the triangle vertices won't change in each render

    // EBO: reduce the draw of repeat vertices by ordering the order of how to draw
    // TO learn more, can see the reference "VAO and EBO relation.png"
    // This part is like VBO(bind and tell the data struct in buffer), just change the type of buffer
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Each vertices property get the data from the VBO which is being binding the GL_ARRAY_BUFFER(if more than one VBO)
    // first para: location of vertices property [layout(location = 0) in vertices shader]
    // second para: size of vertices property (vec3 -> 3)
    // third para: type of data (vec3 is combine with float, so GL_FLOAT)
    // fourth para: Normalize or not?
    // fifth para: Stride (the interval of sequent vertices property, we know the x,y,z; so 3 * float)
    // simply speaking: the interval length of "the same property attribute" appear second time to the first one
    // six para: the offset of the buffer beginning
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);//the para is vertices property location to enable the vertices property

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);//set back to the default mode

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // rendering
        // -----
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);// clear the color buffer

        // draw our first triangle
        // -----------------------
        glUseProgram(shaderProgram);// You MUST use the program before updating the uniform, because the shader program must be active

        // How to set the uniform value
        // MUST change in the loop. Otherwise, it will not change the color with time
        float timeValue = glfwGetTime();// program running time
        float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
        int vertexColorLocation = glGetUniformLocation(shaderProgram,"ourColor");// if return -1 mean not find
        glUniform4f(vertexColorLocation,0.0f,greenValue,0.0f,1.0f);// set uniform value

        glBindVertexArray(VAO);// seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

        // first para: the type of we want to draw;
        // second para: beginning index of vertices array;
        // third para: how many vertices we want to draw
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // first para: same as glDrawArrays()
        // second para: the number of vertices we need to draw
        // third para: the type of index
        // fourth para: the offset of EBO or a array of index when you don't use the EBO
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,0);// it will get the index from EBO binding GL_ELEMENT_ARRAY_BUFFER now.
        // glBindVertexArray(0); // no need to unbind it every time
        // ---------------------

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);// draw the screen (Double Buffer)
        glfwPollEvents();// test whether cause some events
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    //glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    //get the ESC been pressed
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    // first two param is control the windows of left-down location, next two control width and height.
    // make (-1,1) (OpenGL support) -> (0,800) or (0,600) (Windows show)
    glViewport(0, 0, width, height);
}