#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.16384
//
//
///
// Buffer Definitions: 
//
// cbuffer SwizzleProperties
// {
//
//   uint4 SwizzleIndices;              // Offset:    0 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// TextureI3D                        texture   sint4          3d    0        1
// SwizzleProperties                 cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float       
// SV_RENDERTARGETARRAYINDEX     0   x           1  RTINDEX    uint       
// TEXCOORD                 0   xyz         2     NONE   float   xyz 
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET     int   xyzw
//
ps_4_0
dcl_constantbuffer cb0[1], immediateIndexed
dcl_resource_texture3d (sint,sint,sint,sint) t0
dcl_input_ps linear v2.xyz
dcl_output o0.xyzw
dcl_temps 1
dcl_indexableTemp x0[6], 4
resinfo_uint r0.xyzw, l(0), t0.xyzw
utof r0.xyz, r0.xyzx
mul r0.xyz, r0.xyzx, v2.xyzx
ftoi r0.xyz, r0.xyzx
mov r0.w, l(0)
ld r0.xyzw, r0.xyzw, t0.xyzw
mov x0[0].x, r0.x
mov x0[1].x, r0.y
mov x0[2].x, r0.z
mov x0[3].x, r0.w
mov x0[4].x, l(0)
mov x0[5].x, l(1)
mov r0.x, cb0[0].x
mov o0.x, x0[r0.x + 0].x
mov r0.x, cb0[0].y
mov o0.y, x0[r0.x + 0].x
mov r0.x, cb0[0].z
mov o0.z, x0[r0.x + 0].x
mov r0.x, cb0[0].w
mov o0.w, x0[r0.x + 0].x
ret 
// Approximately 21 instruction slots used
#endif

const BYTE g_PS_SwizzleI3D[] =
{
     68,  88,  66,  67, 101,  41, 
    194, 229, 103, 175, 204, 102, 
    192,  43, 200, 161, 117, 114, 
     19, 252,   1,   0,   0,   0, 
    200,   4,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     60,   1,   0,   0, 196,   1, 
      0,   0, 248,   1,   0,   0, 
     76,   4,   0,   0,  82,  68, 
     69,  70,   0,   1,   0,   0, 
      1,   0,   0,   0, 124,   0, 
      0,   0,   2,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    255, 255,   0,   1,   0,   0, 
    204,   0,   0,   0,  92,   0, 
      0,   0,   2,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  13,   0,   0,   0, 
    103,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,  84, 101, 120, 116, 
    117, 114, 101,  73,  51,  68, 
      0,  83, 119, 105, 122, 122, 
    108, 101,  80, 114, 111, 112, 
    101, 114, 116, 105, 101, 115, 
      0, 171, 171, 171, 103,   0, 
      0,   0,   1,   0,   0,   0, 
    148,   0,   0,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 172,   0, 
      0,   0,   0,   0,   0,   0, 
     16,   0,   0,   0,   2,   0, 
      0,   0, 188,   0,   0,   0, 
      0,   0,   0,   0,  83, 119, 
    105, 122, 122, 108, 101,  73, 
    110, 100, 105,  99, 101, 115, 
      0, 171,   1,   0,  19,   0, 
      1,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  57, 
     46,  51,  48,  46,  57,  50, 
     48,  48,  46,  49,  54,  51, 
     56,  52,   0, 171,  73,  83, 
     71,  78, 128,   0,   0,   0, 
      3,   0,   0,   0,   8,   0, 
      0,   0,  80,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  92,   0,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 118,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   7,   7, 
      0,   0,  83,  86,  95,  80, 
     79,  83,  73,  84,  73,  79, 
     78,   0,  83,  86,  95,  82, 
     69,  78,  68,  69,  82,  84, 
     65,  82,  71,  69,  84,  65, 
     82,  82,  65,  89,  73,  78, 
     68,  69,  88,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  65,  82, 
     71,  69,  84,   0, 171, 171, 
     83,  72,  68,  82,  76,   2, 
      0,   0,  64,   0,   0,   0, 
    147,   0,   0,   0,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  88,  40,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  51,  51,   0,   0, 
     98,  16,   0,   3, 114,  16, 
     16,   0,   2,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   1,   0, 
      0,   0, 105,   0,   0,   4, 
      0,   0,   0,   0,   6,   0, 
      0,   0,   4,   0,   0,   0, 
     61,  16,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,  86,   0, 
      0,   5, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,   7, 114,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,  70,  18,  16,   0, 
      2,   0,   0,   0,  27,   0, 
      0,   5, 114,   0,  16,   0, 
      0,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  45,   0,   0,   7, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,  48, 
     32,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   6,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     54,   0,   0,   7,  18,  32, 
     16,   0,   0,   0,   0,   0, 
     10,  48,  32,   4,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   6,  18,   0,  16,   0, 
      0,   0,   0,   0,  26, 128, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   7,  34,  32,  16,   0, 
      0,   0,   0,   0,  10,  48, 
     32,   4,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  54,   0,   0,   6, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  42, 128,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  54,   0,   0,   7, 
     66,  32,  16,   0,   0,   0, 
      0,   0,  10,  48,  32,   4, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58, 128,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     54,   0,   0,   7, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     10,  48,  32,   4,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  62,   0, 
      0,   1,  83,  84,  65,  84, 
    116,   0,   0,   0,  21,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
     10,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
