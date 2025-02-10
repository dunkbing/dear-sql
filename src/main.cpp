#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

struct Request {
    std::string name;
    std::string url;
    std::string response;
    bool open = true;
};

static std::vector<std::shared_ptr<Request>> requests;
static int selectedRequest = -1;
static char urlBuffer[1024] = "";
static char responseBuffer[16384] = "";

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void makeRequest(const char* url, Request* req) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            req->response = response;
            strncpy(responseBuffer, response.c_str(), sizeof(responseBuffer));
        }

        curl_easy_cleanup(curl);
    }
}

void renderRequestPanel(std::shared_ptr<Request>& req) {
    ImGui::PushID(req.get());

    ImGui::InputText("URL", urlBuffer, sizeof(urlBuffer));
    if (ImGui::Button("Send Request")) {
        makeRequest(urlBuffer, req.get());
    }

    ImGui::InputTextMultiline("Response", responseBuffer, sizeof(responseBuffer),
        ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);

    ImGui::PopID();
}

int main() {
    std::cout << "Starting application..." << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "API Client", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::cout << "GLFW window created successfully" << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::cout << "ImGui initialized" << std::endl;

    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DockSpace setup
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        // Requests panel
        ImGui::Begin("Requests");
        for (size_t i = 0; i < requests.size(); i++) {
            auto& req = requests[i];
            if (ImGui::Selectable(req->name.c_str(), selectedRequest == i)) {
                selectedRequest = i;
                req->open = true;
            }
        }
        if (ImGui::Button("New Request")) {
            auto req = std::make_shared<Request>();
            req->name = "Request " + std::to_string(requests.size() + 1);
            requests.push_back(req);
            selectedRequest = requests.size() - 1;
        }
        ImGui::End();

        ImGui::Begin("Content");
        if (requests.empty()) {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
            float buttonWidth = 200;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) / 2);
            if (ImGui::Button("Create First Request", ImVec2(buttonWidth, 0))) {
                auto req = std::make_shared<Request>();
                req->name = "Request 1";
                requests.push_back(req);
                selectedRequest = 0;
            }
        } else {
            if (ImGui::BeginTabBar("RequestTabs")) {
                for (auto& req : requests) {
                    if (req->open && ImGui::BeginTabItem(req->name.c_str(), &req->open)) {
                        renderRequestPanel(req);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // End DockSpace
        ImGui::End();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
