#include "asset_manager.h"
#include <raylib.h>

Texture2D plantAtlas;
Texture2D gardenTexture;
Texture2D planterAtlas;
Texture2D cursorTexture_1;
Texture2D cursorTexture_water;
Texture2D cursorTexture_planter;
Texture2D cursorTexture_plant;
Texture2D cursorTexture_feed;
Texture2D cursorTexture_remove;
Texture2D slab1Texture;
Font uiFont;
Font debugFont;

void assetManager_loadAssets() {
    char *fontUrl = "resources/fonts/micro_5/regular.ttf";
    uiFont = LoadFont(fontUrl);
    debugFont = LoadFont("resources/fonts/roboto/static/Roboto-Bold.ttf");

    plantAtlas = LoadTexture("resources/assets/plants.png");

    gardenTexture = LoadTexture("resources/assets/floor.png");
    SetTextureFilter(gardenTexture, TEXTURE_FILTER_BILINEAR);

    slab1Texture = LoadTexture("resources/assets/slab1.png");

    planterAtlas = LoadTexture("resources/assets/planters.png");

    cursorTexture_1 = LoadTexture("resources/assets/cursor_1.png");
    cursorTexture_water = LoadTexture("resources/assets/cursor_water.png");
    cursorTexture_planter = LoadTexture("resources/assets/cursor_planter.png");
    cursorTexture_plant = LoadTexture("resources/assets/cursor_plant.png");
    cursorTexture_feed = LoadTexture("resources/assets/cursor_feed.png");
    cursorTexture_remove = LoadTexture("resources/assets/cursor_remove.png");
}

// If this is done when the game closes, is it really necesary?
void assetManager_unloadAssets() {
    UnloadFont(uiFont);
    UnloadTexture(plantAtlas);
    UnloadTexture(gardenTexture);
    UnloadTexture(planterAtlas);
    UnloadTexture(cursorTexture_1);
    UnloadTexture(cursorTexture_water);
    UnloadTexture(cursorTexture_planter);
    UnloadTexture(cursorTexture_plant);
    UnloadTexture(cursorTexture_feed);
    UnloadTexture(cursorTexture_remove);
    UnloadTexture(slab1Texture);
}
