#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include <algorithm>
#include <sstream>
#include <iostream>

#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "text_renderer.h"
#include "database.h"

// Game-related State data
SpriteRenderer* Renderer;
TextRenderer* Text;
Database* ImageDatabase;

std::vector<std::pair<std::string, std::string>> imageKeys;
int currentImageIndex = 0;
bool rightPressedLastFrame = false;

float ShakeTime = 0.0f;

float pixelSize = 100.0f; // start pixelated
float duration = 10.0f;   // animation duration
float elapsed = 0.0f;     // per-image animation time

bool animationPaused = true;

bool spacePressedLastFrame = false;

int CurrentScore = 100;


Game::Game(unsigned int width, unsigned int height)
    : State(GAME_MENU), Keys(), KeysProcessed(), Width(width), Height(height)
{

}

Game::~Game()
{
    delete Renderer;
    delete Text;
    delete ImageDatabase;
}

void Game::Init()
{
    ImageDatabase = new Database();
    if (!ImageDatabase->open("pixelguessr.db")) {
        std::cerr << "Failed to open database!" << std::endl;
        return; // exit
    }

    // load shaders
    ResourceManager::LoadShader("shaders\\sprite.vs", "shaders\\sprite.frag", nullptr, "sprite");
    
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);

    Shader SpriteShader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(SpriteShader);

    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("resources\\fonts\\Antonio-Bold.ttf", 24);

    for (const auto& img : ImageDatabase->getAllImages()) {
        //std::cout << img.name << std::endl;
        bool alpha = (img.category == "c" || img.category == "b"); // or smarter logic
        ResourceManager::LoadTextureFromMemory(img.data, alpha, img.name);
        imageKeys.push_back({ img.name, img.category });
    }

    ImageDatabase->close();
}

void Game::Update(float dt)
{
    if (!animationPaused) {
        elapsed += dt;

        pixelSize = glm::mix(100.0f, 1.0f, elapsed / duration);
        pixelSize = glm::clamp(pixelSize, 1.0f, 100.0f);

        float progress = glm::clamp(elapsed / duration, 0.0f, 1.0f);

        // Ease-in: slow start, fast drop at the end
        float decay = pow(progress, 2.0f);  // Try 2.5f to 3.5f for sharper falloff

        CurrentScore = static_cast<int>(glm::mix(100.0f, 1.0f, decay));
        CurrentScore = glm::clamp(CurrentScore, 1, 100);
    }

    Shader shader = ResourceManager::GetShader("sprite");
    shader.Use();
    shader.SetFloat("pixelSize", pixelSize);
    shader.SetVector2f("resolution", glm::vec2(this->Width, this->Height));
}


void Game::ProcessInput(float dt)
{
    if (this->State == GAME_MENU)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
        }
    }
    if (this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->KeysProcessed[GLFW_KEY_ENTER] = true;
            //Effects->Chaos = false;
            this->State = GAME_MENU;
        }
    }
    if (this->State == GAME_ACTIVE)
    {
        bool rightPressed = this->Keys[GLFW_KEY_RIGHT];
        if (rightPressed && !rightPressedLastFrame) {
            currentImageIndex++;
            if (currentImageIndex >= imageKeys.size())
                currentImageIndex = 0;

            elapsed = 0.0f;
            pixelSize = 100.0f;
            animationPaused = true;
            CurrentScore = 100;
        }
        rightPressedLastFrame = rightPressed;

        static bool leftPressedLastFrame = false;
        bool leftPressed = this->Keys[GLFW_KEY_LEFT];
        if (leftPressed && !leftPressedLastFrame) {
            currentImageIndex--;
            if (currentImageIndex < 0)
                currentImageIndex = imageKeys.size() - 1;

            elapsed = 0.0f;
            pixelSize = 100.0f;
            animationPaused = true;
            CurrentScore = 100;
        }
        leftPressedLastFrame = leftPressed;

        bool spacePressed = this->Keys[GLFW_KEY_SPACE];

        if (spacePressed && !spacePressedLastFrame) {
            animationPaused = !animationPaused; // Toggle pause
        }
        spacePressedLastFrame = spacePressed;

        if (this->Keys[GLFW_KEY_R])
        {
            elapsed = 0.0f;
            pixelSize = 100.0f;
            animationPaused = false;
            CurrentScore = 100;
        }

    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
    {
        
    }
    if (this->State == GAME_ACTIVE)
    {
        glm::vec2 baseSize(Width, Height);
        glm::vec2 scaledSize = baseSize * 1.0f;

        // Center the zoomed image by offsetting position
        glm::vec2 position = glm::vec2((baseSize.x - scaledSize.x) / 2.0f,
            (baseSize.y - scaledSize.y) / 2.0f);

        Texture2D t = ResourceManager::GetTexture(imageKeys[currentImageIndex].first);

        Renderer->DrawSprite(t,
            position,
            scaledSize,
            0.0f,
            glm::vec3(1.0f, 1.0f, 1.0f));

        
        //std::stringstream ss; ss << int?;     

        Text->RenderText("Score: " + std::to_string(CurrentScore), 5.0f, 5.0f, 1.0f);

        if (pixelSize == 1.0f) {
            Text->RenderText("ANSWER : " + imageKeys[currentImageIndex].first, 5.0f, 40.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
    }
    if (this->State == GAME_MENU)
    {
        std::string line1 = "Welcome to Pixelated";
        std::string line2 = "Press ENTER to start";
        float scale = 1.0f;
        float lineSpacing = 10.0f; // Space between lines, in pixels
        float height1 = Text->GetTextHeight(line1, scale);
        float height2 = Text->GetTextHeight(line2, scale);

        // Total block height = line1 + spacing + line2
        float totalHeight = height1 + lineSpacing + height2;
        float startY = ((this->Height - totalHeight) / 2.3f);
        // ---- Line 1 ----
        float width1 = Text->GetTextWidth(line1, scale);
        float ascent1 = Text->GetMaxAscent(line1, scale);
        float x1 = (this->Width - width1) / 2.0f;
        float y1 = startY + ascent1;

        Text->RenderText(line1, x1, y1, scale, glm::vec3(1.0f, 0.0f, 0.0f));

        // ---- Line 2 ----
        float width2 = Text->GetTextWidth(line2, scale);
        float ascent2 = Text->GetMaxAscent(line2, scale);
        float x2 = (this->Width - width2) / 2.0f;
        float y2 = y1 + height1 + lineSpacing;

        Text->RenderText(line2, x2, y2, scale, glm::vec3(1.0f));
    }
    if (this->State == GAME_WIN)
    {
        Text->RenderText("Thanks for playing!!!", 320.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));        
    }


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Controls");
    ImGui::Text("image %d of %d", currentImageIndex + 1,imageKeys.size());
    ImGui::Text("Category: %s", imageKeys[currentImageIndex].second);
    ImGui::SliderFloat("Duration", &duration, 1.0f, 20.0f, "%.1f sec");
    ImGui::End();
}

void Game::UpdateWindowSize(int width, int height)
{
    //this->Width = width;
    //this->Height = height;

    Text->SetScreenSize(width, height);

    //glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    //ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
}