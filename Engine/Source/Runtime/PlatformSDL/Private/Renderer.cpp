#include "Lemuria/Platform/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include <SDL3/SDL.h>

#include "VideoSystem.hpp"
#include "WindowImpl.hpp"

namespace lemuria::platform {

namespace {

#include "Shaders/ShapeFragmentSpirv.inc"
#include "Shaders/ShapeVertexSpirv.inc"
#if defined(_WIN32)
#include "Shaders/ShapeFragmentDxil.inc"
#include "Shaders/ShapeVertexDxil.inc"
#endif

template <auto Release>
struct GpuDeleter {
    SDL_GPUDevice* device = nullptr;

    template <typename Resource>
    void operator()(Resource* resource) const noexcept {
        Release(device, resource);
    }
};

struct GpuDeviceDeleter {
    void operator()(SDL_GPUDevice* device) const noexcept { SDL_DestroyGPUDevice(device); }
};

using GpuBuffer = std::unique_ptr<SDL_GPUBuffer, GpuDeleter<&SDL_ReleaseGPUBuffer>>;
using GpuTransferBuffer = std::unique_ptr<SDL_GPUTransferBuffer, GpuDeleter<&SDL_ReleaseGPUTransferBuffer>>;
using GpuTexture = std::unique_ptr<SDL_GPUTexture, GpuDeleter<&SDL_ReleaseGPUTexture>>;
using GpuShader = std::unique_ptr<SDL_GPUShader, GpuDeleter<&SDL_ReleaseGPUShader>>;
using GpuPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline, GpuDeleter<&SDL_ReleaseGPUGraphicsPipeline>>;

// One corner of the shared mesh, with the brightness of the face it belongs to.
struct MeshVertex {
    std::array<float, 3> corner;
    float shade;
};

constexpr float kTopShade = 1.0F;
constexpr float kSideXShade = 0.74F;
constexpr float kSideZShade = 0.58F;

// A box with its base centered on the origin and height 1. The top face comes first, so a ground tile is
// drawn with only the first six indices.
constexpr std::array<MeshVertex, 20> kMeshVertices{{
    {{-0.5F, 1.0F, -0.5F}, kTopShade},   {{-0.5F, 1.0F, 0.5F}, kTopShade},
    {{0.5F, 1.0F, 0.5F}, kTopShade},     {{0.5F, 1.0F, -0.5F}, kTopShade},
    {{0.5F, 0.0F, 0.5F}, kSideXShade},   {{0.5F, 0.0F, -0.5F}, kSideXShade},
    {{0.5F, 1.0F, -0.5F}, kSideXShade},  {{0.5F, 1.0F, 0.5F}, kSideXShade},
    {{-0.5F, 0.0F, -0.5F}, kSideXShade}, {{-0.5F, 0.0F, 0.5F}, kSideXShade},
    {{-0.5F, 1.0F, 0.5F}, kSideXShade},  {{-0.5F, 1.0F, -0.5F}, kSideXShade},
    {{-0.5F, 0.0F, 0.5F}, kSideZShade},  {{0.5F, 0.0F, 0.5F}, kSideZShade},
    {{0.5F, 1.0F, 0.5F}, kSideZShade},   {{-0.5F, 1.0F, 0.5F}, kSideZShade},
    {{0.5F, 0.0F, -0.5F}, kSideZShade},  {{-0.5F, 0.0F, -0.5F}, kSideZShade},
    {{-0.5F, 1.0F, -0.5F}, kSideZShade}, {{0.5F, 1.0F, -0.5F}, kSideZShade},
}};

constexpr std::array<std::uint16_t, 30> kMeshIndices{
    0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19,
};

constexpr Uint32 kTileIndexCount = 6;
constexpr Uint32 kBoxIndexCount = 30;
constexpr Uint32 kBytesPerPixel = 4;
constexpr SDL_GPUTextureFormat kOffscreenFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
constexpr SDL_GPUTextureFormat kDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

// The instances are copied to the GPU exactly as they are stored in the scene.
static_assert(std::is_trivially_copyable_v<core::ShapeInstance>);
static_assert(sizeof(core::ShapeInstance) == 40);
constexpr Uint32 kInstanceStride = sizeof(core::ShapeInstance);

[[nodiscard]] tl::unexpected<core::Error> sdl_error(const char* action) {
    return core::make_error(std::string(action) + ": " + SDL_GetError());
}

struct DrawTarget {
    SDL_GPUTexture* texture = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
};

} // namespace

// The members are released from the last to the first, so the device outlives everything created with it.
struct Renderer::Impl {
    Impl() = default;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    ~Impl() {
        if (device) {
            SDL_WaitForGPUIdle(device.get());
            if (window != nullptr) {
                SDL_ReleaseWindowFromGPUDevice(device.get(), window);
            }
        }
    }

