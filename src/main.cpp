#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

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

const std::map<HttpMethod, ImVec4> methodColors = {
    {HttpMethod::GET, ImVec4(0.0f, 0.8f, 0.0f, 1.0f)},
    {HttpMethod::POST, ImVec4(0.8f, 0.4f, 0.0f, 1.0f)},
    {HttpMethod::PUT, ImVec4(0.0f, 0.4f, 0.8f, 1.0f)},
    {HttpMethod::DELETE, ImVec4(0.8f, 0.0f, 0.0f, 1.0f)},
    {HttpMethod::PATCH, ImVec4(0.8f, 0.8f, 0.0f, 1.0f)}
};

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

struct KeyValueEditor {
    char newKey[128] = "";
    char newValue[128] = "";
    std::map<std::string, std::pair<char[128], char[128]>> buffers;
};

static std::map<std::string, KeyValueEditor> editors;

void renderKeyValueEditor(const std::string& title, std::map<std::string, std::string>& keyValuePairs) {
    if (ImGui::CollapsingHeader(title.c_str())) {
        ImGui::PushID(title.c_str());

        auto& editor = editors[title];

        // Add new key-value pair
        ImGui::InputText("Key", editor.newKey, sizeof(editor.newKey));
        ImGui::SameLine();
        ImGui::InputText("Value", editor.newValue, sizeof(editor.newValue));
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            if (strlen(editor.newKey) > 0) {
                keyValuePairs[editor.newKey] = editor.newValue;
                editor.newKey[0] = '\0';
                editor.newValue[0] = '\0';
            }
        }

        // Remove old buffers for deleted key-value pairs
        for (auto it = editor.buffers.begin(); it != editor.buffers.end();) {
            if (keyValuePairs.find(it->first) == keyValuePairs.end()) {
                it = editor.buffers.erase(it);
            } else {
                ++it;
            }
        }

        // Create or update buffers for existing key-value pairs
        for (const auto& [key, value] : keyValuePairs) {
            if (editor.buffers.find(key) == editor.buffers.end()) {
                auto& [keyBuf, valueBuf] = editor.buffers[key];
                strncpy(keyBuf, key.c_str(), sizeof(keyBuf));
                strncpy(valueBuf, value.c_str(), sizeof(valueBuf));
            }
        }

        for (auto it = keyValuePairs.begin(); it != keyValuePairs.end();) {
            const std::string& currentKey = it->first;
            ImGui::PushID(currentKey.c_str());

            auto& [keyBuf, valueBuf] = editor.buffers[currentKey];

            bool modified = false;
            modified |= ImGui::InputText("##Key", keyBuf, sizeof(keyBuf));
            ImGui::SameLine();
            modified |= ImGui::InputText("##Value", valueBuf, sizeof(valueBuf));
            ImGui::SameLine();

            if (ImGui::Button("Delete")) {
                it = keyValuePairs.erase(it);
            } else {
                if (modified) {
                    std::string newKey = keyBuf;
                    std::string newValue = valueBuf;

                    if (newKey != currentKey) {
                        // Key changed, remove old and add new
                        it = keyValuePairs.erase(it);
                        keyValuePairs[newKey] = newValue;
                        editor.buffers.erase(currentKey);
                    } else {
                        // Only value changed
                        it->second = newValue;
                        ++it;
                    }
                } else {
                    ++it;
                }
            }

            ImGui::PopID();
        }

        ImGui::PopID();
    }
}

void renderRequestPanel(std::shared_ptr<Request>& req) {
    ImGui::PushID(req.get());

    const char* methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};
    int currentMethod = static_cast<int>(req->method);
    ImGui::PushItemWidth(100);
    if (ImGui::Combo("Method", &currentMethod, methods, IM_ARRAYSIZE(methods))) {
        req->method = static_cast<HttpMethod>(currentMethod);
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::InputText("URL", urlBuffer, sizeof(urlBuffer));

    renderKeyValueEditor("Query Parameters", req->queryParams);
    renderKeyValueEditor("Headers", req->headers);

    if (ImGui::CollapsingHeader("Body")) {
        ImGui::InputTextMultiline("##Body", bodyBuffer, sizeof(bodyBuffer), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 6));
        req->body = bodyBuffer;
    }

    if (ImGui::Button("Send Request")) {
        req->url = urlBuffer;
        makeRequest(req->url.c_str(), req.get());
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Response")) {
        ImGui::InputTextMultiline("##Response", responseBuffer, sizeof(responseBuffer), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::PopID();
}

void renderRequestList() {
    ImGui::Begin("Requests");
    for (size_t i = 0; i < requests.size(); i++) {
        auto& req = requests[i];

        // Push method color
        ImGui::PushStyleColor(ImGuiCol_Text, methodColors.at(req->method));

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
