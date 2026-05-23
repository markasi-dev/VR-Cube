#include <android_native_app_glue.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "VRTEST", __VA_ARGS__)

static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
static EGLContext context = EGL_NO_CONTEXT;

static ANativeWindow* window = nullptr;

static GLuint program = 0;
static GLuint vbo = 0;

static GLint modelLoc;
static GLint viewLoc;
static GLint projLoc;

static int screenW = 1;
static int screenH = 1;

static const char* vs = R"(#version 300 es

layout(location = 0) in vec3 pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);
}
)";

static const char* fs = R"(#version 300 es

precision mediump float;

out vec4 color;

void main() {
    color = vec4(0.9, 0.3, 0.2, 1.0);
}
)";

float cube[] = {

        // back
        -0.5f,-0.5f,-0.5f,
        0.5f,-0.5f,-0.5f,
        0.5f, 0.5f,-0.5f,

        0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f,

        // front
        -0.5f,-0.5f, 0.5f,
        0.5f,-0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,

        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,

        // left
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f,

        -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,

        // right
        0.5f, 0.5f, 0.5f,
        0.5f, 0.5f,-0.5f,
        0.5f,-0.5f,-0.5f,

        0.5f,-0.5f,-0.5f,
        0.5f,-0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,

        // bottom
        -0.5f,-0.5f,-0.5f,
        0.5f,-0.5f,-0.5f,
        0.5f,-0.5f, 0.5f,

        0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,

        // top
        -0.5f, 0.5f,-0.5f,
        0.5f, 0.5f,-0.5f,
        0.5f, 0.5f, 0.5f,

        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f,-0.5f
};

GLuint compile(GLenum type, const char* src) {

    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {

        char log[1024];

        glGetShaderInfoLog(shader, 1024, nullptr, log);

        LOGI("Shader compile error: %s", log);
    }

    return shader;
}

void create_program() {

    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);

    program = glCreateProgram();

    glAttachShader(program, v);
    glAttachShader(program, f);

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {

        char log[1024];

        glGetProgramInfoLog(program, 1024, nullptr, log);

        LOGI("Program link error: %s", log);
    }

    glDeleteShader(v);
    glDeleteShader(f);

    modelLoc = glGetUniformLocation(program, "model");
    viewLoc  = glGetUniformLocation(program, "view");
    projLoc  = glGetUniformLocation(program, "projection");
}

bool init_gl(ANativeWindow* win) {

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, nullptr, nullptr);

    EGLConfig config;
    EGLint num;

    EGLint attribs[] = {

            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,

            EGL_DEPTH_SIZE, 24,

            EGL_NONE
    };

    eglChooseConfig(display, attribs, &config, 1, &num);

    surface = eglCreateWindowSurface(display, config, win, nullptr);

    EGLint ctxAttribs[] = {

            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
    };

    context = eglCreateContext(
            display,
            config,
            EGL_NO_CONTEXT,
            ctxAttribs);

    eglMakeCurrent(display, surface, surface, context);

    eglQuerySurface(display, surface, EGL_WIDTH, &screenW);
    eglQuerySurface(display, surface, EGL_HEIGHT, &screenH);

    glViewport(0, 0, screenW, screenH);

    glEnable(GL_DEPTH_TEST);

    create_program();

    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(cube),
            cube,
            GL_STATIC_DRAW);

    LOGI("Initialized");

    return true;
}

void render() {

    static float angle = 0.0f;

    angle += 1.0f;

    glViewport(0, 0, screenW, screenH);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::rotate(
            model,
            glm::radians(angle),
            glm::vec3(1.0f, 1.0f, 0.0f));

    glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::perspective(
            glm::radians(70.0f),
            (float)screenW / (float)screenH,
            0.1f,
            100.0f);

    glUniformMatrix4fv(
            modelLoc,
            1,
            GL_FALSE,
            glm::value_ptr(model));

    glUniformMatrix4fv(
            viewLoc,
            1,
            GL_FALSE,
            glm::value_ptr(view));

    glUniformMatrix4fv(
            projLoc,
            1,
            GL_FALSE,
            glm::value_ptr(projection));

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            nullptr);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    eglSwapBuffers(display, surface);
}

void shutdown_gl() {

    if (display != EGL_NO_DISPLAY) {

        eglMakeCurrent(
                display,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT);

        if (vbo)
            glDeleteBuffers(1, &vbo);

        if (program)
            glDeleteProgram(program);

        if (context != EGL_NO_CONTEXT)
            eglDestroyContext(display, context);

        if (surface != EGL_NO_SURFACE)
            eglDestroySurface(display, surface);

        eglTerminate(display);
    }

    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
}

extern "C"
void android_main(struct android_app* app) {

    app_dummy();

    app->onAppCmd = [](android_app* app, int32_t cmd) {

        switch (cmd) {

            case APP_CMD_INIT_WINDOW:

                if (app->window) {

                    window = app->window;

                    init_gl(window);
                }

                break;

            case APP_CMD_TERM_WINDOW:

                shutdown_gl();

                window = nullptr;

                break;
        }
    };

    while (true) {

        int events;
        android_poll_source* source;

        while (ALooper_pollOnce(
                0,
                nullptr,
                &events,
                (void**)&source) >= 0) {

            if (source)
                source->process(app, source);
        }

        if (app->destroyRequested)
            break;

        if (window)
            render();
    }

    shutdown_gl();
}