#include "Window.h"
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Samplers.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* names[] = {"Halton", "Sobol", "MT19937"};
constexpr ImVec4 colors[] = {{0.32f,0.81f,1,1},{0.48f,0.96f,0.68f,1},{1,0.71f,0.36f,1}};

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
void tests() {
    using namespace sampling;
    const Point expected[] = {{0,0,0},{.5,.5,.5},{.75,.25,.25},{.25,.75,.75},
                             {.375,.375,.625},{.875,.875,.125},{.625,.125,.875},{.125,.625,.375}};
    for (unsigned i=0; i<8; ++i) require(sobol(i)==expected[i], "Sobol reference prefix");
    require(halton(1)==Point{.5,1.0/3,1.0/5} && halton(2)==Point{.25,2.0/3,2.0/5},
            "Halton reference prefix");
    require(std::abs(halton(5)[2]-.04)<1e-15, "Halton base-5 carry");
    Cloud net;
    for (unsigned i=0; i<512; ++i) net.push_back(sobol(i));
    require(occupancy(net).empty==0, "Sobol 512-point dyadic grid coverage");
    for (unsigned n : {0u,1u,512u,2048u,16384u}) {
        const auto a=generate(n,42), b=generate(n,43);
        require(a==generate(n,42), "Same-seed reproducibility");
        require(a[0]==b[0] && a[1]==b[1] && (!n || a[2]!=b[2]), "Seed only affects random baseline");
        for (const auto& cloud : a) {
            require(cloud.size()==n, "Point count");
            for (const auto& p : cloud) for (double x : p)
                require(x>=0 && x<1, "Coordinate outside unit cube");
        }
    }
    const auto prefix=generate(512,42), longer=generate(2048,42);
    for (int i=0;i<3;++i)
        require(std::equal(prefix[i].begin(),prefix[i].end(),longer[i].begin()),"Stable sample prefix");
    std::cout << "Advanced Samplers self-tests passed.\n";
}

// ImGui's draw list displays CPU-projected points and box edges with the same
// camera/point settings in every panel. No depth test: projections show overlap.
void panel(ImDrawList* draw, ImVec2 lo, ImVec2 hi, const sampling::Cloud& cloud,
           int index, const Camera& camera, int view, bool slice, float sliceAt,
           float thickness, float radius, float opacity, uint32_t seed) {
    draw->AddRectFilled(lo,hi,IM_COL32(17,27,40,255),10);
    draw->AddRect(lo,hi,IM_COL32(49,67,86,255),10);
    auto color=colors[index];
    draw->AddText({lo.x+16,lo.y+15},ImGui::ColorConvertFloat4ToU32(color),names[index]);
    const std::string subtitle=std::to_string(cloud.size())+" points  /  [0,1)^3"+
        (index==2 ? "  /  seed "+std::to_string(seed) : "");
    draw->AddText({lo.x+16,lo.y+37},IM_COL32(169,186,205,255),subtitle.c_str());
    const auto stats=sampling::occupancy(cloud);
    char text[128];
    std::snprintf(text,sizeof(text),"8^3 grid   empty %d   min/max %d/%d",stats.empty,stats.minimum,stats.maximum);
    draw->AddText({lo.x+16,hi.y-47},IM_COL32(192,206,220,255),text);
    int a=view==3?1:0,b=view==2||view==3?2:1;
    const auto projection=sampling::occupancy(cloud,a,b,-1);
    if (view)
        std::snprintf(text,sizeof(text),"3D RMS %.2f   2D RMS %.2f",stats.rms,projection.rms);
    else std::snprintf(text,sizeof(text),"3D occupancy RMS %.2f",stats.rms);
    draw->AddText({lo.x+16,hi.y-26},IM_COL32(151,172,193,255),text);
    const ImVec2 top{lo.x+10,lo.y+63}, bottom{hi.x-10,hi.y-60};
    const float size=std::min(bottom.x-top.x,bottom.y-top.y);
    if (size<=0) return;
    const ImVec2 center{(top.x+bottom.x)*.5f,(top.y+bottom.y)*.5f};
    const glm::mat4 vp=camera.getProjectionMatrix(1,1)*camera.getViewMatrix();
    auto project=[&](const sampling::Point& p, ImVec2& pixel) {
        if (view) {
            const float scale=size*.83f*3.7f/camera.distance;
            pixel={center.x+scale*(float(p[a])-.5f),center.y-scale*(float(p[b])-.5f)};
            return true;
        }
        const glm::vec4 q=vp*glm::vec4(p[0]-.5,p[1]-.5,p[2]-.5,1);
        if (q.w<=0) return false;
        pixel={center.x+size*.79f*q.x/q.w,center.y-size*.79f*q.y/q.w};
        return true;
    };
    draw->PushClipRect(top,bottom,true);
    for (int i=0;i<8;++i) for (int axis=0;axis<3;++axis) if (!(i&(1<<axis))) {
        if (view && axis!=a && axis!=b) continue;
        if (view && (i&(1<<(3-a-b)))) continue;
        sampling::Point p{double(i&1),double((i>>1)&1),double((i>>2)&1)},q=p;
        q[axis]=1;
        ImVec2 x,y;
        if (project(p,x) && project(q,y)) draw->AddLine(x,y,IM_COL32(95,120,145,255));
    }
    color.w=opacity;
    const auto packed=ImGui::ColorConvertFloat4ToU32(color);
    const int sliceAxis=view ? 3-a-b : 2;
    for (const auto& p:cloud) {
        if (slice && std::abs(p[sliceAxis]-sliceAt)>thickness*.5) continue;
        ImVec2 pixel;
        if (project(p,pixel)) draw->AddCircleFilled(pixel,radius,packed,6);
    }
    draw->PopClipRect();
}

