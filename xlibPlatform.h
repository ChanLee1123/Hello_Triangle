// Xlib linux platform dependent WSI handling and event loop

#ifndef COMMON_XLIB_WSI_H
#define COMMON_XLIB_WSI_H

#include <functional>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <vulkan/vulkan.h>

#include "CompilerMessages.h"


TODO( "Easier to use, but might prevent platform co-existence. Could be namespaced. Make all of this a class?" )
struct PlatformWindow{ Display* display; Window window; VisualID visual_id; };

std::string getPlatformSurfaceExtensionName(){ return VK_KHR_XLIB_SURFACE_EXTENSION_NAME; };

PlatformWindow initWindow( int canvasWidth, int canvasHeight );
void killWindow( PlatformWindow window );

bool platformPresentationSupport( VkInstance instance, VkPhysicalDevice device, uint32_t queueFamilyIndex, PlatformWindow window );

VkSurfaceKHR initSurface( VkInstance instance, PlatformWindow window );
// killSurface() is not platform dependent

void setSizeEventHandler( std::function<void(void)> newSizeEventHandler );
void setPaintEventHandler( std::function<void(void)> newPaintEventHandler );

void showWindow( PlatformWindow window );

int messageLoop( PlatformWindow window );

// Implementation
//////////////////////////////////

Display* initXlibDisplay(){
	Display* display = XOpenDisplay( nullptr );

	if( !display ) throw "Failed to create Xlib Display*.";

	return display;
}

void killXlibDisplay( Display* display ){
	XCloseDisplay( display );
}

PlatformWindow initWindow( int canvasWidth, int canvasHeight ){
	Display* display = initXlibDisplay();

	Screen* screen = XDefaultScreenOfDisplay( display );

	VisualID visual_id = XVisualIDFromVisual(  XDefaultVisualOfScreen( screen )  );

	Window root_window = XRootWindowOfScreen( screen );

	//auto black = XBlackPixelOfScreen( screen );
	//auto white = XWhitePixelOfScreen( screen );

	unsigned long masks =
		/*  CWBackPixmap
		| CWBackPixel
		| CWBorderPixmap
		| CWBorderPixel
		| CWBitGravity
		| CWWinGravity
		| CWBackingStore
		| CWBackingPlanes
		| CWBackingPixel
		| CWOverrideRedirect
		| CWSaveUnder
		|*/ CWEventMask
		/*| CWDontPropagate
		| CWColormap
		| CWCursor*/
	;

	XSetWindowAttributes values;
	values.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;


	Window window = XCreateWindow(
		display,
		root_window,
		0, 0, // x, y
		static_cast<unsigned int>( canvasWidth ), static_cast<unsigned int>( canvasHeight ),
		1, //border_width
		CopyFromParent,
		InputOutput,
		CopyFromParent,
		masks,
		&values
	); 

	const char* title = "Hello Vulkan Triangle";

	XStoreName( display, window, title );
	XSetIconName( display, window, title );

	Atom WM_DELETE_WINDOW = XInternAtom( display, "WM_DELETE_WINDOW", False );
	XSetWMProtocols( display, window, &WM_DELETE_WINDOW, 1 );

	XFlush( display );
	return { display, window, visual_id };
}

void killWindow( PlatformWindow window ){
	XDestroyWindow( window.display, window.window );
	XFlush( window.display );

	killXlibDisplay( window.display );
}

bool platformPresentationSupport( VkInstance instance, VkPhysicalDevice device, uint32_t queueFamilyIndex, PlatformWindow window ){
	return vkGetPhysicalDeviceXlibPresentationSupportKHR( device, queueFamilyIndex, window.display, window.visual_id ) == VK_TRUE;
}

VkSurfaceKHR initSurface( VkInstance instance, PlatformWindow window ){
	VkXlibSurfaceCreateInfoKHR surfaceInfo{
		VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		nullptr, // pNext for extensions use
		0, // flags - reserved for future use
		window.display,
		window.window
	};


	VkSurfaceKHR surface;
	VkResult errorCode = vkCreateXlibSurfaceKHR( instance, &surfaceInfo, nullptr, &surface ); RESULT_HANDLER( errorCode, "vkCreateXlibSurfaceKHR" );

	return surface;
}


void nullHandler(){}

std::function<void(void)> sizeEventHandler = nullHandler;

void setSizeEventHandler( std::function<void(void)> newSizeEventHandler ){
	if( !newSizeEventHandler ) sizeEventHandler = nullHandler;
	sizeEventHandler = newSizeEventHandler;
}

std::function<void(void)> paintEventHandler = nullHandler;

void setPaintEventHandler( std::function<void(void)> newPaintEventHandler ){
	if( !newPaintEventHandler ) paintEventHandler = nullHandler;
	paintEventHandler = newPaintEventHandler;
}

void showWindow( PlatformWindow window ){
	XMapWindow( window.display, window.window );
	XFlush( window.display );
}

int width = -1;
int height = -1;

int messageLoop( PlatformWindow window ){
	bool quit = false;

	while( !quit ){
		XEvent e;
		XNextEvent( window.display, &e );

		switch( e.type  ){
			case Expose:
				paintEventHandler();
				break;

			case ConfigureNotify:{
				XConfigureEvent ce = e.xconfigure;
				if( ce.width != width || ce.height != height ){
					width = ce.width;
					height = ce.height;

					sizeEventHandler();
				}

				break;
			}

			case KeyPress:{
				XKeyPressedEvent kpe = e.xkey;

				KeySym key = XLookupKeysym( &kpe, 0 );

				switch( key ){
					case XK_Escape:
						quit = true;
				}

				break;
			}

			case ClientMessage:{
				XClientMessageEvent cme = e.xclient;

				Atom WM_DELETE_WINDOW = XInternAtom( window.display, "WM_DELETE_WINDOW", True );
				if( (Atom)cme.data.l[0] == WM_DELETE_WINDOW ){
					quit = true;
				}

				break;
			}

			default:
				throw "Unrecognized event type!";
		}

	}

	return 0;
}


#endif //COMMON_XLIB_WSI_H
