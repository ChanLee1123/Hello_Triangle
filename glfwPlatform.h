// GLFW platform dependent WSI handling and event loop

#ifndef COMMON_GLFW_WSI_H
#define COMMON_GLFW_WSI_H

#include <functional>
#include <string>
#include <queue>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include "to_string.h"

#include "CompilerMessages.h"


TODO( "Easier to use, but might prevent platform co-existence. Could be namespaced. Make all of this a class?" )
struct PlatformWindow{ GLFWwindow* window; };

std::string getPlatformSurfaceExtensionName();

int messageLoop( PlatformWindow window );

bool platformPresentationSupport( VkInstance instance, VkPhysicalDevice device, uint32_t queueFamilyIndex, PlatformWindow window );

PlatformWindow initWindow( int canvasWidth, int canvasHeight );
void killWindow( PlatformWindow window );

VkSurfaceKHR initSurface( VkInstance instance, PlatformWindow window );
// killSurface() is not platform dependent

void setSizeEventHandler( std::function<void(void)> newSizeEventHandler );
void setPaintEventHandler( std::function<void(void)> newPaintEventHandler );

void showWindow( PlatformWindow window );

// Implementation
//////////////////////////////////

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

struct GlfwError{
	int error;
	std::string description;
};

std::queue<GlfwError> errors;

void glfwErrorCallback( int error, const char* description ){
	errors.push( {error, description} );
}

void myInitGlfw(){
	glfwSetErrorCallback( glfwErrorCallback );

	auto success = glfwInit();
	if( !success ) throw "Trouble initializing GLFW!";

	if( !glfwVulkanSupported() ) throw "GLFW has trouble acquiring Vulkan support";
}

void showWindow( PlatformWindow window ){
	glfwShowWindow( window.window );
}

bool endsWith( const std::string& what, const std::string& ending ){
	if( ending.size() > what.size() ) return false;

	return std::equal( ending.rbegin(), ending.rend(), what.rbegin() );
}

std::string getPlatformSurfaceExtensionName(){
	TODO( "Leaks GLFW if killWindow never called." )
	myInitGlfw();

	uint32_t count;
	const char** extensions = glfwGetRequiredInstanceExtensions( &count );

	if( !count || !extensions ) throw "GLFW failed to return required extensions!";

	if( count != 2 )  throw "GLFW returned unexpected number of required extensions!";

	const std::string surfaceE = "VK_KHR_surface";
	if( extensions[0] == surfaceE ){
		if(  endsWith( extensions[1], "_surface" )  ) return extensions[1];
		else throw "Unexpected GLFW required instance extensions!";
	}
	else if( extensions[1] == surfaceE ){
		if(  endsWith( extensions[0], "_surface" )  ) return extensions[0];
		else throw "Unexpected GLFW required instance extensions!";
	}
	else throw "Unexpected GLFW required instance extensions!";
}

void windowSizeCallback( GLFWwindow*, int, int ){
	sizeEventHandler();
}

void windowRefreshCallback( GLFWwindow* ){
	paintEventHandler();
}

int messageLoop( PlatformWindow window ){

	while(  !glfwWindowShouldClose( window.window )  ){
		if( !errors.empty() ) throw to_string( errors.front().error ) + ": " + errors.front().description;

		if(  glfwGetKey( window.window, GLFW_KEY_ESCAPE ) == GLFW_PRESS  ){
			glfwSetWindowShouldClose( window.window, GLFW_TRUE );
		}

		paintEventHandler(); // repaint always

		glfwPollEvents();
	}

	return EXIT_SUCCESS;
}


bool platformPresentationSupport( VkInstance instance, VkPhysicalDevice device, uint32_t queueFamilyIndex, PlatformWindow ){
	return glfwGetPhysicalDevicePresentationSupport( instance, device, queueFamilyIndex ) == GLFW_TRUE;
}

PlatformWindow initWindow( int canvasWidth, int canvasHeight ){
	myInitGlfw();

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
	GLFWwindow* window = glfwCreateWindow( canvasWidth, canvasHeight, "Hello Vulkan Triangle", NULL, NULL );

	if( !window ) throw "Trouble creating GLFW window!";

	glfwSetInputMode( window, GLFW_STICKY_KEYS, GLFW_TRUE );

	glfwSetWindowSizeCallback( window, windowSizeCallback );
	glfwSetWindowRefreshCallback( window, windowRefreshCallback );

	return { window };
}

void killWindow( PlatformWindow window ){
	glfwDestroyWindow( window.window );
	glfwTerminate();
}

VkSurfaceKHR initSurface( VkInstance instance, PlatformWindow window ){
	VkSurfaceKHR surface;
	VkResult errorCode = glfwCreateWindowSurface( instance, window.window, nullptr, &surface ); RESULT_HANDLER( errorCode, "glfwCreateWindowSurface" );

	return surface;
}


#endif //COMMON_GLFW_WSI_H