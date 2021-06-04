#include <array>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using i32 = int32_t;

template <typename T>
std::vector<T> LoadBin(const std::filesystem::path &path) {
    std::basic_ifstream<T> file{path, std::ios::binary};
    return {std::istreambuf_iterator<T>{file}, {}};
}

template <typename T>
T Read(char *buf, size_t offset) {
    return *reinterpret_cast<T *>(&buf[offset]);
};

int convertImage(std::filesystem::path path) {
    if (!std::filesystem::exists(path)) {
        std::cout << std::format("{} does not exist\n", path.string());
        return ENOENT;
    }
    if (!std::filesystem::is_regular_file(path)) {
        std::cout << std::format("{} is not a file\n", path.string());
        return EINVAL;
    }

    auto bin = LoadBin<char>(path);
    auto data = bin.data();

    static constexpr u32 kMagic = 0x43535344; // DSSC
    static constexpr u32 kMaxVersion = 1;

    auto magic = Read<u32>(data, 0);
    if (magic != kMagic) {
        std::cout << std::format("{} is not a valid 3dtst screen capture file\n", path.string());
        return EINVAL;
    }

    auto version = Read<u16>(data, 4);
    if (version > kMaxVersion) {
        std::cout << std::format("{}: unsupported version {}\n", path.string(), version);
        return EINVAL;
    }

    if (bin.size() < 256 * 192 * 2 + 0x40) {
        std::cout << std::format("{}: file truncated\n", path.string());
        return EINVAL;
    }

    auto params = Read<u32>(data, 8);
    bool antiAlias = (params >> 0) & 1;
    bool edgeMark = (params >> 1) & 1;
    bool wireframe = (params >> 2) & 1;
    bool quad = (params >> 3) & 1;
    u8 texMode = (params >> 4) & 7;
    u8 alpha = (params >> 8) & 0x1F;

    i32 verts[4][3];
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 3; j++) {
            verts[i][j] = Read<i32>(data, 0x10 + (j + i * 3) * 4);
        }
    }

    auto fmtToggle = [](bool on) { return on ? "On" : "Off"; };
    auto fmtShape = [](bool quad) { return quad ? "Quad" : "Triangle"; };
    auto fmtTexture = [](u8 texMode) -> std::string {
        switch (texMode) {
        case 0: return "Vertical stripes";
        case 1: return "Horizontal stripes";
        case 2: return "Vertical+Horizontal stripes";
        case 3: return "Coordinate grid (RG = ST)";
        case 4: return "Colored vertices";
        case 5: return "Plain white";
        default: return std::format("Invalid value: {:d}", texMode);
        }
    };

    auto printParams = [&](std::ostream &out, std::string indent) {
        out << std::format("{}Antialiasing: {}\n", indent, fmtToggle(antiAlias));
        out << std::format("{}Edge marking: {}\n", indent, fmtToggle(edgeMark));
        out << std::format("{}Wireframe   : {}\n", indent, fmtToggle(wireframe));
        out << std::format("{}Alpha level : {}\n", indent, alpha);
        out << std::format("{}Shape       : {}\n", indent, fmtShape(quad));
        out << std::format("{}Texture     : {}\n", indent, fmtTexture(texMode));
        for (size_t i = 0; i < 4; i++) {
            out << std::format("{}Vertex {}    : {} x {} x {}\n", indent, i + 1, verts[i][0], verts[i][1], verts[i][2]);
        }
    };

    std::cout << std::format("{}: file read successfully\n", path.string());

    std::cout << "\nParameters:\n";
    printParams(std::cout, "  ");

    u8 tga[18];
    std::fill_n(tga, 18, 0);
    tga[2] = 2;                               // uncompressed truecolor
    *reinterpret_cast<u16 *>(&tga[12]) = 256; // width
    *reinterpret_cast<u16 *>(&tga[14]) = 192; // height
    tga[16] = 24;                             // bits per pixel
    tga[17] = 32;                             // image descriptor: top to bottom, left to right

    auto paramsPath = path.replace_extension("txt");
    std::cout << std::format("Writing parameters to {}... ", paramsPath.string());
    {
        std::ofstream out{paramsPath, std::ios::binary | std::ios::trunc};
        printParams(out, "");
    }
    std::cout << "Done\n";

    auto imagePath = path.replace_extension("tga");
    std::cout << std::format("Writing image to {}... ", imagePath.string());
    {
        std::ofstream out{imagePath, std::ios::binary | std::ios::trunc};
        out.write((char *)tga, sizeof(tga));
        for (size_t y = 0; y < 192; y++) {
            for (size_t x = 0; x < 256; x++) {
                auto color = Read<u16>(data, 0x40 + (x + y * 256) * 2);
                u8 r5 = (color >> 0) & 0x1F;
                u8 g5 = (color >> 5) & 0x1F;
                u8 b5 = (color >> 10) & 0x1F;
                auto expand = [](u8 c5) { return (c5 << 3) | (c5 >> 2); };
                u8 r8 = expand(r5);
                u8 g8 = expand(g5);
                u8 b8 = expand(b5);
                out.write((char *)&b8, 1);
                out.write((char *)&g8, 1);
                out.write((char *)&r8, 1);
            }
        }
    }
    std::cout << "Done\n";
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << std::format("usage: {} filename\n", argv[0]);
        return EINVAL;
    }

    int exitcode = 0;
    for (int i = 1; i < argc; i++) {
        int result = convertImage(argv[i]);
        if (result != 0) exitcode = result;
        std::cout << "\n";
    }

    return exitcode;
}
