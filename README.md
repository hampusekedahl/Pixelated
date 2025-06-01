# Pixelated - Image Guessing Game

A pixelated image guessing game built with C++, OpenGL, and SQLite. Images are stored in a SQLite3 database bundled with the game, and are gradually revealed from a pixelated state. Players must guess the image before it's fully shown.

# Demo
![](https://github.com/hampusekedahl/Pixelated/gif/pixelated_demo.gif)

---

## Game Concept

- Images are retrieved from a SQLite3 database at runtime.
- Each image starts highly pixelated.
- Over time, the resolution increases smoothly.
- Players type in their guess before the image is fully revealed.
- The game checks guesses against the image’s metadata (e.g., filename or title).

---

## Tech Stack

- **Language:** C++
- **Graphics:** OpenGL + GLFW3
- **Image Loading:** `stb_image`
- **Text Rendering:** FreeType
- **Database:** SQLite3 (BLOBs for storing images)