// fdik.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <vector>
#include <map>
#include <boost/crc.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <variant>
#include <chrono>

using namespace std;

GLFWwindow* window;

void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

map<string, string> props;

string toHex(const void * data, size_t size)
{
    char buf[16];
    string s;
    for (size_t i = 0; i < size; i++) {
        sprintf_s(buf, "%02x ", ((uint8_t*)data)[i]);
        s += buf;
    }
    s.pop_back();
    return s;
}

string toBin(const void* data, size_t size)
{
    string s;
    for (size_t i = 0; i < size; i++) {
        uint8_t u = ((uint8_t*)data)[i];
        
        for (int i = 0; i < 8; i++) {
            s += to_string(bool((u<<i)&0x80));
        }
        
        s += " ";
    }
    s.pop_back();
    return s;
}

bool cont(vector<uint8_t> data, vector<uint8_t> subdata)
{
    return equal(data.begin(), data.begin() + min(data.size(), subdata.size()), subdata.begin());
}

void refresh(GLFWwindow* window)
{
    // Poll events
    glfwPollEvents();

    // Start a new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Your ImGui code here
    ImGui::Begin("Simple Text Box"); // Create a window
    for (auto i : props) {
        ImGui::Text((i.first + ":\t" + i.second).c_str());
    }
    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers
    glfwSwapBuffers(window);
}

auto start = chrono::steady_clock::now();

void packetReceived(const vector<uint8_t> &data)
{
    if(data[0]==0xa5)
        cout << chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - start).count() << ": ";

    static int propid = -1;
    size_t len = data[3];

    if (data[0] == 0xa5) {
        propid = -1;
    }

    if (cont(data, { 0xa5, 0x30, 0x00, 0x01 })) {
        propid = data[4];
        cout << "read prop ";
        switch (propid) {
        case 0x03: cout << "pwrMax"; break;
        case 0x04: cout << "pwrMin"; break;
        case 0x05: cout << "pumpFlow"; break;
        case 0x0b: cout << "error"; break;
        case 0x0c: cout << "state"; break;
        case 0x20: cout << "burnerTemp"; break;
        case 0x24: cout << "voltage"; break;
        case 0x50: cout << "compAct"; break;
        default: printf("%02x", propid);
        }
        cout << "\n";
    }
    else if (propid != -1 && cont(data, { 0xa0, 0x00, 0x00 })) {
        uint32_t prop = 0;
        

        for (size_t i = 0; i < len; i++) {
            prop += data[4 + i] << (i * 8);
        }

        if (propid == 0x03) {
            float val = *(float*)(&data[4]);
            props["pwrMax"] = to_string(val);
        }
        else if (propid == 0x04) {
            float val = *(float*)(&data[4]);
            props["pwrMin"] = to_string(val);
        }
        else if (propid == 0x05) {
            float val = *(float*)(&data[4]);
            props["pumpFlow"] = to_string(val);
        }
        else if (propid == 0x0b) {
            props["error"] = to_string(prop);
        }
        else if (propid == 0x0c) {
            props["state"] = to_string(prop);
        }
        else if (propid == 0x20) {
            float val = *(float*)(&data[4]);
            props["burnerTemp"] = to_string(val);
        }
        else if (propid == 0x24) {
            float val = *(float*)(&data[4]);
            props["voltage"] = to_string(val);
        }
        else if (propid == 0x66) {
            string modes[] = { "Normal", "Prime", "Fan" };
            props["mode"] = modes[data[4]];
        }
        else if (propid == 0x50) {
            uint8_t bf = data[4];

            props["compAct"] = toBin(&bf, len)
                + ((bf & (1 << 0)) ? " glowPlug" : "")
                + ((bf & (1 << 1)) ? " injector" : "")
                + ((bf & (1 << 2)) ? " fan" : "")
                + ((bf & (1 << 3)) ? " heating" : "")
                + ((bf & (1 << 5)) ? " turned_on" : "");
        }
        else {
            char buf[16];
            sprintf_s(buf, "%02x", propid);

            if (propid == 1) {
                props[buf] = toHex(&data[4], len);
            }

            else if (propid == 0x23 || propid == 0x66) {
                props[buf] = toBin(&data[4], len);
            }
            else
                props[buf] = to_string(prop);
        }

        refresh(window);
    }
    else if (cont(data, { 0xa5, 0x20, 0x00, 0x05, 0x00 })) {
        float val = *(float*)(&data[5]);
        props["temp"] = to_string(val);
        printf_s("send room temp: %.2f\n", val);
    }
    else if (cont(data, { 0xa5, 0x01, 0x00, 0x05 })) {
        float val = *(float*)(&data[5]);
        if (data[4] == 0) {
            props.erase("setPwr");
            props.erase("setFan");
            props["setTemp"] = to_string(val);
            printf_s("set TEMP: %.2f\n", val);
        }
        else if (data[4] == 1) {
            props.erase("setTemp");
            props.erase("setFan");
            props["setPwr"] = to_string(val);
            printf_s("set POWER: %.2f\n", val);
        }
        else if (data[4] == 5) {
            uint32_t val = *(uint32_t*)(&data[5]);
            props.erase("setTemp");
            props.erase("setPwr");
            props["setFan"] = to_string(val);
            printf_s("set fan SPEED: %d\n", val);
        }
        else {
            uint32_t val = *(uint32_t*)(&data[5]);
            props["setUknown" + to_string(data[4])] = to_string(val);
            printf_s("set UKNOWN: %d\n", val);
        }
    }
    else if (cont(data, { 0xa5, 0x20, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00 })) {
        cout << "HEARTBEAT\n";
    }
    else if (cont(data, { 0xa5, 0x01, 0x00, 0x02, 0x02 })) {
        string modes[] = {"Normal", "Prime", "Fan"};
        printf_s("set MODE: %s\n", modes[data[5]].c_str());
    }
    else if (cont(data, { 0xa5, 0x01, 0x00, 0x02, 0x03 })) {

        printf_s(data[5]?"turn ON\n":"turn OFF\n");
        }
    else 
    {
        for (int i = 0; i <  5 + len; i++) {
            printf("%02x ", data[i]);
        }
        cout << "\n";
    }

    if (data[0] == 0xa0) {
        propid = -1;
    }
}



