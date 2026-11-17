// ---------------------------------------------------------------------------
// Простой графический калькулятор на Dear ImGui + GLFW (OpenGL3 backend)
//
// Окно фиксированного размера, два поля ввода (Число А, Число Б),
// четыре кнопки операций в ряд, крупный вывод результата,
// проверка деления на ноль.
// ---------------------------------------------------------------------------

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>

namespace {

// Фиксированный размер окна
constexpr int kWindowWidth  = 480;
constexpr int kWindowHeight = 320;

// Путь к системному шрифту с поддержкой кириллицы
#ifdef _WIN32
    constexpr const char* kCyrillicFontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
#else
    constexpr const char* kCyrillicFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

// Состояние калькулятора
double g_valueA       = 0.0;   // "Число А"
double g_valueB       = 0.0;   // "Число Б"
double g_result       = 0.0;   // результат последней операции
bool   g_hasResult    = false; // была ли выполнена хотя бы одна операция
bool   g_divZeroError = false; // сработала ли ошибка деления на ноль

// Выполнить арифметическую операцию над А и Б
void applyOperation(char op)
{
    switch (op)
    {
    case '+':
        g_result = g_valueA + g_valueB;
        break;
    case '-':
        g_result = g_valueA - g_valueB;
        break;
    case '*':
        g_result = g_valueA * g_valueB;
        break;
    case '/':
        if (g_valueB == 0.0)
        {
            g_divZeroError = true;
            g_hasResult    = true;
            return;
        }
        g_result = g_valueA / g_valueB;
        break;
    default:
        return;
    }

    // Избавляемся от отрицательного нуля (-0) при умножении/делении на ноль
    if (g_result == 0.0)
        g_result = 0.0;

    g_divZeroError = false;
    g_hasResult    = true;
}

// Отрисовать строку результата крупным шрифтом
void drawResult()
{
    if (!g_hasResult)
        return;

    ImGui::SetWindowFontScale(1.7f);

    if (g_divZeroError)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f),
                           "Ошибка: деление на ноль!");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.25f, 1.0f, 0.45f, 1.0f),
                           "Результат: %.10g", g_result);
    }

    ImGui::SetWindowFontScale(1.0f);
}

} // namespace

int main()
{
    // ----------------------------- GLFW ------------------------------------
    if (!glfwInit())
        return 1;

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Запрет изменения размера окна на уровне ОС
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight,
                                          "Калькулятор (Dear ImGui + GLFW)",
                                          nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync

    // ----------------------------- ImGui -----------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Отключаем создание файла imgui.ini рядом с .exe

    ImGui::StyleColorsDark();

    // Загружаем системный шрифт с кириллицей и делаем его шрифтом по умолчанию
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(
        kCyrillicFontPath,
        20.0f,
        nullptr,
        io.Fonts->GetGlyphRangesCyrillic()
    );

    if (mainFont)
    {
        io.FontDefault = mainFont;
    }
    else
    {
        io.Fonts->AddFontDefault();
        std::fprintf(stderr,
                     "Warning: could not load '%s'; Cyrillic will render as '?'.\n",
                     kCyrillicFontPath);
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --------------------------- Main loop ---------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kWindowWidth),
                                            static_cast<float>(kWindowHeight)));

            ImGui::Begin("Калькулятор", nullptr,
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize   |
                         ImGuiWindowFlags_NoMove     |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoBringToFrontOnFocus);

            ImGui::Text("Введите два числа:");
            ImGui::Spacing();

            const float fieldWidth = 220.0f;
            ImGui::PushItemWidth(fieldWidth);
            ImGui::InputDouble("Число А", &g_valueA, 0.0, 0.0, "%.10g");
            ImGui::InputDouble("Число Б", &g_valueB, 0.0, 0.0, "%.10g");
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Четыре кнопки операций в один ряд
            const float btnW = 95.0f;
            const float btnH = 42.0f;

            if (ImGui::Button("+", ImVec2(btnW, btnH)))
                applyOperation('+');

            ImGui::SameLine();
            if (ImGui::Button("-", ImVec2(btnW, btnH)))
                applyOperation('-');

            ImGui::SameLine();
            if (ImGui::Button("*", ImVec2(btnW, btnH)))
                applyOperation('*');

            ImGui::SameLine();
            if (ImGui::Button("/", ImVec2(btnW, btnH)))
                applyOperation('/');

            ImGui::Spacing();
            ImGui::Spacing();

            drawResult();

            ImGui::End();
        }

        // ------------------------------ Render -----------------------------
        int displayW = 0, displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.12f, 0.12f, 0.14f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ------------------------------ Shutdown -------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
