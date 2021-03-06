#pragma once

//
// 深度センサ関連の処理
//

// Kinect 関連
#include <Windows.h>
#include <NuiApi.h>

// ウィンドウ関連の処理
#include "Window.h"

// 深度センサ関連の基底クラス
#include "DepthCamera.h"

// デプス画像のサイズ
#define DEPTH_W 320
#define DEPTH_H 240

// カラー画像のサイズ
#define COLOR_W 640
#define COLOR_H 480

// NUI_IMAGE_RESOLUTION の設定
#define EXPAND_RESOLUTION(width, height) NUI_IMAGE_RESOLUTION_##width##x##height
#define RESOLUTION(width, height) EXPAND_RESOLUTION(width, height)
#define DEPTH_RESOLUTION RESOLUTION(DEPTH_W, DEPTH_H)
#define COLOR_RESOLUTION RESOLUTION(COLOR_W, COLOR_H)

class KinectV1 : public DepthCamera
{
  // センサの識別子
  INuiSensor *sensor;

  // デプスデータのストリームハンドル
  HANDLE depthStream;

  // デプスデータのイベントハンドル
  const HANDLE nextDepthFrameEvent;

  // デプスデータからカメラ座標を求めるときに用いる一時メモリ
  GLfloat (*position)[3];

  // カラーデータのストリームハンドル
  HANDLE colorStream;

  // カラーデータのイベントハンドル
  const HANDLE nextColorFrameEvent;

  // データを取得する
  void getImage(HANDLE event, HANDLE stream,
    GLuint texture, GLsizei width, GLsizei height, GLenum format, GLenum type) const;

  // コピーコンストラクタ (コピー禁止)
  KinectV1(const KinectV1 &w);

  // 代入 (代入禁止)
  KinectV1 &operator=(const KinectV1 &w);

public:

  // コンストラクタ
  KinectV1();

  // デストラクタ
  virtual ~KinectV1();

  // デプスデータを取得する
  GLuint getDepth() const
  {
    getImage(nextDepthFrameEvent, depthStream, depthTexture,
      depthWidth, depthHeight, GL_RED, GL_UNSIGNED_SHORT);
    return depthTexture;
  }

  // カメラ座標を取得する
  GLuint getPoint() const
  {
    getImage(nextDepthFrameEvent, depthStream, pointTexture,
      depthWidth, depthHeight, GL_RED, GL_UNSIGNED_SHORT);
    return pointTexture;
  }

  // カラーデータを取得する
  GLuint getColor() const
  {
    getImage(nextColorFrameEvent, colorStream, colorTexture,
      colorWidth, colorHeight, GL_BGRA, GL_UNSIGNED_BYTE);
    return colorTexture;
  }
};
