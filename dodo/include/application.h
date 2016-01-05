#pragma once

#include <types.h>
#include <GL/glx.h>
#include <render-manager.h>

namespace Dodo
{

enum Key
{
  KEY_UP = 0,
  KEY_DOWN = 1,
  KEY_LEFT = 2,
  KEY_RIGHT = 3,

  KEY_A = 'a',
  KEY_B = 'b',
  KEY_C = 'c',
  KEY_D = 'd',
  KEY_E = 'e',
  KEY_F = 'f',
  KEY_G = 'g',
  KEY_H = 'h',
  KEY_I = 'i',
  KEY_J = 'j',
  KEY_K = 'k',
  KEY_L = 'l',
  KEY_M = 'm',
  KEY_N = 'n',
  KEY_O = 'o',
  KEY_P = 'p',
  KEY_Q = 'q',
  KEY_R = 'r',
  KEY_S = 's',
  KEY_T = 't',
  KEY_U = 'u',
  KEY_V = 'v',
  KEY_W = 'w',
  KEY_X = 'x',
  KEY_Y = 'y',
  KEY_Z = 'z',

  KEY_COUNT
};

enum MouseButton
{
  MOUSE_LEFT = 0,
  MOUSE_RIGHT = 1
};

class Application
{

public:

  Application(const char* title, u32 width, u32 height, u32 glMajorVersion, u32 glMinorVersion);
  virtual ~Application();

  void MainLoop();
  virtual void Render();
  virtual void Init();
  virtual void OnResize( size_t width, size_t height );
  virtual void OnKey( Key key, bool pressed );
  virtual void OnMouseMove( f32 x, f32 y );
  virtual void OnMouseButton( MouseButton button, f32 x, f32 y, bool pressed );
  f32 GetTimeDelta();

  uvec2       mWindowSize;
  RenderManager mRenderManager;


private:

  bool CreateXGLWindow(const char* title, u32 width, u32 height, u32 glMajorVersion, u32 glMinorVersion);

  Key KeyFromKeyCode( unsigned int keycode );
  void CreateWindow( int width ,int height );

  Display*    mDisplay;
  Window      mWindow;
  GLXContext  mContext;
  f32         mTimeDelta; //milliseconds
  f32         mTimePrev;
};

}
