#include <iostream>
#include <raylib.h>
#include <math.h>
#include <chrono>

//textures
#include "textures.h"

//code
#include "checkpoints.hpp"
#include "enums.hpp"
#include "drawrace.hpp"

using namespace std;

int main(void)
{
    //mode 7 transformations
    float a = 1.0f;
    float b = 0.0f;

    float h = 315.5f; //yeah its weird but its perfectly centered on the starting line
    float v = 808.0f;

    float xn = 128.0f + h;
    float yn = 112.0f + v;

    const float aChange = 8000;

    //time  
    float previousTime = 0.0f;
    float deltaTime = 0.0f;

    unsigned long long elapsedFrames = 0;

    //kart physics

    float angle = 4.71238897f;
    float positiveAngle = angle;
    float modAngle = angle;
    float forwardX = 0.0f;
    float forwardY = 0.0f;
    float rotationEpsilon = 0.03f;

    float velocity = 0.0f;
    const float acceleration = 0.25f;
    const float maxVelocity = 1.25f;
    const float drag = 0.125f;

    const float offroadVelocityMultiplier = 0.75f;

    float torque = 0.0f;
    float torqueAcceleration = 0.0375f;
    const float maxTorque = 0.06f;
    const float torqueDrag = 0.015f;

    bool hasFinishedRace = false;

    const float kartCollisionOffset = -0.5f;

    //kart drawing

    int kartPosX = 128;
    int kartPosY = 0;
    int kartSize = 64;
    int kartOffsetYForFinish = 0;
    float finishAngle = 0.0f;
    bool isReversed = false;

    KartState kartState = SCALE0;
    int kartAnimationFrame = 0;

    int spriteReversed = 0; //0 = not reversed, 1 = reversed
    float timeAboutToDrift = 0.0f;
    float accumulatedRotation = 0.0f;

    unsigned int originalWheelColor = 0x5b7a7aff;
    unsigned int rotatedWheelColor = 0x385b5b;

    int wiggleRate = 1;
    int wiggledKartPos = kartPosY + wiggleRate;

    bool drawDirtParticles = false;

    //lap count
    int currentCheckpoint = 5;
    int currentLap = 4;
    int displayedLap = 1;

    //map
    float mapScaling = 0.5;

    //horizon
    float horizonHeight = 32;
    float hillTurnSpeed = -50; //multiplier for how fast the hills move (based on angle)
    float treeTurnSpeed = -90; //multiplier for how fast the trees move (based on angle)
    int hillTextureWidth = 768;

    //initialization
    const int screenWidth = 256;
    const int screenHeight = 224;

    InitWindow(screenWidth, screenHeight, "Super Mario Kart (raylib)");

    SetTargetFPS(60); 

    //game loop
    while (!WindowShouldClose())
    {
        //update up/down velocity
        if(IsKeyDown(KEY_W) && !hasFinishedRace) { velocity += acceleration; }
        if(IsKeyDown(KEY_S) && !hasFinishedRace) { velocity -= acceleration; }
        
        //update left/right turning
        if(IsKeyDown(KEY_A) && !hasFinishedRace) { torque += (torqueAcceleration * max(velocity, 1.0f)); }
        if(IsKeyDown(KEY_D) && !hasFinishedRace) { torque -= (torqueAcceleration * max(velocity, 1.0f)); }

        //clamp velocity and make it move toward zero
        if(velocity < 0) { velocity += drag; }
        if(velocity > 0) { velocity -= drag; }  
        if(velocity > maxVelocity) { velocity = maxVelocity; }
        if(velocity < -maxVelocity) { velocity = -maxVelocity; }
        
        //clamp turning force and make it move toward zero
        //WHAT IS THIS
        if(torque > 0) { torque -= torqueDrag; }
        if(torque < 0) { torque += torqueDrag; }
        if(torque < 0.02 && torque > -0.02) { torque = 0.0; } 
        if(torque > maxTorque) { torque = maxTorque; }
        if(torque < -maxTorque) { torque = -maxTorque; }

        //collision
        //(0,0,0) = track, (255,255,255) = offroad, (127,127,127) = endline, (90,90,90) = coin, (100,100,100) = wall/bounceback, (200,200,200) = block
        //(44, 44, 44) = checkpoint 1 on track, (77, 77, 77) = checkpoint 1 offroad, (45, 45, 45) = checkpoint 2 on track, (78, 78, 78) = checkpoint 2 offroad
        //(46, 46, 46) = checkpoint 3 on track, (79, 79, 79) = checkpoint 3 offroad, (47, 47, 47) = checkpoint 4 on track, (80, 80, 80) = checkpoint 4 offroad
        //(48, 48, 48) = checkpoint 5 on track, (81, 81, 81) = checkpoint 5 offroad, (43, 43, 43) = checkpoint 0 on track, (76, 76, 76) = checkpoint 0 offroad
        
        //check if on track
        //this line took a whole week to fix aaaaaaaaaaaaaaaaaaaaaaaaaaaa
        //thank you to my brother for finding the bug in this one line in 30 seconds. i spent like a week trying to fix the bug and he found it instantly lmao.

        unsigned int currentCollisionColor = mariocircuit1CollisionColorLookup[mariocircuit1CollisionColorList[128 - (int)floor(xn / 8 - forwardX * kartCollisionOffset)][128 - (int)floor(yn / 8 - forwardY * kartCollisionOffset)]];
        unsigned int currentCheckpointColor = mariocircuit1CheckpointColorLookup[mariocircuit1CheckpointColorList[128 - (int)floor(xn / 8 - forwardX * kartCollisionOffset)][128 - (int)floor(yn / 8 - forwardY * kartCollisionOffset)]];

        //this is awesome code. i totally would not get fired if this was done in a professional environment
        if(currentCollisionColor == 0xffffffff) { velocity *= offroadVelocityMultiplier; drawDirtParticles = true; }
        else { drawDirtParticles = false; }

        //check if the player hits a wall
        if(currentCollisionColor == 0x646464ff) { velocity = -velocity * 4; }

        //update current checkpoint
        UpdateCheckpoint(currentCheckpointColor, currentCheckpoint, currentLap);

        if(currentLap == 5)
        {
            if(!hasFinishedRace)
            {
                hasFinishedRace = true;
                finishAngle = positiveAngle;
            }       
        }

        if(hasFinishedRace)
        {
            if(!isReversed)
            {
                //turn around
                angle += 0.03f;
                positiveAngle += 0.03f;
                modAngle = fmod(positiveAngle, 6.28319f);
                if((modAngle < (finishAngle - 3.14159f) + rotationEpsilon && modAngle > (finishAngle - 3.14159f) - rotationEpsilon) || kartAnimationFrame == 12) { isReversed = true; }
                kartAnimationFrame = (int)abs(round((finishAngle - positiveAngle) / 0.2855995455f));
                spriteReversed = 0;
            }
            else
            {
                kartAnimationFrame = 11;
                //rise into air
                kartOffsetYForFinish -= 3;
            }
        }

        displayedLap = max(displayedLap, currentLap);

        //update mode 7 variables
        v -= forwardY * velocity;
        h -= forwardX * velocity;
        yn -= forwardY * velocity;
        xn -= forwardX * velocity;
        angle += torque;
        positiveAngle += torque;
        if(positiveAngle <= 0) { positiveAngle = 6.28319f; }

        //calculate these vectors. thank you random man on stackoverflow for this code
        forwardX = sin(angle);
        forwardY = cos(angle);

        BeginDrawing();
        //draw horizon/bg
        DrawHorizon(horizonHeight, screenWidth, angle, treeTurnSpeed, hillTurnSpeed, hillTextureWidth, hillsColorLookup, hillsColorList, treesColorLookup, treesColorList);

        DrawTrack(screenHeight, horizonHeight, mapScaling, screenWidth, a, b, h, v, xn, yn, aChange, forwardX, forwardY);

        //draw kart

        accumulatedRotation += velocity;
        if(fmod(accumulatedRotation, 7.5) > 3.75) { marioSpritesheetColorLookup[10] = rotatedWheelColor; kartPosY = wiggledKartPos; }
        else { marioSpritesheetColorLookup[10] = originalWheelColor; kartPosY = wiggledKartPos - wiggleRate; }

        if(!hasFinishedRace)
        {
            //calculate the current sprite based off of the current torque
            if(torque < 0.01) { kartAnimationFrame = 0; spriteReversed = 0; }
            if(torque > -0.01) { kartAnimationFrame = 0; spriteReversed = 0; }

            if(torque > 0.01) { kartAnimationFrame = 1; spriteReversed = 1; }
            if(torque < -0.01) { kartAnimationFrame = 1; spriteReversed = 0; }

            if(torque > 0.05) { kartAnimationFrame = 2; spriteReversed = 1; timeAboutToDrift += deltaTime; }
            if(torque < -0.05) { kartAnimationFrame = 2; spriteReversed = 0; timeAboutToDrift += deltaTime; }
        }

        if(!IsKeyDown(KEY_A) && !IsKeyDown(KEY_D)) { timeAboutToDrift = 0; }

        for (int x = 0; x < kartSize; x++)
        {
            for (int y = 0; y < kartSize; y++)
            {
                DrawPixel((kartPosX - (kartSize >> 1)) + x, (kartPosY - (kartSize >> 1) + kartOffsetYForFinish) + y + (112 + (kartSize >> 1)), GetColor(marioSpritesheetColorLookup[marioSpritesheetColorList[kartState * kartSize + y][kartAnimationFrame * kartSize + x + (spriteReversed * kartSize * 11)]]));
            }
        }

        //draw big dirt particles (sorry for the hardcoded numbers im going insane from coding and i dont feel like making variables)
        if(drawDirtParticles)
        {
            if(velocity > 0.1f || velocity < -0.1f)
                for (int x = 0; x < 32; x++)
                {
                    for (int y = 0; y < 32; y++)
                    {
                        //left particles
                        DrawPixel(kartPosX + 11 + x + 4 * floor(sinf(accumulatedRotation * 2)), kartPosY + 150 + y + 2 * floor(1 - sinf(accumulatedRotation * 2)), GetColor(particlesColorLookup[particles[y][x]]));
                        //right particles
                        DrawPixel(screenWidth - (kartPosX + 11 + x + 4 * floor(sinf(accumulatedRotation * 2))), kartPosY + 150 + y + 2 * floor(1 - sinf(accumulatedRotation * 2)), GetColor(particlesColorLookup[particles[y][x]]));
                    }
                }
               
        }

        //hud
        //lap count
        for(int x = 0; x < 32; x++)
        {
            for(int y = 0; y < 8; y++)
            {
                //bad way of doubling the size of the text. listen im tired ok
                DrawPixel(x * 2 + 10, y * 2 + 10, GetColor(lapColorLookup[lapCount[y][x]]));
                DrawPixel(x * 2 + 1 + 10, y * 2 + 10, GetColor(lapColorLookup[lapCount[y][x]]));
                DrawPixel(x * 2 + 10, y * 2 + 1 + 10, GetColor(lapColorLookup[lapCount[y][x]]));
                DrawPixel(x * 2 + 1 + 10, y * 2 + 1 + 10, GetColor(lapColorLookup[lapCount[y][x]]));
            }
        }
        for(int x = 0; x < 8; x++)
        {
            for(int y = 0; y < 8; y++)
            {
                //bad way of doubling the size of the text. listen im tired ok
                DrawPixel(x * 2 + 70, y * 2 + 10, GetColor(numbersColorLookup[numbers[y][x + 8 * displayedLap]]));
                DrawPixel(x * 2 + 1 + 70, y * 2 + 10, GetColor(numbersColorLookup[numbers[y][x + 8 * displayedLap]]));
                DrawPixel(x * 2 + 70, y * 2 + 1 + 10, GetColor(numbersColorLookup[numbers[y][x + 8 * displayedLap]]));
                DrawPixel(x * 2 + 1 + 70, y * 2 + 1 + 10, GetColor(numbersColorLookup[numbers[y][x + 8 * displayedLap]]));
            }
        }

        EndDrawing();
        
        //update the time variables
        deltaTime = (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() / 1000000000 - previousTime);
        previousTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() / 1000000000;
        elapsedFrames++;
    }
    
    //deinitialization
    CloseWindow();
    return 0;
}