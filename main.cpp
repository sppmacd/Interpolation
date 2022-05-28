#include <LLGL/Core/VectorUtils.hpp>
#include <LLGL/OpenGL/Shaders/Basic330Core.hpp>
#include <LLGL/OpenGL/Shaders/ShadeFlat.hpp>
#include <LLGL/OpenGL/Utils.hpp>
#include <LLGL/OpenGL/Vertex.hpp>
#include <LLGL/Renderer/Transform.hpp>
#include <LLGL/Window/Window.hpp>

#include <iostream>
#include <vector>

constexpr int WorldSize = 1024;
constexpr float NoiseScale = 32;
constexpr float NoiseDetailScale = 4;
constexpr float NoiseDetail2Scale = 2;

constexpr float DisplaySizeX = 64;
constexpr float DisplaySizeY = 2;

auto interpolate(float fac, auto a, auto b)
{
    return (b - a) * (3.0 - fac * 2.0) * fac * fac + a;
}

template<size_t RawSize>
class Noise
{
public:
    Noise()
    {
        for (int x = 0; x < RawSize; x++)
        {
            for (int y = 0; y < RawSize; y++)
            {
                raw_noise({ x, y }) = rand() / static_cast<double>(RAND_MAX);
            }
        }
    }

    float noise(llgl::Vector2f p)
    {
        auto noise_pos = p;
        auto fx = noise_pos.x - (int)noise_pos.x;
        auto fy = noise_pos.y - (int)noise_pos.y;
        auto n00 = raw_noise({ (int)noise_pos.x, (int)noise_pos.y });
        auto n01 = raw_noise({ (int)noise_pos.x, (int)noise_pos.y + 1 });
        auto n10 = raw_noise({ (int)noise_pos.x + 1, (int)noise_pos.y });
        auto n11 = raw_noise({ (int)noise_pos.x + 1, (int)noise_pos.y + 1 });
        float i0 = interpolate(fy, n00, n01);
        float i1 = interpolate(fy, n10, n11);
        return interpolate(fx, i0, i1);
    }

private:
    float& raw_noise(llgl::Vector2i p) { return m_noise[(p.x % RawSize) + RawSize * (p.y % RawSize)]; }

    std::array<float, RawSize * RawSize> m_noise;
};

class World
{
public:
    float& block(llgl::Vector2i p) { return m_blocks[p.x + WorldSize * p.y]; }

    void generate()
    {
        m_blocks.resize(WorldSize * WorldSize);
        for (int x = 0; x < WorldSize; x++)
        {
            for (int y = 0; y < WorldSize; y++)
            {
                float value = m_noise.noise({ x / NoiseScale, y / NoiseScale });
                value += m_noise_detail.noise({ x / NoiseDetailScale, y / NoiseDetailScale }) * 0.1;
                value += m_noise_detail2.noise({ x / NoiseDetail2Scale, y / NoiseDetail2Scale }) * 0.05;
                block({ x, y }) = value;
            }
        }
    }

private:
    Noise<128> m_noise;
    Noise<16> m_noise_detail;
    Noise<16> m_noise_detail2;
    std::vector<float> m_blocks;
};

int main()
{
    llgl::Window window({ 500, 500 }, u8"test");

    World world;
    world.generate();

    std::vector<llgl::Vertex> vertexes;
    for (int x = 0; x < WorldSize - 1; x++)
    {
        for (int y = 0; y < WorldSize - 1; y++)
        {
            auto block00 = world.block({ x, y });
            auto block01 = world.block({ x, y + 1 });
            auto block10 = world.block({ x + 1, y });
            auto block11 = world.block({ x + 1, y + 1 });

            auto position = [&](float block, int ox, int oy)
            {
                return llgl::Vector3f {
                    (static_cast<float>(x + ox) / WorldSize - 0.5f) * DisplaySizeX,
                    block * DisplaySizeY,
                    (static_cast<float>(y + oy) / WorldSize - 0.5f) * DisplaySizeX
                };
            };
            auto color = [&](float block)
            {
                return interpolate(block, llgl::Color { 132, 140, 132 }, llgl::Color { 200, 200, 230 });
            };
            auto plane_normal = [&](float a, float b, float c)
            {
                float diff1 = (a - b) / 2;
                float diff2 = (a + b) / 2 - c;
            };

            float diag1 = block10 - block01;
            float diag2 = block00 - block11;
            vertexes.push_back(llgl::Vertex {
                .position = position(block00, 0, 0),
                .color = color(block00),
                .normal = llgl::Vector3f { 1, diag2, 1 } });
            vertexes.push_back(llgl::Vertex {
                .position = position(block10, 1, 0),
                .color = color(block10),
                .normal = llgl::Vector3f { 1, diag2, 1 } });
            vertexes.push_back(llgl::Vertex {
                .position = position(block01, 0, 1),
                .color = color(block01),
                .normal = llgl::Vector3f { 1, diag2, 1 } });
            vertexes.push_back(llgl::Vertex {
                .position = position(block10, 1, 0),
                .color = color(block10),
                .normal = llgl::Vector3f { 1, diag1, 1 } });
            vertexes.push_back(llgl::Vertex {
                .position = position(block01, 0, 1),
                .color = color(block01),
                .normal = llgl::Vector3f { 1, diag1, 1 } });
            vertexes.push_back(llgl::Vertex {
                .position = position(block11, 1, 1),
                .color = color(block11),
                .normal = llgl::Vector3f { 1, diag1, 1 } });
        }
    }

    auto shader = llgl::opengl::shaders::ShadeFlat();
    shader.set_light_color(llgl::Color { 255, 255, 230 });
    shader.set_light_position({ 0, 10, -5 });

    llgl::opengl::VAO vao;
    vao.load_vertexes(shader.attribute_mapping(), vertexes);

    auto& renderer = window.renderer();

    llgl::opengl::enable(llgl::opengl::Feature::DepthTest);

    float angle = 0;
    while (true)
    {
        llgl::Event event;
        while (window.poll_event(event))
        {
        }
        llgl::View view;
        view.set_viewport(llgl::Recti { 0, 0, window.size().x, window.size().y });
        view.set_perspective({ 70, window.aspect(), 0.5, 50 });

        llgl::Transform transform;
        transform = transform.translate({ 0, -2.5, -7 });
        transform = transform.rotate_y(angle);
        angle += 0.001;

        llgl::opengl::set_clear_color(llgl::Color { 150, 150, 255 });
        llgl::opengl::clear(llgl::opengl::ClearMask::Color | llgl::opengl::ClearMask::Depth);
        renderer.apply_view(view);
        renderer.draw_vao(vao, llgl::opengl::PrimitiveType::Triangles, llgl::DrawState { .shader = &shader, .model_matrix = transform.matrix() });
        window.display();
    }
}
