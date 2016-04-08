#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// テクスチャ
layout (location = 3) uniform sampler2D color;		// カラーのテクスチャ

// ラスタライザから受け取る頂点属性の補間値
in vec2 texcoord;                                   // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

void main(void)
{
  // テクスチャをそのまま表示する
  fc = texture(color, texcoord);
}