    [[nodiscard]] core::Result<void> open_device(const RendererOptions& options);
    [[nodiscard]] core::Result<void> claim_window(SDL_Window* claimed_window, bool wait_for_display);
    [[nodiscard]] core::Result<void> create_offscreen_target(Uint32 width, Uint32 height);
    [[nodiscard]] core::Result<void> create_pipeline();
    [[nodiscard]] core::Result<void> create_buffers(const RendererOptions& options);
    [[nodiscard]] core::Result<void> upload_mesh();
    [[nodiscard]] core::Result<void> ensure_depth_texture(Uint32 width, Uint32 height);

    [[nodiscard]] GpuShader create_shader(SDL_GPUShaderStage stage, Uint32 uniform_buffers);
    [[nodiscard]] GpuBuffer create_buffer(SDL_GPUBufferUsageFlags usage, Uint32 size);
    [[nodiscard]] GpuTransferBuffer create_transfer_buffer(SDL_GPUTransferBufferUsage usage, Uint32 size);

    [[nodiscard]] bool upload_instances(SDL_GPUCommandBuffer* commands,
                                        std::span<const core::ShapeInstance> tiles,
                                        std::span<const core::ShapeInstance> boxes);
    [[nodiscard]] core::Result<DrawTarget> acquire_target(SDL_GPUCommandBuffer* commands);
    void draw_shapes(SDL_GPUCommandBuffer* commands, const DrawTarget& target,
                     const core::SceneDescription& scene, Uint32 tile_count, Uint32 box_count);

    VideoSystem video;
    std::unique_ptr<SDL_GPUDevice, GpuDeviceDeleter> device;
    SDL_Window* window = nullptr;
    SDL_GPUTextureFormat color_format = kOffscreenFormat;
    Uint32 instance_capacity = 0;

    GpuPipeline pipeline;
    GpuBuffer mesh_vertices;
    GpuBuffer mesh_indices;
    GpuBuffer instances;
    GpuTransferBuffer instance_upload;

    GpuTexture depth_texture;
    Uint32 depth_width = 0;
    Uint32 depth_height = 0;

    GpuTexture offscreen_target;
    GpuTransferBuffer image_download;
    Uint32 offscreen_width = 0;
    Uint32 offscreen_height = 0;

