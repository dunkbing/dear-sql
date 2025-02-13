#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include "themes.hpp"

static bool dark_theme = true;

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

struct Request {
    std::string name;
    std::string url;
    std::string response;
    HttpMethod method = HttpMethod::GET;
    bool open = true;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> headers;
    std::string body;
};

static std::vector<std::shared_ptr<Request>> requests;
static int selectedRequest = -1;
static char urlBuffer[1024] = "";
static char responseBuffer[16384] = "";
static char queryParamsBuffer[1024] = "";
static char headersBuffer[1024] = "";
static char bodyBuffer[4096] = "";

const char* methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}

const std::map<HttpMethod, ImVec4>& getMethodColors() {
    static const std::map<HttpMethod, ImVec4> darkColors = {
        {HttpMethod::GET, Theme::MOCHA.green},
        {HttpMethod::POST, Theme::MOCHA.peach},
        {HttpMethod::PUT, Theme::MOCHA.blue},
        {HttpMethod::DELETE, Theme::MOCHA.red},
        {HttpMethod::PATCH, Theme::MOCHA.yellow}
    };

    static const std::map<HttpMethod, ImVec4> lightColors = {
        {HttpMethod::GET, Theme::LATTE.green},
        {HttpMethod::POST, Theme::LATTE.peach},
        {HttpMethod::PUT, Theme::LATTE.blue},
        {HttpMethod::DELETE, Theme::LATTE.red},
        {HttpMethod::PATCH, Theme::LATTE.yellow}
    };

    return dark_theme ? darkColors : lightColors;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void makeRequest(const char* url, Request* req) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;
        std::string fullUrl = url;

        if (!req->queryParams.empty()) {
            fullUrl += "?";
            for (const auto& [key, value] : req->queryParams) {
                fullUrl += key + "=" + value + "&";
            }
            fullUrl.pop_back();
        }

        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Set headers
        struct curl_slist* headers = NULL;
        for (const auto& [key, value] : req->headers) {
            std::string header = key + ": " + value;
            headers = curl_slist_append(headers, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        switch (req->method) {
            case HttpMethod::POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                if (!req->body.empty()) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());
                }
                break;
            case HttpMethod::PUT:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                if (!req->body.empty()) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());
                }
                break;
            case HttpMethod::DELETE:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case HttpMethod::PATCH:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
                if (!req->body.empty()) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body.c_str());
                }
                break;
            default:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            req->response = response;
            strncpy(responseBuffer, response.c_str(), sizeof(responseBuffer));
        }

        if (headers) {
            curl_slist_free_all(headers);
        }

        curl_easy_cleanup(curl);
    }
}

struct KeyValuePair {
    bool enabled = true;
    std::array<char, 128> key{};
    std::array<char, 128> value{};

    KeyValuePair() {
        key.fill('\0');
        value.fill('\0');
    }
};

struct KeyValueEditor {
    std::vector<KeyValuePair> buffers;

    KeyValueEditor() {
        KeyValuePair empty{};
        buffers.push_back(empty);
    }
};

static std::map<std::string, KeyValueEditor> editors;

void renderKeyValueEditor(const std::string& title, std::map<std::string, std::string>& keyValuePairs) {
    auto& editor = editors[title];
    ImGui::PushID(title.c_str());

    float availWidth = ImGui::GetContentRegionAvail().x;
    float checkboxWidth = ImGui::GetFrameHeight();
    float deleteButtonWidth = 60.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float inputWidth = (availWidth - checkboxWidth - deleteButtonWidth - spacing * 3) / 2.0f;

    bool needNewEditor = false;

    for (size_t i = 0; i < editor.buffers.size(); i++) {
        ImGui::PushID(static_cast<int>(i));
        auto& pair = editor.buffers[i];

        ImGui::Checkbox("##Enabled", &pair.enabled);
        ImGui::SameLine();

        ImGui::PushItemWidth(inputWidth);
        if (ImGui::InputText("##Key", pair.key.data(), pair.key.size() - 1,
                           ImGuiInputTextFlags_CharsNoBlank | (pair.enabled ? 0 : ImGuiInputTextFlags_ReadOnly))) {
            if (!pair.enabled && pair.key[0] != '\0') {
                pair.enabled = true;
            }
        }
        if (ImGui::IsItemHovered() && pair.key[0] != '\0') {
            ImGui::SetTooltip("%s", pair.key.data());
        }

        ImGui::SameLine();

        // Use size-1 for buffer size to ensure space for null terminator
        ImGui::InputText("##Value", pair.value.data(), pair.value.size() - 1,
                       pair.enabled ? 0 : ImGuiInputTextFlags_ReadOnly);
        if (ImGui::IsItemHovered() && pair.value[0] != '\0') {
            ImGui::SetTooltip("%s", pair.value.data());
        }
        ImGui::PopItemWidth();

        if (editor.buffers.size() > 1 && i < editor.buffers.size() - 1) {
            ImGui::SameLine();
            if (ImGui::Button("Delete", ImVec2(deleteButtonWidth, 0))) {
                editor.buffers.erase(editor.buffers.begin() + i);
                ImGui::PopID();
                break;
            }
        }

        if (i == editor.buffers.size() - 1 && (pair.key[0] != '\0' || pair.value[0] != '\0')) {
            needNewEditor = true;
        }

        ImGui::PopID();
    }

    if (needNewEditor) {
        editor.buffers.push_back(KeyValuePair{});
    }

    keyValuePairs.clear();
    for (const auto& pair : editor.buffers) {
        if (pair.enabled && pair.key[0] != '\0') {
            keyValuePairs[pair.key.data()] = pair.value.data();
        }
    }

    ImGui::PopID();
}

