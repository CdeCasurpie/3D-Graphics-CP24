#include "Filters.h"
#include "../SimplePathTracer/Scene.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace reconstruction;
using path_tracer::add;
using path_tracer::mul;
using Images = std::array<std::vector<Vec>,Count>;
constexpr const char* labels[]{"BOX","TRIANGLE","GAUSSIAN","MITCHELL-NETRAVALI"};
constexpr const char* filenames[]{"box.png","triangle.png","gaussian.png","mitchell.png"};

template<class Work>
void rows(int begin,int end,unsigned threads,Work work) {
    std::atomic<int> next{begin};
    auto run=[&] {
        for (int y=next.fetch_add(1);y<end;y=next.fetch_add(1)) work(y);
    };
    std::vector<std::thread> pool;
    try {
        for (unsigned i=1;i<std::min(threads,unsigned(end-begin));++i) pool.emplace_back(run);
    } catch (...) {
        for (auto& t:pool) t.join();
        throw;
    }
    run();
    for (auto& t:pool) t.join();
}

Images render(int w,int h,int spp,uint32_t seed,unsigned threads) {
    const auto scene=path_tracer::makeScene();
    const auto view=scene.view();
    path_tracer::Settings settings;
    settings.width=w; settings.height=h; settings.bounces=4;
    Film film(w,h,spp);
    rows(-halo,h+halo,threads,[&](int y) {
        for (int x=-halo;x<w+halo;++x) {
            auto* samples=film.cell(x,y);
            const uint32_t index=uint32_t(y+halo)*uint32_t(w+2*halo)+uint32_t(x+halo);
            for (int i=0;i<spp;++i) {
                using path_tracer::hash;
                path_tracer::Random rng{hash(seed^hash(index)^hash(uint32_t(i)+0xa511e9b3u))};
                const float px=x+rng.next(),py=y+rng.next();
                const auto& c=view.camera;
                const float sx=(2*px/w-1)*(float(w)/h)*c.tanHalfFov;
                const float sy=(1-2*py/h)*c.tanHalfFov;
                const path_tracer::Ray ray{c.eye,path_tracer::normalize(add(c.forward,
                    add(mul(c.right,sx),mul(c.up,sy))))};
                samples[i]={px,py,path_tracer::trace(ray,view,settings,rng)};
            }
        }
    });
    Images images;
    for (auto& i:images) i.resize(size_t(w)*h);
    rows(0,h,threads,[&](int y) {
        for (int x=0;x<w;++x) {
            const auto pixels=reconstruct(film,x,y);
            for (int f=0;f<Count;++f) images[f][size_t(y)*w+x]=pixels[f];
        }
    });
    return images;
}

struct Canvas {
    int w,h;
    std::vector<unsigned char> data;
    Canvas(int width,int height): w(width),h(height),data(size_t(w)*h*3,22) {}
    void pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b) {
        if (x<0 || y<0 || x>=w || y>=h) return;
        auto* p=data.data()+3*(size_t(y)*w+x);
        p[0]=r; p[1]=g; p[2]=b;
    }
    void blit(const std::vector<Vec>& image,int stride,int x,int y,int sx,int sy,int width,int height,int scale,double exposure) {
        auto encode=[&](float linear) {
            if (!std::isfinite(linear)) throw std::runtime_error("Nonfinite radiance");
            const double exposed=std::max(0.0,double(linear))*exposure;
            const double t=exposed/(1+exposed);
            return static_cast<unsigned char>(std::lround(255*(t<=.0031308?12.92*t:1.055*std::pow(t,1/2.4)-.055)));
        };
        for (int j=0;j<height;++j) for (int i=0;i<width;++i) {
            const auto c=image[size_t(sy+j)*stride+sx+i];
            const auto r=encode(c.x),g=encode(c.y),b=encode(c.z);
            for (int v=0;v<scale;++v) for (int u=0;u<scale;++u) pixel(x+i*scale+u,y+j*scale+v,r,g,b);
        }
    }
    void text(int x,int y,const char* text,int scale) {
        constexpr const char* keys="ABCEGHILMNOPRSTUVX-";
        constexpr const char* glyphs[]={
            "010101111101101","110101110101110","011100100100011","111100110100111",
            "011100101101011","101101111101101","111010010010111","100100100100111",
            "101111111101101","101111111111101","111101101101111","110101110100100","110101110101101",
            "011100010001110","111010010010010","101101101101111","101101101101010",
            "101101010101101","000000111000000"};
        for (;*text;++text,x+=4*scale) {
            const std::string alphabet=keys;
            const auto k=alphabet.find(*text);
            if (k==std::string::npos) continue;
            for (int j=0;j<5;++j) for (int i=0;i<3;++i) if (glyphs[k][3*j+i]=='1')
                for (int v=0;v<scale;++v) for (int u=0;u<scale;++u)
                    pixel(x+i*scale+u,y+j*scale+v,225,235,250);
        }
    }
    void save(const std::filesystem::path& path) const {
        if (!stbi_write_png(path.string().c_str(),w,h,3,data.data(),w*3))
            throw std::runtime_error("Cannot save "+path.string());
    }
};

