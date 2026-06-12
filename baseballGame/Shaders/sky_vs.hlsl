// sky_vs.hlsl
// Fullscreen pass vertex shader for sky rendering

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// Fullscreen triangle (no vertex buffer needed)
VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    // Generate a fullscreen triangle using SV_VertexID
    // vertexID: 0 -> (-1,-1), 1 -> (-1, 3), 2 -> (3, -1)
    float2 ndc = float2(
        (vertexID == 2) ?  3.0f : -1.0f,
        (vertexID == 1) ? -3.0f :  1.0f
    );

    output.position = float4(ndc, 0.999f, 1.0f); // depth = 0.999 (max-1, behind everything)
    output.texcoord = ndc * float2(0.5f, -0.5f) + 0.5f;

    return output;
}