void renderRequestPanel(std::shared_ptr<Request>& req) {
    ImGui::PushID(req.get());

    const char* methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
    int currentMethod = static_cast<int>(req->method);
    ImGui::PushItemWidth(100);
    if (ImGui::Combo("##Method", &currentMethod, methods, IM_ARRAYSIZE(methods))) {
        req->method = static_cast<HttpMethod>(currentMethod);
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::InputText("URL", urlBuffer, sizeof(urlBuffer));

    if (ImGui::Button("Send Request")) {
        req->url = urlBuffer;
        makeRequest(req->url.c_str(), req.get());
    }

    ImGui::Separator();

    // Add static variable for panel height
    static float topPanelHeight = ImGui::GetContentRegionAvail().y * 0.4f;

    // Create top panel for request details
    ImGui::BeginChild("TopPanel", ImVec2(-1.0f, topPanelHeight), true);
    if (ImGui::BeginTabBar("RequestDetailsTabs")) {
        if (ImGui::BeginTabItem("Query Params")) {
            renderKeyValueEditor("Query Parameters", req->queryParams);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Headers")) {
            renderKeyValueEditor("Headers", req->headers);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Body")) {
            ImGui::InputTextMultiline("##Body", bodyBuffer, sizeof(bodyBuffer),
                ImVec2(-1.0f, -1.0f));
            req->body = bodyBuffer;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    // splitter
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.9f, 0.9f, 0.3f));
    ImGui::Button("##splitter", ImVec2(-1, 4.0f));
    if (ImGui::IsItemActive()) {
        float mouseDelta = ImGui::GetIO().MouseDelta.y;
        if (mouseDelta != 0.0f) {
            topPanelHeight += mouseDelta;
            // Add minimum and maximum constraints
            float minHeight = 100.0f;
            float maxHeight = ImGui::GetContentRegionAvail().y - 100.0f;
            topPanelHeight = std::clamp(topPanelHeight, minHeight, maxHeight);
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    ImGui::PopStyleColor(3);

    // Response panel below
    ImGui::BeginChild("ResponsePanel", ImVec2(-1.0f, -1.0f), true);
    if (ImGui::CollapsingHeader("Response", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputTextMultiline("##Response", responseBuffer, sizeof(responseBuffer),
            ImVec2(-1.0f, -1.0f),
            ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::EndChild();

    ImGui::PopID();
}

void renderRequestList() {
    ImGui::Begin("Requests");
    for (size_t i = 0; i < requests.size(); i++) {
        auto& req = requests[i];

        // Push method color
        ImGui::PushStyleColor(ImGuiCol_Text, getMethodColors().at(req->method));

        std::string label = std::string(methodToString(req->method)) + " " + req->name;
        if (ImGui::Selectable(label.c_str(), selectedRequest == i)) {
            selectedRequest = i;
            req->open = true;
            strncpy(urlBuffer, req->url.c_str(), sizeof(urlBuffer));
            std::string queryParamsStr;
            for (const auto& [key, value] : req->queryParams) {
                queryParamsStr += key + "=" + value + "&";
            }
            if (!queryParamsStr.empty()) {
                queryParamsStr.pop_back();
            }
            strncpy(queryParamsBuffer, queryParamsStr.c_str(), sizeof(queryParamsBuffer));
            std::string headersStr;
            for (const auto& [key, value] : req->headers) {
                headersStr += key + ": " + value + "\n";
            }
            strncpy(headersBuffer, headersStr.c_str(), sizeof(headersBuffer));
            strncpy(bodyBuffer, req->body.c_str(), sizeof(bodyBuffer));
            strncpy(responseBuffer, req->response.c_str(), sizeof(responseBuffer));
        }

        ImGui::PopStyleColor();
    }

    if (ImGui::Button("New Request")) {
        auto req = std::make_shared<Request>();
        req->name = "Request " + std::to_string(requests.size() + 1);
        requests.push_back(req);
        selectedRequest = requests.size() - 1;
    }
    ImGui::End();
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
    Theme::ApplyTheme(Theme::MOCHA);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::cout << "ImGui initialized" << std::endl;

    glClearColor(
        dark_theme ? 0.110f : 0.957f,
        dark_theme ? 0.110f : 0.957f,
        dark_theme ? 0.137f : 0.957f,
        1.0f
    );

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
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark Theme", nullptr, dark_theme)) {
                    dark_theme = true;
                    Theme::ApplyTheme(Theme::MOCHA);
                }
                if (ImGui::MenuItem("Light Theme", nullptr, !dark_theme)) {
                    dark_theme = false;
                    Theme::ApplyTheme(Theme::LATTE);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        // Requests panel
        renderRequestList();

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