    std::uint64_t skipped_frames = 0;
};

core::Result<void> Renderer::Impl::open_device(const RendererOptions& options) {
    if (!video.ready()) {
        return sdl_error("could not start the video system");
    }
    SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV;
#if defined(_WIN32)
    formats |= SDL_GPU_SHADERFORMAT_DXIL;
#endif
    const char* api = options.graphics_api.empty() ? nullptr : options.graphics_api.c_str();
    device.reset(SDL_CreateGPUDevice(formats, false, api));
    if (!device) {
        return sdl_error("could not open the GPU");
    }
    return {};
}

core::Result<void> Renderer::Impl::claim_window(SDL_Window* claimed_window, bool wait_for_display) {
    if (!SDL_ClaimWindowForGPUDevice(device.get(), claimed_window)) {
        return sdl_error("could not draw on the window");
    }
    window = claimed_window;

    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    if (!wait_for_display) {
        for (const SDL_GPUPresentMode candidate :
             {SDL_GPU_PRESENTMODE_MAILBOX, SDL_GPU_PRESENTMODE_IMMEDIATE}) {
            if (SDL_WindowSupportsGPUPresentMode(device.get(), window, candidate)) {
                mode = candidate;
            }
        }
    }
    if (!SDL_SetGPUSwapchainParameters(device.get(), window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode)) {
        return sdl_error("could not configure the window");
    }
    color_format = SDL_GetGPUSwapchainTextureFormat(device.get(), window);
    return {};
}

core::Result<void> Renderer::Impl::create_offscreen_target(Uint32 width, Uint32 height) {
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = kOffscreenFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    offscreen_target = GpuTexture(SDL_CreateGPUTexture(device.get(), &info), {device.get()});
    if (!offscreen_target) {
        return sdl_error("could not create the image to draw on");
    }

    image_download =
        create_transfer_buffer(SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, width * height * kBytesPerPixel);
    if (!image_download) {
        return sdl_error("could not create the buffer to read the image");
    }
    offscreen_width = width;
    offscreen_height = height;
    return {};
}

GpuShader Renderer::Impl::create_shader(SDL_GPUShaderStage stage, Uint32 uniform_buffers) {
    const bool is_vertex = stage == SDL_GPU_SHADERSTAGE_VERTEX;
    SDL_GPUShaderCreateInfo info{};
    info.entrypoint = "main";
    info.stage = stage;
    info.num_uniform_buffers = uniform_buffers;
#if defined(_WIN32)
    if ((SDL_GetGPUShaderFormats(device.get()) & SDL_GPU_SHADERFORMAT_DXIL) != 0) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = is_vertex ? kShapeVertexDxil : kShapeFragmentDxil;
        info.code_size = is_vertex ? sizeof(kShapeVertexDxil) : sizeof(kShapeFragmentDxil);
        return GpuShader(SDL_CreateGPUShader(device.get(), &info), {device.get()});
    }
#endif
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.code = is_vertex ? kShapeVertexSpirv : kShapeFragmentSpirv;
    info.code_size = is_vertex ? sizeof(kShapeVertexSpirv) : sizeof(kShapeFragmentSpirv);
    return GpuShader(SDL_CreateGPUShader(device.get(), &info), {device.get()});
}

core::Result<void> Renderer::Impl::create_pipeline() {
    const GpuShader vertex_shader = create_shader(SDL_GPU_SHADERSTAGE_VERTEX, 1);
    const GpuShader fragment_shader = create_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, 0);
    if (!vertex_shader || !fragment_shader) {
        return sdl_error("could not load the shaders");
    }

    const std::array<SDL_GPUVertexBufferDescription, 2> buffers{{
        {0, sizeof(MeshVertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
        {1, kInstanceStride, SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0},
    }};
    const std::array<SDL_GPUVertexAttribute, 5> attributes{{
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(MeshVertex, corner)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(MeshVertex, shade)},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(core::ShapeInstance, position)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(core::ShapeInstance, size)},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(core::ShapeInstance, color)},
    }};

    SDL_GPUColorTargetDescription color_target{};
    color_target.format = color_format;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertex_shader.get();
    info.fragment_shader = fragment_shader.get();
    info.vertex_input_state.vertex_buffer_descriptions = buffers.data();
    info.vertex_input_state.num_vertex_buffers = static_cast<Uint32>(buffers.size());
    info.vertex_input_state.vertex_attributes = attributes.data();
    info.vertex_input_state.num_vertex_attributes = static_cast<Uint32>(attributes.size());
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.target_info.color_target_descriptions = &color_target;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = kDepthFormat;
    info.target_info.has_depth_stencil_target = true;

    pipeline = GpuPipeline(SDL_CreateGPUGraphicsPipeline(device.get(), &info), {device.get()});
    if (!pipeline) {
        return sdl_error("could not prepare the drawing pipeline");
    }
    return {};
}

