// SpriteUI MVP shader. ABI must match SpriteInstanceGPU (80 bytes).
// The backend binds one instance buffer and draws a unit quad instanced per batch.

struct SpriteInstanceGPU
{
    float4 transformRow0; // m00, m01, tx, depth
    float4 transformRow1; // m10, m11, ty, reserved
    float4 uvRect;        // u, v, width, height
    float4 tint;          // linear premultiplied RGBA
    uint4  resourceIds;   // textureSlice, materialIndex, clipIndex, flags
};

cbuffer SpriteUIGlobals : register(b0)
{
    row_major float4x4 ViewProjection;
    uint BaseInstance;
    uint3 _GlobalPadding;
};

StructuredBuffer<SpriteInstanceGPU> SpriteInstances : register(t0);
Texture2DArray SpriteTextures : register(t1);
StructuredBuffer<float4> ClipRects : register(t2); // minX,minY,maxX,maxY in framebuffer pixels
SamplerState SpriteSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 tint : COLOR0;
    nointerpolation uint4 resourceIds : TEXCOORD1;
};

VSOutput VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    static const float2 Quad[6] = {
        float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0),
        float2(0.0, 1.0), float2(1.0, 0.0), float2(1.0, 1.0)
    };

    const SpriteInstanceGPU item = SpriteInstances[BaseInstance + instanceId];
    const float2 local = Quad[vertexId];

    const float2 world = float2(
        dot(item.transformRow0.xy, local) + item.transformRow0.z,
        dot(item.transformRow1.xy, local) + item.transformRow1.z
    );

    VSOutput output;
    output.position = mul(float4(world, item.transformRow0.w, 1.0), ViewProjection);
    output.uv = item.uvRect.xy + local * item.uvRect.zw;
    output.tint = item.tint;
    output.resourceIds = item.resourceIds;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    const float4 texel = SpriteTextures.Sample(
        SpriteSampler, float3(input.uv, input.resourceIds.x));

    // Both texel and tint are premultiplied. Component-wise multiplication
    // preserves Source-Over premultiplied-alpha semantics.
    const float4 color = texel * input.tint;

    const uint clipIndex = input.resourceIds.z;
    if (clipIndex != 0xFFFFFFFFu)
    {
        const float4 clipRect = ClipRects[clipIndex];
        const float2 pixel = input.position.xy;
        clip(min(pixel.x - clipRect.x, clipRect.z - pixel.x));
        clip(min(pixel.y - clipRect.y, clipRect.w - pixel.y));
    }

    return color;
}
