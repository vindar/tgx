@page intro_concepts Installation. 

The library is cross-platform: it can used on on CPU or on an embedded platform with the Arduino framework. 

It is written in pure C++ and all the sources are located in the `/src` subdirectory. This folder contains both the header files (`.h`) and the implementation files (`.cpp`, `.inl`). 

*The library has no external dependency.*

## Using TGX on a microcontroller (Arduino)


- **With the Arduino IDE.**

  Install the library directly from within the library manager of the IDE: `[Sketch] > [Include Library] > [Manage Libraries]` and then search for 'TGX' and install the latest version. 

- **With PlaftormIO.**

  Install the library directly from within VSCode/PlaftormIO: `[PIO Home] > [libraries]`  then search for 'TGX' , select it and press `[add to Project]`. 
  
  
## Using TGX on a desktop computer (Windows/Linux/MacOS)

There are two options: 

- **Manually adding the library code directly to a project.**  

  
  
    1. Copy all the files from the `\src` directory directly into the target project directory.
    2. Reference all TGX's `.cpp` files to be compiled together with the project's source files.  
  


- **Building a library file.**  

  Alternatively, it is possible to build a standalone library file. A CMake configuration `CMakeLists.txt` is provided at the root of the library:  
    1. Install CMake 3.10 (or later).
    2. Open a terminal/shell at the library root folder and type:
        - `mkdir build`
        - `cd build`
        - `cmake ..`
    3. Build the library using the generated project files.
        - *On Windows.* Open the Visual Studio solution file 'tgx.sln' and build the  library from within the IDE.
        - *On Linux/MacOS.* Use command `make` to build the library.  
  TGX can now be added to any project be appending TGX's `/src` subdirectory to the project's include path and adding the library to the linker.