void exportImages(const Images& images,int w,int h,double exposure,const std::filesystem::path& out) {
    std::filesystem::create_directories(out);
    const int cropW=std::min(w,32),cropH=std::min(h,24);
    const int cropX=std::clamp(int(w*.63)-cropW/2,0,w-cropW);
    const int cropY=std::clamp(int(h*.14)-cropH/2,0,h-cropH);
    const int panelW=std::max(360,2*w+4*cropW+36),panelH=std::max(2*h,4*cropH)+62;
    Canvas comparison(2*panelW,2*panelH);
    for (int f=0;f<Count;++f) {
        Canvas single(w,h);
        single.blit(images[f],w,0,0,0,0,w,h,1,exposure);
        single.save(out/filenames[f]);
        const int x=f%2*panelW+12,y=f/2*panelH+12;
        comparison.text(x,y,labels[f],3);
        comparison.blit(images[f],w,x,y+26,0,0,w,h,2,exposure);
        comparison.blit(images[f],w,x+2*w+12,y+26,cropX,cropY,cropW,cropH,4,exposure);
        comparison.text(x+2*w+12,y+30+4*cropH,"CROP",2);
    }
    comparison.save(out/"comparison.png");
    std::cout<<"Saved "<<out/"comparison.png"<< " and four native-resolution PNGs\n";
}

void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
void tests() {
    require(weight(Box,0)==1 && weight(Box,.5)==0 && weight(Triangle,.25)==.75 &&
            weight(Gaussian,2)==0 && weight(Gaussian,0)>weight(Gaussian,1) &&
            std::abs(weight(Mitchell,0)-8.0/9)<1e-12 &&
            std::abs(weight(Mitchell,1)-1.0/18)<1e-12 &&
            weight(Mitchell,1.5)<0,"Filter values");
    for (int f=0;f<Count;++f) {
        const auto filter=static_cast<Filter>(f);
        require(weight(filter,-.37)==weight(filter,.37) && weight(filter,radii[f])==0,"Symmetry/support");
    }
    Film film(3,2,1);
    for (auto& s:film.data) s.rgb={.25f,.5f,.75f};
    for (int y=-halo;y<film.height+halo;++y) for (int x=-halo;x<film.width+halo;++x) {
        auto& p=*film.cell(x,y); p.x=x+.5f; p.y=y+.5f;
    }
    for (int x=0;x<film.width;++x) for (auto c:reconstruct(film,x,0))
        require(c.x==.25f && c.y==.5f && c.z==.75f,"Constant preservation at border");
    for (int y=-halo;y<film.height+halo;++y) for (int x=-halo;x<film.width+halo;++x)
        film.cell(x,y)->rgb={float(x>=1),0,0};
    const auto edge=reconstruct(film,0,0);
    require(edge[Box].x==0 && edge[Gaussian].x>edge[Mitchell].x && edge[Mitchell].x>0,"Cross-pixel edge weights");
    const auto a=render(9,7,4,42,1), b=render(9,7,4,42,3);
    for (int f=0;f<Count;++f) for (size_t i=0;i<a[f].size();++i)
        require(a[f][i].x==b[f][i].x && a[f][i].y==b[f][i].y && a[f][i].z==b[f][i].z,"Deterministic threads");
    std::cout<<"Reconstruction filter tests passed.\n";
}

int main(int argc,char** argv) try {
    int w=160,h=120,spp=128;
    unsigned threads=std::clamp(std::thread::hardware_concurrency(),1u,16u);
    uint32_t seed=42;
    double exposure=1;
    std::filesystem::path out="src/ReconstructionFilters/output";
    for (int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        if (arg=="--self-test") { tests(); return 0; }
        if (arg=="--help") {
            std::cout<<"ReconstructionFilters [--width 160 --height 120 --samples 128 --seed 42 "
                        "--threads N --exposure 1 --output-dir DIR | --self-test]\n";
            return 0;
        }
        require(i+1<argc,"Missing option value");
        const std::string value=argv[++i];
        auto integer=[&](long long low,long long high) {
            size_t parsed=0;
            const auto n=std::stoll(value,&parsed);
            require(parsed==value.size() && n>=low && n<=high,"Numeric option out of range");
            return n;
        };
        if (arg=="--width") w=int(integer(1,2048));
        else if (arg=="--height") h=int(integer(1,2048));
        else if (arg=="--samples") spp=int(integer(1,4096));
        else if (arg=="--threads") threads=unsigned(integer(1,256));
        else if (arg=="--seed") seed=uint32_t(integer(0,4294967295LL));
        else if (arg=="--output-dir") out=value;
        else if (arg=="--exposure") {
            size_t parsed=0;
            exposure=std::stod(value,&parsed);
            require(parsed==value.size() && std::isfinite(exposure) && exposure>0 && exposure<=100,"Invalid exposure");
        } else throw std::runtime_error("Unknown option: "+arg);
    }
    require(uint64_t(w+2*halo)*(h+2*halo)*spp<=16000000,"Sample buffer exceeds limit; reduce dimensions or samples");
    exportImages(render(w,h,spp,seed,threads),w,h,exposure,out);
    return 0;
} catch (const std::exception& e) {
    std::cerr<<"ReconstructionFilters: "<<e.what()<<'\n';
    return 1;
}
