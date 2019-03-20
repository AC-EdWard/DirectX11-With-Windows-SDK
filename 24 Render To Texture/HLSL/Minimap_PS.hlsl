#include "Minimap.hlsli"

// ������ɫ��
float4 PS(VertexPosHTex pIn) : SV_Target
{
    // Ҫ��Tex��ȡֵ��Χ����[0.0f, 1.0f], yֵ��Ӧ��������z��
    float2 PosW = pIn.Tex * float2(g_RectW.zw - g_RectW.xy) + g_RectW.xy;
    
    float4 color = g_Tex.Sample(g_Sam, pIn.Tex);

    [flatten]
    if (g_FogEnabled && length(PosW - g_EyePosW.xz) / g_VisibleRange > 1.0f)
    {
        return g_InvisibleColor;
    }

    return color;
}