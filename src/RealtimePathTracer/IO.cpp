#include "Backend.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace realtime {
void savePNG(const std::string& path,const std::vector<Vec>& pixels,int width,int height,float exposure) {
    if (pixels.size()!=size_t(width)*height) throw std::runtime_error("Invalid image dimensions");
    std::vector<unsigned char> bytes(pixels.size()*3);
    auto encode=[&](float value) {
        if (!std::isfinite(value) || value<0) throw std::runtime_error("Nonfinite/negative output radiance");
        const double c=double(value)*exposure;
        const double mapped=c/(1+c);
        const double s=mapped<=0.0031308?12.92*mapped:1.055*std::pow(mapped,1.0/2.4)-0.055;
        return static_cast<unsigned char>(std::lround(255*std::clamp(s,0.0,1.0)));
    };
    for (size_t i=0;i<pixels.size();++i) {
        bytes[3*i]=encode(pixels[i].x); bytes[3*i+1]=encode(pixels[i].y); bytes[3*i+2]=encode(pixels[i].z);
    }
    const auto parent=std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    if (!stbi_write_png(path.c_str(),width,height,3,bytes.data(),width*3))
        throw std::runtime_error("Cannot write PNG: "+path);
}
} // namespace realtime