GpuBuffer Renderer::Impl::create_buffer(SDL_GPUBufferUsageFlags usage, Uint32 size) {
    SDL_GPUBufferCreateInfo info{};
    info.usage = usage;
    info.size = size;
    return GpuBuffer(SDL_CreateGPUBuffer(device.get(), &info), {device.get()});
}

GpuTransferBuffer Renderer::Impl::create_transfer_buffer(SDL_GPUTransferBufferUsage usage, Uint32 size) {
    SDL_GPUTransferBufferCreateInfo info{};
    info.usage = usage;
    info.size = size;
    return GpuTransferBuffer(SDL_CreateGPUTransferBuffer(device.get(), &info), {device.get()});
}

core::Result<void> Renderer::Impl::create_buffers(const RendererOptions& options) {
    instance_capacity = static_cast<Uint32>(options.max_ground_tiles + options.max_boxes);
    const Uint32 instance_bytes = instance_capacity * kInstanceStride;

    mesh_vertices = create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(kMeshVertices));
    mesh_indices = create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, sizeof(kMeshIndices));
    instances = create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, instance_bytes);
    instance_upload = create_transfer_buffer(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, instance_bytes);
    if (!mesh_vertices || !mesh_indices || !instances || !instance_upload) {
        return sdl_error("could not create the drawing buffers");
    }
    return upload_mesh();
}

core::Result<void> Renderer::Impl::upload_mesh() {
    const Uint32 total_bytes = sizeof(kMeshVertices) + sizeof(kMeshIndices);
    const GpuTransferBuffer upload = create_transfer_buffer(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, total_bytes);
    if (!upload) {
        return sdl_error("could not create the mesh upload buffer");
    }

    auto* mapped = static_cast<std::uint8_t*>(SDL_MapGPUTransferBuffer(device.get(), upload.get(), false));
    if (mapped == nullptr) {
        return sdl_error("could not write the mesh");
    }
    std::memcpy(mapped, kMeshVertices.data(), sizeof(kMeshVertices));
    std::memcpy(mapped + sizeof(kMeshVertices), kMeshIndices.data(), sizeof(kMeshIndices));
    SDL_UnmapGPUTransferBuffer(device.get(), upload.get());

    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device.get());
    if (commands == nullptr) {
        return sdl_error("could not start the mesh upload");
    }
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTransferBufferLocation vertex_source{upload.get(), 0};
    const SDL_GPUBufferRegion vertex_region{mesh_vertices.get(), 0, sizeof(kMeshVertices)};
    SDL_UploadToGPUBuffer(copy, &vertex_source, &vertex_region, false);
    const SDL_GPUTransferBufferLocation index_source{upload.get(), sizeof(kMeshVertices)};
    const SDL_GPUBufferRegion index_region{mesh_indices.get(), 0, sizeof(kMeshIndices)};
    SDL_UploadToGPUBuffer(copy, &index_source, &index_region, false);
    SDL_EndGPUCopyPass(copy);
    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return sdl_error("could not upload the mesh");
    }
    return {};
}

core::Result<void> Renderer::Impl::ensure_depth_texture(Uint32 width, Uint32 height) {
    if (depth_texture && depth_width == width && depth_height == height) {
        return {};
    }
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = kDepthFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    depth_texture = GpuTexture(SDL_CreateGPUTexture(device.get(), &info), {device.get()});
    if (!depth_texture) {
        return sdl_error("could not create the depth texture");
    }
    depth_width = width;
    depth_height = height;
    return {};
}