void save(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(size_t(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    const size_t stride=size_t(width)*3;
    for (int y=0;y<height/2;++y)
        std::swap_ranges(pixels.begin()+y*stride,pixels.begin()+(y+1)*stride,
                         pixels.begin()+(height-1-y)*stride);
    const auto parent=std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    require(glGetError()==GL_NO_ERROR,"Framebuffer read failed");
    require(stbi_write_png(path.c_str(),width,height,3,pixels.data(),int(stride))!=0,"PNG write failed");
    std::cout << "Saved " << path << '\n';
}

int viewer(const std::string& capture, int view, int countIndex) {
    Window window(1280,720,"Task 17 - Advanced Samplers");
    auto* handle=window.glfwWindow;
    glfwSetWindowSizeLimits(handle,1280,720,1280,720);
    glfwSetWindowAspectRatio(handle,16,9);
    glfwSetWindowSize(handle,1280,720);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename=nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(handle,true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    Camera camera(3.7f);
    const glm::quat pose(glm::vec3(.38f,-.52f,0));
    camera.orientation=pose;
    camera.minDistance=1.4f; camera.maxDistance=10;
    constexpr unsigned counts[]={512,2048,8192,16384};
    constexpr const char* views[]={"Orbit","XY","XZ","YZ"};
    uint32_t seed=42;
    auto clouds=sampling::generate(counts[countIndex],seed);
    int selected=0,frame=0;
    bool slice=false,clean=false;
    float radius=1.4f, opacity=.64f,sliceAt=.5f,thickness=.15f;
    while (!window.shouldClose()) {
        window.processInput();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        auto& io=ImGui::GetIO();
        if (!io.WantCaptureMouse && io.DisplaySize.x>0 && io.DisplaySize.y>0) {
            const auto x=io.MousePos.x,y=io.MousePos.y;
            const int w=int(io.DisplaySize.x),h=int(io.DisplaySize.y);
            if (!view && ImGui::IsMouseClicked(0)) camera.startDragging(x,y,w,h);
            if (view || !ImGui::IsMouseDown(0)) camera.stopDragging();
            camera.onMouseDrag(x,y,w,h);
            if (!view && ImGui::IsMouseClicked(1)) camera.startPanning(x,y);
            if (view || !ImGui::IsMouseDown(1)) camera.stopPanning();
            camera.onMousePan(x,y,w,h);
            if (io.MouseWheel) camera.onScroll(io.MouseWheel);
        } else { camera.stopDragging(); camera.stopPanning(); }
        if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Tab)) clean=!clean;
        const bool screenshot=(!capture.empty() && frame>=3) ||
            (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F12));
        auto* draw=ImGui::GetBackgroundDrawList();
        draw->AddText(ImGui::GetFont(),26,{24,20},IM_COL32(235,244,255,255),"Task 17  /  Advanced Samplers");
        const std::string details=std::string(views[view])+" view   /   "+(slice?"Thin slice":"Full unit cube")+
            "   /   Same camera, point count and point style";
        draw->AddText({26,57},IM_COL32(157,181,206,255),details.c_str());
        const int columns=selected?1:3;
        const float width=(io.DisplaySize.x-48-16*(columns-1))/columns;
        const float bottom=io.DisplaySize.y-(clean||screenshot||!capture.empty()?26:140);
        if (width>0) for (int i=0;i<columns;++i) {
            const int algorithm=selected?selected-1:i;
            const float x=24+i*(width+16);
            panel(draw,{x,85},{x+width,bottom},clouds[algorithm],algorithm,camera,view,slice,
                  sliceAt,thickness,radius,opacity,seed);
        }
        if (!clean && !screenshot && capture.empty()) {
            ImGui::SetNextWindowPos({24,bottom+10});
            ImGui::SetNextWindowSize({io.DisplaySize.x-48,112});
            ImGui::Begin("Comparison controls",nullptr,ImGuiWindowFlags_NoDecoration);
            ImGui::PushItemWidth(105);
            bool rebuild=ImGui::Combo("Points",&countIndex,"512\0002048\0008192\00016384\000");
            ImGui::SameLine(); ImGui::Combo("View",&view,"Orbit\000XY\000XZ\000YZ\000");
            ImGui::SameLine(); ImGui::Combo("Cloud",&selected,"All\000Halton\000Sobol\000MT19937\000");
            ImGui::SameLine(); rebuild |= ImGui::InputScalar("Seed",ImGuiDataType_U32,&seed);
            ImGui::SameLine(); if (ImGui::Button("Reset camera")) { camera=Camera(3.7f); camera.orientation=pose; camera.minDistance=1.4f; camera.maxDistance=10; }
            ImGui::SliderFloat("Point radius",&radius,.7f,4);
            ImGui::SameLine(); ImGui::SliderFloat("Opacity",&opacity,.1f,1);
            ImGui::SameLine(); ImGui::Checkbox("Slice",&slice);
            ImGui::SameLine(); ImGui::SliderFloat("Center",&sliceAt,0,1);
            ImGui::SameLine(); ImGui::SliderFloat("Thickness",&thickness,.01f,1);
            ImGui::TextUnformatted("Left drag: orbit   Right drag: pan   Scroll: zoom   Tab: controls   F12: clean PNG");
            ImGui::PopItemWidth();
            ImGui::End();
            if (rebuild) clouds=sampling::generate(counts[countIndex],seed);
        }
        ImGui::Render();
        int w,h; glfwGetFramebufferSize(handle,&w,&h);
        glViewport(0,0,w,h);
        glClearColor(.032f,.055f,.085f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (screenshot && w>0 && h>0) {
            save(capture.empty()?"src/AdvancedSamplers/comparison.png":capture,w,h);
            if (!capture.empty()) glfwSetWindowShouldClose(handle,true);
        }
        ++frame;
        window.update();
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    return 0;
}
}

int main(int argc,char** argv) {
    try {
        std::string capture;
        int view=0,count=1;
        for (int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if (arg=="--self-test") { tests(); return 0; }
            if (arg=="--stats") {
                const auto clouds=sampling::generate(2048,42);
                for (int j=0;j<3;++j) {
                    const auto s=sampling::occupancy(clouds[j]);
                    std::cout<<names[j]<<": empty="<<s.empty<<" min="<<s.minimum<<" max="<<s.maximum<<" RMS="<<s.rms<<'\n';
                }
                return 0;
            }
            if (arg=="--capture" && i+1<argc) capture=argv[++i];
            else if (arg=="--view" && i+1<argc) {
                const std::string v=argv[++i];
                view=v=="orbit"?0:v=="xy"?1:v=="xz"?2:v=="yz"?3:-1;
                require(view>=0,"Expected orbit, xy, xz, or yz");
            } else if (arg=="--count" && i+1<argc) {
                const std::string n=argv[++i];
                count=n=="512"?0:n=="2048"?1:n=="8192"?2:n=="16384"?3:-1;
                require(count>=0,"Expected 512, 2048, 8192, or 16384");
            } else throw std::runtime_error("Usage: AdvancedSamplers [--self-test | --stats | [--capture PNG] [--view orbit|xy|xz|yz] [--count N]]");
        }
        return viewer(capture,view,count);
    } catch (const std::exception& e) {
        std::cerr<<"AdvancedSamplers: "<<e.what()<<'\n';
        return 1;
    }
}
