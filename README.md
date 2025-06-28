# Blueprint Task Forge

![BTFCVard](https://github.com/user-attachments/assets/ca80110d-f518-48b7-98e9-ce97faf13465)

A powerful Unreal Engine plugin that enables you to create custom Blueprint nodes without writing C++. Build asynchronous tasks, custom functionality, and complex workflows directly in Blueprints with a visual, node-based approach.

## 🚀 What is Blueprint Task Forge?

Blueprint Task Forge (formerly Blueprint Node Template by Alexandr Marchenko) revolutionizes how you create custom Blueprint nodes in Unreal Engine. Instead of diving into complex C++ K2_Node APIs, you can now create sophisticated custom nodes using only Blueprints - making advanced functionality accessible to all developers, regardless of C++ experience.

## ✨ Key Features

### 🎯 **No C++ Required**
- Create custom Blueprint nodes using only Blueprint classes
- Extend base classes to define your task behavior
- Visual workflow design without complex API knowledge

### ⚡ **Asynchronous & Synchronous Support**
- Build latent (asynchronous) nodes similar to `AIMoveTo`
- Create regular synchronous functions
- Full control over execution flow and timing

### 🔌 **Advanced Pin Configuration**
- **Spawn Parameters**: Expose class properties as input pins
- **Auto-Call Functions**: Functions that execute automatically after object creation
- **Exec Functions**: Expose object functions as execution pins
- **Input/Output Delegates**: Connect Blueprint events as input/output parameters
- **Custom Output Pins**: Create specialized output pins with custom payload data

### 🎨 **Visual Customization**
- **Node Decorators**: Customize the visual appearance of your nodes
- **Status Indicators**: Real-time status display during execution
- **Custom Colors**: Visual feedback through node title and background colors
- **Tooltips & Descriptions**: Rich documentation directly in the node interface

### 🛠️ **Developer-Friendly Tools**
- **Task Palette**: Dedicated panel for browsing and organizing custom tasks
- **Instance Editing**: Edit task properties directly in the Details panel
- **Runtime Debugging**: Track active tasks and their status during gameplay
- **Validation System**: Compile-time validation with helpful error messages

## 📋 Requirements

- **Unreal Engine**: 5.1 or later
- **License**: Compatible with all Unreal Engine license types
- **Platform**: Windows, Mac, Linux

## 💬 Community & Support
[DiscordServer](https://discord.gg/b9e3u68p8t)

## 🏃‍♂️ Quick Start

### 1. Installation
1. Download the latest release or clone this repository
2. Extract/copy to your project's `Plugins` folder
3. Enable "Blueprint Task Forge" in the Plugin Manager
4. Restart the editor

### 2. Create Your First Task

1. **Create a new Blueprint class** inheriting from `Btf Task Forge`
2. **Add your logic**:
  - Create public functions for task behavior
  - Add delegates for event callbacks
  - Define properties for configuration
3. **Configure exposure options** in the class defaults:
  - Mark functions for auto-execution or manual triggering
  - Expose properties as spawn parameters
  - Set up input/output delegates
4. **Use in Blueprints**: Your custom node appears automatically in the Blueprint palette

## 📚 Essential Concepts
- **Spawn Parameters**: Properties exposed as input pins on the node
- **Auto-Call Functions**: Functions executed automatically during task initialization
- **Exec Functions**: Functions exposed as execution input pins
- **Output Delegates**: Events that create execution output pins
- **Custom Output Pins**: Specialized outputs with structured data

## 🎯Advanced Features
- **Node Decorators**: Create custom visual elements and additional functionality
- **Instance Mode**: Edit task properties directly in the node's details panel
- **Class Limitations**: Restrict which Blueprint classes can use specific tasks
- **Custom Validation**: Implement compile-time validation for your tasks

## 📄 License
See LICENSE file for more details.

## 🙏 Acknowledgments
Originally based on the (now discontinued) Blueprint Node Template plugin by Alexandr Marchenko
