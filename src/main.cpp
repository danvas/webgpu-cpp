#include <iostream>
#include <GLFW/glfw3.h>

int main(int, char **)
{
  std::cout << "Hello, world!" << std::endl;

  if (!glfwInit())
  {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return 1;
  }
  GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
  {
    if (!window)
    {
      std::cerr << "Could not open window!" << std::endl;
      glfwTerminate();
      return 1;
    }

    while (!glfwWindowShouldClose(window))
    {
      // Check whether the user clicked on the close button (and any other
      // mouse/key event, which we don't use so far)
      glfwPollEvents();
    }
  }
  glfwDestroyWindow(window);

  return 0;
}