bool Renderer::Impl::upload_instances(SDL_GPUCommandBuffer* commands,
                                      std::span<const core::ShapeInstance> tiles,
                                      std::span<const core::ShapeInstance> boxes) {
    const std::size_t total_bytes = tiles.size_bytes() + boxes.size_bytes();
    if (total_bytes == 0) {
        return true;
    }
    auto* mapped =
        static_cast<std::uint8_t*>(SDL_MapGPUTransferBuffer(device.get(), instance_upload.get(), true));
    if (mapped == nullptr) {
        return false;
    }
    if (!tiles.empty()) {
        std::memcpy(mapped, tiles.data(), tiles.size_bytes());
    }
    if (!boxes.empty()) {
        std::memcpy(mapped + tiles.size_bytes(), boxes.data(), boxes.size_bytes());
    }
    SDL_UnmapGPUTransferBuffer(device.get(), instance_upload.get());

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTransferBufferLocation source{instance_upload.get(), 0};
    const SDL_GPUBufferRegion destination{instances.get(), 0, static_cast<Uint32>(total_bytes)};
    SDL_UploadToGPUBuffer(copy, &source, &destination, true);
    SDL_EndGPUCopyPass(copy);
    return true;
}

core::Result<DrawTarget> Renderer::Impl::acquire_target(SDL_GPUCommandBuffer* commands) {
    if (window == nullptr) {
        return DrawTarget{offscreen_target.get(), offscreen_width, offscreen_height};
    }
    DrawTarget target;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window, &target.texture, &target.width,
                                               &target.height)) {
        return sdl_error("could not get the window image");
    }
    return target;
}

void Renderer::Impl::draw_shapes(SDL_GPUCommandBuffer* commands, const DrawTarget& target,
                                 const core::SceneDescription& scene, Uint32 tile_count, Uint32 box_count) {
    SDL_GPUColorTargetInfo color{};
    color.texture = target.texture;
    color.clear_color = {scene.background.r, scene.background.g, scene.background.b, scene.background.a};
    color.load_op = SDL_GPU_LOADOP_CLEAR;
    color.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = depth_texture.get();
    depth.clear_depth = 1.0F;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth.cycle = true;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &color, 1, &depth);
    SDL_BindGPUGraphicsPipeline(pass, pipeline.get());
    SDL_PushGPUVertexUniformData(commands, 0, scene.view_projection.values.data(),
                                 static_cast<Uint32>(sizeof(scene.view_projection.values)));

    const SDL_GPUBufferBinding index_binding{mesh_indices.get(), 0};
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    const auto draw_batch = [&](Uint32 first_instance, Uint32 count, Uint32 index_count) {
        if (count == 0) {
            return;
        }
        const std::array<SDL_GPUBufferBinding, 2> bindings{{
            {mesh_vertices.get(), 0},
            {instances.get(), first_instance * kInstanceStride},
        }};
        SDL_BindGPUVertexBuffers(pass, 0, bindings.data(), static_cast<Uint32>(bindings.size()));
        SDL_DrawGPUIndexedPrimitives(pass, index_count, count, 0, 0, 0);
    };
    draw_batch(0, tile_count, kTileIndexCount);
    draw_batch(tile_count, box_count, kBoxIndexCount);

    SDL_EndGPURenderPass(pass);
}

