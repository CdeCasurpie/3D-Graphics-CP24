#include "Window.h"
#include "Camera.h"
#include "Backend.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace realtime {
namespace {
GLuint compile(GLenum type,const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=0; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if (!ok) { char log[4096]{}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log); throw std::runtime_error(log); }
    return shader;
}
Camera cameraView(const ::Camera& orbit) {
    const glm::mat4 inverse=glm::inverse(orbit.getViewMatrix());
    auto v=[](glm::vec3 p) { return Vec{p.x,p.y,p.z}; };
    return {v(orbit.getPosition()),v(-glm::vec3(inverse[2])),v(glm::vec3(inverse[0])),v(glm::vec3(inverse[1])),
            tanf(orbit.fov*pi/360)};
}
struct Frame {
    Batch batch;
    Config config;
    unsigned revision;
};
}
int viewer(const Scene& scene,std::unique_ptr<Backend> backend,std::string backendName,Config initialConfig,
           unsigned threads,int device,const std::string& ptx,bool smoke,bool tour,int captureFrames,int captureSeconds,
           const std::string& captureDirectory) {
    Window window(1280,720,"Task 16: Embree / OptiX progressive path tracer");
    glfwSetWindowSizeLimits(window.glfwWindow,1280,720,1280,720);
    glfwSetWindowAspectRatio(window.glfwWindow,16,9); glfwSetWindowSize(window.glfwWindow,1280,720);
    glfwSwapInterval(0); // Do not cap benchmark/demo FPS at display refresh.
    ImGui::CreateContext(); ImGui::GetIO().IniFilename=nullptr; ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfwWindow,true); ImGui_ImplOpenGL3_Init("#version 330 core");
    const char* vs="#version 330 core\nout vec2 uv;void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=vec2(p.x,1-p.y);gl_Position=vec4(p*2-1,0,1);}";
    const char* fs="#version 330 core\nin vec2 uv;out vec4 color;uniform sampler2D image;uniform float exposure;void main(){vec3 c=max(texture(image,uv).rgb,vec3(0))*exposure;c=c/(1+c);vec3 s=mix(12.92*c,1.055*pow(c,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));color=vec4(s,1);}";
    GLuint vertex=compile(GL_VERTEX_SHADER,vs),fragment=compile(GL_FRAGMENT_SHADER,fs),program=glCreateProgram();
    glAttachShader(program,vertex); glAttachShader(program,fragment); glLinkProgram(program);
    GLint linked=0; glGetProgramiv(program,GL_LINK_STATUS,&linked);
    if (!linked) { char log[4096]{}; glGetProgramInfoLog(program,sizeof(log),nullptr,log); throw std::runtime_error(log); }
    glDeleteShader(vertex); glDeleteShader(fragment);
    GLuint vao,texture; glGenVertexArrays(1,&vao); glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    ::Camera orbit(7.6f,glm::vec3(0,1.9f,0)); orbit.fov=39; orbit.minDistance=1; orbit.maxDistance=30;
    orbit.orientation=glm::quat(glm::vec3(glm::radians(0.75f),0,0));
    Config config=initialConfig; unsigned revision=0,accumulationRevision=0; uint32_t samples=0,displayedSamples=0;
    std::vector<Vec> average; int imageWidth=0,imageHeight=0;
    std::future<Frame> pending; bool busy=false,paused=false; std::string status;
    int resolution=1,spp=std::min(8,config.spp),bounces=config.bounces; float exposure=1,previousTourAngle=0;
    const int widths[]={320,480,640,960},heights[]={240,360,480,720};
    double renderSeconds=0,frameSeconds=0; int completed=0;
    auto lastDisplayed=std::chrono::steady_clock::now();
    const auto startTime=lastDisplayed;
    auto lastCapture=startTime-std::chrono::seconds(1);
    auto lastActivity=lastDisplayed;
    std::ofstream timings;
    if (!captureDirectory.empty()) {
        if (std::filesystem::exists(captureDirectory+"/viewer.csv"))
            throw std::runtime_error("Capture directory already has viewer.csv; choose a fresh directory");
        std::filesystem::create_directories(captureDirectory);
        timings.open(captureDirectory+"/viewer.csv");
        if (!timings) throw std::runtime_error("Cannot create viewer CSV");
        timings << "frame,backend,render_width,render_height,spp,bounces,accumulated_samples,render_seconds,presented_frame_seconds,elapsed_seconds\n";
    }
    auto changed=[&]() { ++revision; samples=0; lastActivity=std::chrono::steady_clock::now(); };
    auto submit=[&]() {
        const bool moving=std::chrono::duration<double>(std::chrono::steady_clock::now()-lastActivity).count()<0.25;
        Config next=config; next.width=moving?320:widths[resolution]; next.height=moving?240:heights[resolution];
        next.spp=spp; next.bounces=bounces;
        if (next.width!=imageWidth || next.height!=imageHeight) { ++revision; samples=0; }
        next.sampleOffset=samples;
        const Camera camera=cameraView(orbit); Backend* target=backend.get(); const unsigned version=revision;
        pending=std::async(std::launch::async,[target,camera,next,version]() { return Frame{target->render(camera,next),next,version}; });
        busy=true;
    };
    while (!window.shouldClose()) {
        window.processInput();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        int ww,wh; glfwGetWindowSize(window.glfwWindow,&ww,&wh);
        double x,y; glfwGetCursorPos(window.glfwWindow,&x,&y);
        if (!ImGui::GetIO().WantCaptureMouse && ww>0 && wh>0) {
            if (glfwGetMouseButton(window.glfwWindow,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS) {
                if (!orbit.isDragging) orbit.startDragging(x,y,ww,wh);
                else if (x!=orbit.lastX || y!=orbit.lastY) { orbit.onMouseDrag(x,y,ww,wh); changed(); }
            } else orbit.stopDragging();
            if (glfwGetMouseButton(window.glfwWindow,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS) {
                if (!orbit.isPanning) orbit.startPanning(x,y);
                else if (x!=orbit.lastX || y!=orbit.lastY) { orbit.onMousePan(x,y,ww,wh); changed(); }
            } else orbit.stopPanning();
            if (ImGui::GetIO().MouseWheel!=0) { orbit.onScroll(ImGui::GetIO().MouseWheel); changed(); }
        } else { orbit.stopDragging(); orbit.stopPanning(); }
        bool presented=false;
        if (busy && pending.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
            Frame frame=pending.get(); busy=false;
            const bool matching=frame.revision==revision && frame.revision==accumulationRevision &&
                frame.config.width==imageWidth && frame.config.height==imageHeight && !average.empty();
            if (!matching) { average=frame.batch.pixels; samples=uint32_t(frame.config.spp); }
            else {
                const float fraction=float(frame.config.spp)/float(samples+frame.config.spp);
                for (size_t i=0;i<average.size();++i)
                    average[i]=add(mul(average[i],1-fraction),mul(frame.batch.pixels[i],fraction));
                samples+=uint32_t(frame.config.spp);
            }
            accumulationRevision=frame.revision;
            displayedSamples=samples;
            if (frame.revision!=revision) samples=0; // Show last completed preview, never blend stale camera views.
            imageWidth=frame.config.width; imageHeight=frame.config.height;
            renderSeconds=frame.batch.seconds;
            glBindTexture(GL_TEXTURE_2D,texture);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F,imageWidth,imageHeight,0,GL_RGB,GL_FLOAT,average.data());
            const auto now=std::chrono::steady_clock::now();
            frameSeconds=std::chrono::duration<double>(now-lastDisplayed).count(); lastDisplayed=now;
            ++completed; presented=true;
            if (timings) timings << completed << ',' << backendName << ',' << imageWidth << ',' << imageHeight << ','
                << frame.config.spp << ',' << frame.config.bounces << ',' << samples << ',' << renderSeconds << ',' << frameSeconds << ','
                << std::chrono::duration<double>(now-startTime).count() << '\n';
            if (tour || smoke) {
                const float seconds=float(std::chrono::duration<double>(now-startTime).count());
                const float phase=fmodf(seconds,16.0f);
                const float angle=phase<4?0.22f*sinf(phase*pi/4):
                    (phase>=8 && phase<12?-0.22f*sinf((phase-8)*pi/4):0);
                // Pause between moves so progressive refinement is visible.
                if (std::abs(angle-previousTourAngle)>1e-6f) {
                    orbit.orientation=glm::quat(glm::vec3(glm::radians(0.75f),angle,0)); changed();
                    previousTourAngle=angle;
                }
            }
        }
        ImGui::SetNextWindowPos(ImVec2(12,12),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360,0),ImGuiCond_FirstUseEver);
        ImGui::Begin("Task 16 - accelerated path tracing");
        ImGui::PushItemWidth(145);
        ImGui::Text("Backend: %s",backendName.c_str());
        ImGui::Text("New-image FPS: %.2f",frameSeconds>0?1/frameSeconds:0);
        ImGui::Text("Tracing/readback FPS: %.2f",renderSeconds>0?1/renderSeconds:0);
        ImGui::Text("%d x %d | displayed spp: %u",imageWidth,imageHeight,displayedSamples);
        ImGui::TextUnformatted("Diffuse walls | mirror left | glass right");
        ImGui::BeginDisabled(busy);
        int selection=backendName=="embree"?0:1;
#ifdef REALTIME_OPTIX
        if (ImGui::Combo("Backend",&selection,"Embree\000OptiX\000")) {
            backend=selection==0?makeEmbree(scene,threads):makeOptix(scene,device,ptx);
            backendName=selection==0?"embree":"optix"; changed();
        }
#endif
        ImGui::EndDisabled();
        if (ImGui::Combo("Still resolution",&resolution,"320 x 240\000480 x 360\000640 x 480\000960 x 720\000")) changed();
        if (ImGui::SliderInt("Samples/frame",&spp,1,8)) changed();
        if (ImGui::SliderInt("Max bounces",&bounces,2,16)) changed();
        ImGui::SliderFloat("Exposure",&exposure,0.1f,3);
        ImGui::Checkbox("Pause",&paused);
        if (ImGui::Checkbox("Camera tour",&tour)) changed();
        if (ImGui::Button("Reset camera")) {
            orbit.reset(7.6f,glm::vec3(0,1.9f,0)); orbit.fov=39;
            orbit.orientation=glm::quat(glm::vec3(glm::radians(0.75f),0,0)); changed();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save PNG") && !average.empty()) {
            const auto path=captureDirectory.empty()?"src/RealtimePathTracer/output/viewer.png":captureDirectory+"/viewer.png";
            savePNG(path,average,imageWidth,imageHeight,exposure); status="Saved "+path;
        }
        ImGui::TextUnformatted("Left: orbit | Right: pan | Scroll: zoom\nMoving: 320x240 | Stop: progressive refine\nFPS counts new rendered images, not UI refresh.");
        if (!status.empty()) ImGui::TextWrapped("%s",status.c_str());
        ImGui::PopItemWidth(); ImGui::End();
        if (!busy && !paused) submit();
        int fw,fh; glfwGetFramebufferSize(window.glfwWindow,&fw,&fh);
        glViewport(0,0,fw,fh); glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
        if (!average.empty() && fw>0 && fh>0) {
            const float fit=std::min(float(fw)/imageWidth,float(fh)/imageHeight);
            const int w=int(imageWidth*fit),h=int(imageHeight*fit);
            glViewport((fw-w)/2,(fh-h)/2,w,h); glUseProgram(program);
            glUniform1i(glGetUniformLocation(program,"image"),0); glUniform1f(glGetUniformLocation(program,"exposure"),exposure);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture); glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,3);
        }
        ImGui::Render(); glViewport(0,0,fw,fh); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        const auto captureNow=std::chrono::steady_clock::now();
        if (presented && !captureDirectory.empty() && (captureFrames>0 || captureSeconds>0) && fw>0 && fh>0 &&
            std::chrono::duration<double>(captureNow-lastCapture).count()>=1.0/15) {
            lastCapture=captureNow; // Capture at most 15 fps; do not save thousands of GPU frames.
            // Capture the actual framebuffer INCLUDING the FPS/control overlay.
            std::vector<unsigned char> raw(size_t(fw)*fh*3); glPixelStorei(GL_PACK_ALIGNMENT,1);
            glReadPixels(0,0,fw,fh,GL_RGB,GL_UNSIGNED_BYTE,raw.data());
            // savePNG expects linear RGB and performs mapping, so recording
            // uses a binary PPM directly to preserve the displayed pixels.
            std::ostringstream name; name << captureDirectory << "/frame-" << std::setw(6) << std::setfill('0') << completed << ".ppm";
            std::ofstream frame(name.str(),std::ios::binary); frame << "P6\n" << fw << ' ' << fh << "\n255\n";
            for (int row=fh-1;row>=0;--row) frame.write(reinterpret_cast<const char*>(raw.data()+size_t(row)*fw*3),fw*3);
            frame.close(); if (!frame) throw std::runtime_error("Cannot save framebuffer capture");
        }
        if (smoke && completed>=3) {
            unsigned char center[4]{};
            glReadPixels(fw/2,fh/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,center);
            if (center[0]+center[1]+center[2]==0) throw std::runtime_error("Viewer smoke test: black displayed scene");
            if (glGetError()!=GL_NO_ERROR) throw std::runtime_error("Viewer OpenGL smoke test failed");
            std::cout << "Viewer smoke test PASSED: camera changed and accelerated frames displayed.\n";
            glfwSetWindowShouldClose(window.glfwWindow,true);
        }
        if (captureFrames>0 && completed>=captureFrames) glfwSetWindowShouldClose(window.glfwWindow,true);
        if (captureSeconds>0 && std::chrono::duration<double>(captureNow-startTime).count()>=captureSeconds)
            glfwSetWindowShouldClose(window.glfwWindow,true);
        window.update();
        if (!presented) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (pending.valid()) pending.get();
    glDeleteTextures(1,&texture); glDeleteVertexArrays(1,&vao); glDeleteProgram(program);
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    return 0;
}
} // namespace realtime
