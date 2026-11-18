// Draws a batch of boxes or flat tiles. Each instance brings its own position, size and color.

[[vk::binding(0, 1)]] cbuffer Camera : register(b0, space1) {
    float4x4 view_projection;
};

struct Input {
    [[vk::location(0)]] float3 corner : TEXCOORD0;
    [[vk::location(1)]] float shade : TEXCOORD1;
    [[vk::location(2)]] float3 instance_position : TEXCOORD2;
    [[vk::location(3)]] float3 instance_size : TEXCOORD3;
    [[vk::location(4)]] float4 instance_color : TEXCOORD4;
};

struct Output {
    [[vk::location(0)]] float4 color : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input) {
    const float3 world_position = input.instance_position + input.corner * input.instance_size;

    Output output;
    output.position = mul(view_projection, float4(world_position, 1.0));
    output.color = float4(input.instance_color.rgb * input.shade, input.instance_color.a);
    return output;
}