Renderer::Renderer(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
Renderer::Renderer(Renderer&& other) noexcept = default;
Renderer& Renderer::operator=(Renderer&& other) noexcept = default;
Renderer::~Renderer() = default;

core::Result<Renderer> Renderer::for_window(Window& window, const RendererOptions& options) {
    auto impl = std::make_unique<Impl>();
    auto ready = impl->open_device(options)
                     .and_then([&] {
                         return impl->claim_window(window.impl_->window.get(), options.wait_for_display);
                     })
                     .and_then([&] { return impl->create_pipeline(); })
                     .and_then([&] { return impl->create_buffers(options); });
    if (!ready) {
        return tl::unexpected(std::move(ready.error()));
    }
    return Renderer(std::move(impl));
}

core::Result<Renderer> Renderer::offscreen(std::uint32_t width, std::uint32_t height,
                                           const RendererOptions& options) {
    if (width == 0 || height == 0) {
        return core::make_error("an image to draw on needs a width and a height");
    }
    auto impl = std::make_unique<Impl>();
    auto ready = impl->open_device(options)
                     .and_then([&] { return impl->create_offscreen_target(width, height); })
                     .and_then([&] { return impl->create_pipeline(); })
                     .and_then([&] { return impl->create_buffers(options); });
    if (!ready) {
        return tl::unexpected(std::move(ready.error()));
    }
    return Renderer(std::move(impl));
}

core::Result<void> Renderer::draw(const core::SceneDescription& scene) {
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(impl_->device.get());
    if (commands == nullptr) {
        return sdl_error("could not start the frame");
    }

    auto tiles = scene.ground_tiles.shapes();
    auto boxes = scene.boxes.shapes();
    tiles = tiles.first(std::min<std::size_t>(tiles.size(), impl_->instance_capacity));
    boxes = boxes.first(std::min<std::size_t>(boxes.size(), impl_->instance_capacity - tiles.size()));

    if (!impl_->upload_instances(commands, tiles, boxes)) {
        SDL_CancelGPUCommandBuffer(commands);
        return sdl_error("could not send the shapes to the GPU");
    }

    const auto target = impl_->acquire_target(commands);
    if (!target) {
        SDL_CancelGPUCommandBuffer(commands);
        return tl::unexpected(target.error());
    }

    // A minimized window has no image to draw on, and the frame is simply skipped.
    core::Result<void> outcome;
    if (target->texture != nullptr) {
        outcome = impl_->ensure_depth_texture(target->width, target->height);
        if (outcome) {
            impl_->draw_shapes(commands, *target, scene, static_cast<Uint32>(tiles.size()),
                               static_cast<Uint32>(boxes.size()));
        }
    } else {
        ++impl_->skipped_frames;
    }

    if (!SDL_SubmitGPUCommandBuffer(commands)) {
        return sdl_error("could not finish the frame");
    }
    return outcome;
}

core::Result<Image> Renderer::read_image() {
    if (!impl_->offscreen_target) {
        return core::make_error("only an offscreen renderer can read its image");
    }
    SDL_GPUDevice* device = impl_->device.get();
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands == nullptr) {
        return sdl_error("could not start reading the image");
    }

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTextureRegion source{};
    source.texture = impl_->offscreen_target.get();
    source.w = impl_->offscreen_width;
    source.h = impl_->offscreen_height;
    source.d = 1;
    SDL_GPUTextureTransferInfo destination{};
    destination.transfer_buffer = impl_->image_download.get();
    destination.pixels_per_row = impl_->offscreen_width;
    destination.rows_per_layer = impl_->offscreen_height;
    SDL_DownloadFromGPUTexture(copy, &source, &destination);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (fence == nullptr) {
        return sdl_error("could not read the image");
    }
    const bool finished = SDL_WaitForGPUFences(device, true, &fence, 1);
    SDL_ReleaseGPUFence(device, fence);
    if (!finished) {
        return sdl_error("could not wait for the image");
    }

    Image image;
    image.width = impl_->offscreen_width;
    image.height = impl_->offscreen_height;
    image.pixels.resize(std::size_t{image.width} * image.height * kBytesPerPixel);
    const void* mapped = SDL_MapGPUTransferBuffer(device, impl_->image_download.get(), false);
    if (mapped == nullptr) {
        return sdl_error("could not open the image");
    }
    std::memcpy(image.pixels.data(), mapped, image.pixels.size());
    SDL_UnmapGPUTransferBuffer(device, impl_->image_download.get());
    return image;
}

std::uint64_t Renderer::skipped_frame_count() const noexcept {
    return impl_->skipped_frames;
}

std::string_view Renderer::graphics_api() const noexcept {
    return SDL_GetGPUDeviceDriver(impl_->device.get());
}

} // namespace lemuria::platform
