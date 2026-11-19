// Paints each shape with the color computed by the vertex shader.

struct Input {
    [[vk::location(0)]] float4 color : TEXCOORD0;
};

float4 main(Input input) : SV_Target0 {
    return input.color;
}