int main()
{
    HANDLE h_Serial;
    h_Serial = CreateFileA("COM1", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 0);
    if (h_Serial == INVALID_HANDLE_VALUE) {
        auto err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            cout << "ERROR_FILE_NOT_FOUND\n";
        }
        cout << "CreateFileA ERROR: " << err << "\n";
        return err;
    }

    DCB dcbSerialParam = { 0 };
    dcbSerialParam.DCBlength = sizeof(dcbSerialParam);

    if (!GetCommState(h_Serial, &dcbSerialParam)) {
        // handle error here
    }

    dcbSerialParam.BaudRate = CBR_9600;
    dcbSerialParam.ByteSize = 8;
    dcbSerialParam.StopBits = ONESTOPBIT;
    dcbSerialParam.Parity = NOPARITY;

    if (!SetCommState(h_Serial, &dcbSerialParam)) {
        auto err = GetLastError();
        cout << "SetCommState ERROR: " << err << "\n";
        return err;
    }

    COMMTIMEOUTS timeout = { 0 };
    timeout.ReadIntervalTimeout = 60;
    timeout.ReadTotalTimeoutConstant = 60;
    timeout.ReadTotalTimeoutMultiplier = 15;
    timeout.WriteTotalTimeoutConstant = 60;
    timeout.WriteTotalTimeoutMultiplier = 8;
    if (!SetCommTimeouts(h_Serial, &timeout)) {
        auto err = GetLastError();
        cout << "SetCommTimeouts ERROR: " << err << "\n";
        return err;
    }


    uint8_t sBuff[16] = { 0 };
    DWORD dwRead = 0;
    vector<uint8_t> data;


    // Set up GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    // Create a GLFW window
    window = glfwCreateWindow(800, 600, "Dear ImGui Example", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader (GLAD or GLEW required)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize OpenGL loader" << std::endl;
        return -1;
    }

    // Set up Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); // Configurations (if needed)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls

    // Set up Dear ImGui style
    ImGui::StyleColorsDark();

    // Initialize Dear ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");


    // Main application loop
    while (!glfwWindowShouldClose(window)) {


        if (ReadFile(h_Serial, sBuff, 1, &dwRead, NULL) && dwRead) {
            //printf("%02x ", sBuff[0]);
            data.push_back(sBuff[0]);
        }
        else {
            refresh(window);
            continue;
        }

        auto res = data.end();
        for (size_t i = 0; i < data.size(); i++) {
            if (data[i] == 0xa5 || data[i] == 0xa0) {
                res = data.begin() + i;
                break;
            }
        }

        if (data.size() && res != data.begin()) {
            cout << ">>>>> UKNOWN DATA  [";
            for (auto i = data.begin(); i != res; i++) {
                printf("%02x ", *i);
            }
            cout << "]\n";
            data.erase(data.begin(), res);
        }


        if (data.size() > 4) {
            size_t len = data[3];
            size_t packetSize = 5 + len;
            if (data.size() >= packetSize) {
                boost::crc_optimal<8, 0x31, 0, 0, true, true> result;
                result.process_bytes(data.data(), packetSize);
                uint8_t crc1 = result.checksum();
                if (!crc1) {
                    packetReceived(data);
                    data.erase(data.begin(), data.begin() + packetSize);
                    
                    
                }
                else {
                    cout << ">>>>> BAD CRC\n";
                    data.erase(data.begin());
                }
            }
        }


    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;



    

    ImGui::Text("Hello, world %d", 123);

    while (1) {

    }



}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
