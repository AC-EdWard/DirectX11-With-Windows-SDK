Texture2D g_TexA : register(t0);
Texture2D g_TexB : register(t1);

RWTexture2D<float4> g_Output : register(u0);

// һ���߳����е��߳���Ŀ���߳̿���1άչ����Ҳ����
// 2ά��3ά�Ų�
[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    g_Output[DTid.xy] = g_TexA[DTid.xy] * g_TexB[DTid.xy];
}
