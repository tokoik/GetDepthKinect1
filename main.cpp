//
// Kinect (v2) のデプスマップ取得
//

// 標準ライブラリ
#include <Windows.h>

// OpenCV
#include <opencv2/highgui/highgui.hpp>
#ifdef _WIN32
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

// ウィンドウ関連の処理
#include "Window.h"

// センサ関連の処理
#include "KinectV2.h"

// 描画に用いるメッシュ
#include "Mesh.h"

// 計算に用いるシェーダ
#include "Calculate.h"

// 頂点位置の生成をシェーダ (position.frag) で行うなら 1
#define GENERATE_POSITION 0

//
// メインプログラム
//
int main()
{
  // OpenCV によるビデオキャプチャを初期化する
  cv::VideoCapture camera(0);
  if (!camera.isOpened())
  {
    // ビデオカメラが取得できない
    MessageBox(NULL, TEXT("ビデオカメラが見つかりません。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // カメラの初期設定
  camera.grab();
  const GLsizei capture_env_width(GLsizei(camera.get(CV_CAP_PROP_FRAME_WIDTH)));
  const GLsizei capture_env_height(GLsizei(camera.get(CV_CAP_PROP_FRAME_HEIGHT)));

  // GLFW を初期化する
  if (glfwInit() == GL_FALSE)
  {
    // GLFW の初期化に失敗した
    MessageBox(NULL, TEXT("GLFW の初期化に失敗しました。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // プログラム終了時には GLFW を終了する
  atexit(glfwTerminate);

  // OpenGL Version 3.2 Core Profile を選択する
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // ウィンドウを開く
  Window window(640, 480, "Depth Map Viewer");
  if (!window.get())
  {
    // ウィンドウが作成できなかった
    MessageBox(NULL, TEXT("GLFW のウィンドウが開けませんでした。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // 深度センサを有効にする
  KinectV2 sensor;
  if (sensor.getActivated() == 0)
  {
    // センサが使えなかった
    MessageBox(NULL, TEXT("深度センサを有効にできませんでした。"), TEXT("すまんのう"), MB_OK);
    return EXIT_FAILURE;
  }

  // 深度センサの解像度
  int width, height;
  sensor.getDepthResolution(&width, &height);

  // 描画に使うメッシュ
  const Mesh mesh(width, height, sensor.getCoordBuffer());

  // 背景に使う矩形
  const Rect rect;

  // 描画用のシェーダ
  GgSimpleShader simple("refraction.vert", "refraction.frag");
  const GLint sizeLoc(glGetUniformLocation(simple.get(), "size"));

  // テクスチャをそのまま表示するシェーダ
  const GLuint backimage(ggLoadShader("rectangle.vert", "rectangle.frag"));

  // デプスデータから頂点位置を計算するシェーダ
  const Calculate position(width, height, "position.frag");

  // バイラテラルフィルタのシェーダ
  const Calculate bilateral(width, height, "bilateral.frag");

  // 頂点位置から法線ベクトルを計算するシェーダ
  const Calculate normal(width, height, "normal.frag");

  // 背景画像
  GLuint bmap;
  glGenTextures(1, &bmap);
  glBindTexture(GL_TEXTURE_2D, bmap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, capture_env_width, capture_env_height, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // 背景色を設定する
  glClearColor(background[0], background[1], background[2], background[3]);

  // 隠面消去処理を有効にする
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // ウィンドウが開いている間くり返し描画する
  while (!window.shouldClose())
  {
#if GENERATE_POSITION
    // 頂点位置の計算
    position.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    sensor.getDepth();
    const std::vector<GLuint> &positionTexture(position.calculate());
#endif

    // バイラテラルフィルタ
    bilateral.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
#if GENERATE_POSITION
    glBindTexture(GL_TEXTURE_2D, positionTexture[0]);
#else
    sensor.getPoint();
#endif
    const std::vector<GLuint> &bilateralTexture(bilateral.calculate());

    // 法線ベクトルの計算
    normal.use();
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bilateralTexture[0]);
    const std::vector<GLuint> &normalTexture(normal.calculate());

    // 画像のキャプチャ
    if (camera.grab())
    {
      // キャプチャ映像から画像を切り出す
      cv::Mat frame;
      camera.retrieve(frame, 3);

      // 切り出した画像をテクスチャに転送する
      cv::Mat flipped;
      cv::flip(frame, flipped, 0);
      glBindTexture(GL_TEXTURE_2D, bmap);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, flipped.rows, GL_BGR, GL_UNSIGNED_BYTE, flipped.data);
    }

    // 画面消去
    window.clear();

    // 背景描画
    glUseProgram(backimage);
    glUniform1i(3, 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, bmap);
    glDepthMask(GL_FALSE);
    rect.draw();
    glDepthMask(GL_TRUE);

    // 描画用のシェーダプログラムの使用開始
    simple.use();
    simple.loadMatrix(window.getMp(), window.getMw());
    simple.setLight(light);
    simple.setMaterial(material);
    glUniform2f(sizeLoc, GLfloat(window.getSize()[0]), GLfloat(window.getSize()[1]));

    // 頂点位置テクスチャ
    glUniform1i(0, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bilateralTexture[0]);

    // 頂点法線テクスチャ
    glUniform1i(1, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture[0]);

    // 背景テクスチャ
    glUniform1i(3, 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, bmap);

    // 図形描画
    mesh.draw();

    // バッファを入れ替える
    window.swapBuffers();
  }
}
