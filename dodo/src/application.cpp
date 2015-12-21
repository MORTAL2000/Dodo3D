#include <application.h>

#include <log.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

using namespace Dodo;

Application::Application(const char* title, u32 width, u32 height, u32 glMajorVersion, u32 glMinorVersion)
:mWindowSize(width, height),
 mRenderManager(),
 mDisplay(0),
 mWindow(0),
 mContext(0),
 mTimeDelta(0),
 mTimePrev(0)
{
  CreateXGLWindow(title, width, height, glMajorVersion, glMinorVersion);
}

Application::~Application()
{
  if( mDisplay )
  {
    if( mContext )
    {
      glXDestroyContext(mDisplay, mContext);
    }

    if( mWindow)
    {
      XDestroyWindow(mDisplay, mWindow);
    }

    XCloseDisplay(mDisplay);


  }
}

bool Application::CreateXGLWindow(const char* title, u32 width, u32 height, u32 glMajorVersion, u32 glMinorVersion)
{
  mDisplay = XOpenDisplay(NULL);
  if( !mDisplay )
  {
    return false;
  }

  static int visual_attribs[] =
  {
   GLX_X_RENDERABLE    , True,
   GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
   GLX_RENDER_TYPE     , GLX_RGBA_BIT,
   GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
   GLX_RED_SIZE        , 8,
   GLX_GREEN_SIZE      , 8,
   GLX_BLUE_SIZE       , 8,
   GLX_ALPHA_SIZE      , 8,
   GLX_DEPTH_SIZE      , 24,
   GLX_STENCIL_SIZE    , 8,
   GLX_DOUBLEBUFFER    , True,
   GLX_SAMPLE_BUFFERS  , 1,
   GLX_SAMPLES         , 4,
   None
  };

  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(mDisplay, DefaultScreen(mDisplay), visual_attribs, &fbcount);
  if (!fbc)
  {
    DODO_LOG( "Failed to retrieve a framebuffer config" );
    return false;
  }
  DODO_LOG( "Found %i matching FB configurations",fbcount );

  //Pick FB config with more samples per pixel
  int bestFBConfig(0);
  int maxSamples(0);
  for (int i=0; i<fbcount; ++i)
  {
    XVisualInfo *vi = glXGetVisualFromFBConfig( mDisplay, fbc[i] );
    if ( vi )
    {
      int samp_buf, samples;
      glXGetFBConfigAttrib( mDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
      glXGetFBConfigAttrib( mDisplay, fbc[i], GLX_SAMPLES       , &samples  );

      DODO_LOG( "%i) Visual ID: %u. Sample Buffers: %i. Samples: %i", i, (unsigned int)vi->visualid, samp_buf, samples );

      if( samp_buf && samples > maxSamples )
      {
        bestFBConfig = i;
        maxSamples = samples;
      }
    }
    XFree( vi );
  }

  GLXFBConfig fbConfig = fbc[ bestFBConfig ];

  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree( fbc );

  // Get a visual
  XVisualInfo *vi = glXGetVisualFromFBConfig( mDisplay, fbConfig );

  DODO_LOG( "Chosen Visual ID: %u",(unsigned int)vi->visualid );

  Window root = DefaultRootWindow(mDisplay);
  XSetWindowAttributes windowAttributes;
  windowAttributes.colormap = XCreateColormap(mDisplay,root,vi->visual, AllocNone );
  windowAttributes.background_pixmap = None ;
  windowAttributes.event_mask = ExposureMask      |
      KeyPressMask      |
      KeyReleaseMask    |
      ButtonPressMask   |
      ButtonReleaseMask |
      PointerMotionMask;


  mWindow = XCreateWindow( mDisplay, root,
                           0, 0, width, height,
                           0, vi->depth, InputOutput,vi->visual,
                           CWColormap|CWEventMask,
                           &windowAttributes );
  if ( !mWindow )
  {
    DODO_LOG("Failed to create window");
    return false;
  }

  XStoreName( mDisplay, mWindow, title);
  XMapWindow( mDisplay, mWindow );

  //Create gl context
  mContext = glXCreateContext(mDisplay, vi, NULL, GL_TRUE);
  XFree( vi );

  if ( !mContext )
  {
    DODO_LOG("Failed to create GL context");
    return false;
  }

  //Make context current
  glXMakeCurrent( mDisplay, mWindow, mContext );

  //Capture destroy window event (will be received as a ClientMessage )
  Atom wmDelete=XInternAtom(mDisplay, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(mDisplay, mWindow, &wmDelete, 1);

  //Init render manager
  mRenderManager.Init();
  glGetError();
  mRenderManager.PrintInfo();

  return true;
}

void Application::MainLoop()
{
  Init();

  XEvent event;

  while (true)
  {
    while(XPending(mDisplay))
    {
      XNextEvent(mDisplay, &event);
      switch (event.type)
      {
        case KeyPress:
        {
          XKeyPressedEvent* keyPressEvent = (XKeyPressedEvent*)&event;
          OnKey( KeyFromKeyCode(keyPressEvent->keycode), true );
          break;
        }
        case KeyRelease:
        {
          XKeyReleasedEvent* keyPressEvent = (XKeyReleasedEvent*)&event;
          OnKey( KeyFromKeyCode(keyPressEvent->keycode), false );
          break;
        }
        case MotionNotify:
        {
          XMotionEvent* motionEvent = (XMotionEvent*)&event;
          OnMouseMove(motionEvent->x_root,  motionEvent->y_root );

          break;
        }
        case ButtonPress:
        {
          XButtonEvent* buttonEvent = (XButtonEvent*)&event;
          OnMouseButton(MOUSE_LEFT, buttonEvent->x_root,  buttonEvent->y_root, true );
          break;
        }
        case ButtonRelease:
        {
          XButtonEvent* buttonEvent = (XButtonEvent*)&event;
          OnMouseButton(MOUSE_LEFT, buttonEvent->x_root,  buttonEvent->y_root, false );
          break;
        }
        case ClientMessage:
        {
          //Exit
          return;
        }
        case Expose:
        {
          XWindowAttributes wa;
          XGetWindowAttributes(mDisplay, mWindow, &wa);
          mWindowSize = uvec2(wa.width, wa.height);
          OnResize(wa.width, wa.height);
          break;
        }
        default:
          break;
      }
    }



    Render();
    glXSwapBuffers ( mDisplay, mWindow );

    if( mTimePrev == 0.0f )
    {
      timespec time;
      clock_gettime( CLOCK_MONOTONIC, &time );
      mTimePrev = ( time.tv_sec * 1e3 ) + ( time.tv_nsec / 1e6 ) ;
    }

    timespec time;
    clock_gettime( CLOCK_MONOTONIC, &time );
    f32 currentTime = ( time.tv_sec * 1e3 ) + ( time.tv_nsec / 1e6 ) ;

    mTimeDelta = currentTime - mTimePrev;
    mTimePrev = currentTime;
  }
}

void Application::Render()
{
}

void Application::Init()
{
}

void Application::OnResize( size_t width, size_t height )
{
}

f32 Application::GetTimeDelta()
{
  return mTimeDelta;
}

void Application::OnKey( Key key, bool pressed )
{}

void Application::OnMouseMove( f32 x, f32 y )
{}

void Application::OnMouseButton( MouseButton button, f32 x, f32 y, bool pressed )
{}

Key Application::KeyFromKeyCode( unsigned int keycode )
{
  switch( keycode )
  {
    case 111:
      return KEY_UP;
    case 116:
      return KEY_DOWN;
    case 114:
      return KEY_RIGHT;
    case 113:
      return KEY_LEFT;
    case 25:
      return KEY_W;
    case 38:
      return KEY_A;
    case 39:
      return KEY_S;
    case 40:
      return KEY_D;
    case 52:
      return KEY_Z;
    case 53:
      return KEY_X;

  }

  DODO_LOG("Unknow key %u", keycode );
  return KEY_COUNT;
}
