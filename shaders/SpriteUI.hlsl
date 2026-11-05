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
    float MsdfPxRange;   // Amplitude do MSDF em pixels do atlas (FontMetrics::pxRange).
    uint2 _GlobalPadding;
};

StructuredBuffer<SpriteInstanceGPU> SpriteInstances : register(t0);
Texture2DArray SpriteTextures : register(t1);
// minX,minY,maxX,maxY. O backend deve converter de pixels logicos (ClipRectGPU) para pixels
// do framebuffer (multiplicando pelo fator de DPI) ao preencher este buffer.
StructuredBuffer<float4> ClipRects : register(t2);
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

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 color = input.tint;

    const uint flags = input.resourceIds.w;
    if ((flags & 0x2u) != 0u) // Flag_MSDFText
    {
        const float3 msd = SpriteTextures.Sample(
            SpriteSampler, float3(input.uv, input.resourceIds.x)).rgb;
        const float sd = Median(msd.r, msd.g, msd.b) - 0.5;
        // Screen pixel distance derived from UV derivatives
        // pxRange vem do cbuffer (V4.2 sec. 4): amplitude em px / derivada real do UV.
        const float2 unitRange = float2(MsdfPxRange, MsdfPxRange) / max(fwidth(input.uv), float2(1e-5, 1e-5));
        const float screenPxDistance = sd * 0.5 * (unitRange.x + unitRange.y);
        const float textAlpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
        color *= textAlpha;
    }
    else
    {
        const float4 texel = SpriteTextures.Sample(
            SpriteSampler, float3(input.uv, input.resourceIds.x));
        color *= texel;
    }

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
