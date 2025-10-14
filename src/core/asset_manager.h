#include <raylib.h>

// Change to a struct and a function to get assetManager instance if this grows
extern Texture2D plantAtlas;
extern Texture2D gardenTexture;
extern Texture2D planterAtlas;
extern Texture2D cursorTexture_1;
extern Texture2D cursorTexture_water;
extern Texture2D cursorTexture_planter;
extern Texture2D cursorTexture_plant;
extern Texture2D cursorTexture_feed;
extern Texture2D cursorTexture_remove;
extern Texture2D slab1Texture;
extern Font uiFont;
extern Font debugFont;

void assetManager_loadAssets();
void assetManager_unloadAssets();
