# 3D Game Engine (in development)

A cutting-edge 3D game engine crafted in C++ and powered by OpenGL. This robust framework offers a comprehensive suite of tools for game developers, emphasizing immersive player interactions, dynamic platform mechanics, and a highly customizable GUI system. Whether you're a seasoned developer or an aspiring game creator, this engine provides the essential building blocks for bringing your 3D game visions to life. The compiled 3D game engine now includes the dlls in parent folder so can be run without compiling it yourself or instilling nay dependicies.

## Know Bugs
- Menu gittering when entering it while moving. 
## Features

### Player Movement
- **Basic Actions**: Walk, sprint, crouch, and jump.
- **Advanced Mechanics**:
  - **Stamina System**: Limits sprinting and jumping.
  - **Crouch Mechanics**: Includes collision detection to prevent uncrouching beneath objects.
  - **Auto Step**: Player can automaticaly step up small platforms of 0.5f or less in height.

### Block and Platform System
- Create blocks and platforms of customizable sizes.
- Define platform start heights for varied level designs.
- Platforms are fully interactable, supporting collision and navigation.
- Define platform textures location for unique visual effects and to have variation in maps.
- Player can walk up platforms that are 0.5f or less in height without jumping.

### Graphical User Interface (GUI)
- Customizable fixed GUI layout.
- Add and manage items to enhance user experience.

### Menu System
- **Pause Menu**: Access by pressing ESC key
- **Interactive Buttons**:
  - Resume: Returns to gameplay
  - Quit: Exits the application
  - Toggle FPS:  Shows/hides FPS counter
  - Toggle VSync:  Enables/disables VSync
  - Toggle Fullscreen:  Enables/disables fullscreen mode
  - Wireframes: Enables/disables wireframe mode for debugging
- **Visual Features**:
  - Clean button layout with consistent spacing
  - Text rendering using custom font system
  - Smooth cursor interaction
  - Text typing affect on buttons text
- **State Management**:
  - Automatic camera position saving/restoration
  - Cursor mode switching between gameplay and menu

## Getting Started

### Prerequisites
- **Libraries**: 
  - [GLFW](https://www.glfw.org/) - For managing windows and inputs.
  - [GLEW](http://glew.sourceforge.net/) - For handling OpenGL extensions.
- **Development Environment**: Any C++ compiler supporting C++17 or later.
- **Build System**: Makefile-based setup (editable for custom requirements).

### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/RalphAKing/3D-Game-Engin-in-Cpp-Test.git
   ```
2. Navigate to the project directory:
   ```bash
   cd 3D-Game-Engin-in-Cpp-Test
   ```
3. Build the project using the provided Makefile:
   ```bash
   make
   ```

### Running the Engine
Run the compiled executable to start the game engine:
```bash
./main
```

## Code Highlights

### Core Movement Mechanics
- Comprehensive player movement system with jumping, sprinting, crouching, and stamina control.
- Collision detection for platforms and surrounding objects.
- Auto step up mechanic for platforms under 0.5f in height.

### Platform Interaction
- Ability to create and manipulate platforms with adjustable height and size.
- Dynamic collision detection for smooth navigation and interactions.

### GUI Customization
- Easily add GUI elements through a dedicated interface.

## Contributing

Contributions are welcome! To contribute:
1. Fork the repository.
2. Create a feature branch:
   ```bash
   git checkout -b feature-name
   ```
3. Commit your changes:
   ```bash
   git commit -m "Description of your changes"
   ```
4. Push to the branch:
   ```bash
   git push origin feature-name
   ```
5. Submit a pull request.

## Future Plans
- Advanced lighting and shadows.
- Chickens
- Physics engine integration.
- Enhanced level editor for seamless design.

## License

This project is licensed under the MIT License. See the full license text below:

```plaintext
MIT License

Copyright (c) 2024-2025 Ralph King

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```