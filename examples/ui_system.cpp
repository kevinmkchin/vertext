#include "ui_system.hpp"
#include "common.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "player_system.hpp"
#include "file_system.hpp"
#include "input.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define VERTEXT_IMPLEMENTATION
#include "vertext.h"

#define TEXT_SIZE 64
INTERNAL vtxt_font font_c64;
INTERNAL TextureHandle texture_c64;
INTERNAL vtxt_font font_medusa_gothic;
INTERNAL TextureHandle texture_medusa_gothic;


void LoadFont(vtxt_font* font_handle, TextureHandle* font_atlas, const char* font_path, u8 font_size, bool useNearest)
{
    BinaryFileHandle fontfile;
    ReadFileBinary(fontfile, font_path);
    assert(fontfile.memory);
    vtxt_init_font(font_handle, (u8*) fontfile.memory, font_size);
    FreeFileBinary(fontfile);
    CreateTextureFromBitmap(*font_atlas, font_handle->font_atlas.pixels, font_handle->font_atlas.width, 
        font_handle->font_atlas.height, GL_RED, GL_RED, (useNearest ? GL_NEAREST : GL_LINEAR));
    free(font_handle->font_atlas.pixels);
}

UISystem::UISystem()
{
    cachedGameStage = GAME_NOT_STARTED;
}

void UISystem::Init(RenderSystem* render_sys_arg, WorldSystem* world_sys_arg, PlayerSystem* player_sys_arg)
{
    renderer = render_sys_arg;
    world = world_sys_arg;
    playerSystem = player_sys_arg;

    LoadFont(&font_c64, &texture_c64, font_path("c64.ttf").c_str(), 32, true);
    LoadFont(&font_medusa_gothic, &texture_medusa_gothic, font_path("medusa-gothic.otf").c_str(), TEXT_SIZE);

    renderer->worldTextFontPtr = &font_c64;
    renderer->worldTextFontAtlas = texture_c64;
}

void UISystem::PushWorldText(vec2 pos, const std::string& text, u32 size)
{
    WorldText newWorldText;
    newWorldText.pos = pos;
    newWorldText.size = size;
    newWorldText.text = text;
    renderer->worldTextsThisFrame.push_back(newWorldText);
}

void UISystem::UpdateHealthBarUI(float dt)
{
    if(registry.players.size() > 0)
    {
        auto e = registry.players.entities[0];

        auto& playerHP = registry.healthBar.get(e);

        float currentHP = playerHP.health;
        float lowerBound = 0.f;
        float upperBound = playerHP.maxHealth;

        renderer->healthPointsNormalized = ((currentHP - lowerBound) / (upperBound - lowerBound));
    }
}

void UISystem::UpdateExpUI(float dt)
{
    if(registry.players.size() > 0)
    {
        Player playerComponent = registry.players.components[0];

        float currentExp = playerComponent.experience;
        float lowerBound = 0.f;
        float upperBound = 9999.f;
        for(int i = 1; i < ARRAY_COUNT(PLAYER_EXP_THRESHOLDS_ARRAY); ++i)
        {
            float il = PLAYER_EXP_THRESHOLDS_ARRAY[i-1];
            float iu = PLAYER_EXP_THRESHOLDS_ARRAY[i];
            if(currentExp < iu)
            {
                lowerBound = il;
                upperBound = iu;
                break;
            }
        }

        renderer->expProgressNormalized = ((currentExp - lowerBound) / (upperBound - lowerBound));
    }
}

#pragma warning(push)
#pragma warning(disable : 4996)
void UISystem::UpdateTextUI(float dt)
{
    vtxt_setflags(VTXT_CREATE_INDEX_BUFFER|VTXT_USE_CLIPSPACE_COORDS);
    vtxt_backbuffersize(UI_LAYER_RESOLUTION_WIDTH, UI_LAYER_RESOLUTION_HEIGHT);

    vtxt_clear_buffer();

    switch(world->GetCurrentMode())
    {
        case MODE_MAINMENU:
        {
            vtxt_move_cursor(350, 340);
            vtxt_append_line("ASCENT", &font_c64, 72);
            vtxt_move_cursor(350, 500);
            vtxt_append_line("PLAY (ENTER)", &font_c64, 48);
            vtxt_move_cursor(350, 660);
            vtxt_append_line("HELP (H)", &font_c64, 48);
            vtxt_move_cursor(350, 820);
            vtxt_append_line("EXIT (Q)", &font_c64, 48);
            vtxt_move_cursor(350, 960);
            if (world->GetCurrentDifficulty() == DIFFICULTY_EASY) { 
                vtxt_append_line("SWAP DIFFICULTY [STANDARD] (R)", &font_c64, 36); 
            }
            else {
                vtxt_append_line("SWAP DIFFICULTY [HARD] (R)", &font_c64, 36);
            }
            
        }break;
        case MODE_INGAME:
        {
            Entity playerEntity = registry.players.entities[0];
            TransformComponent& playerTransform = registry.transforms.get(playerEntity);
            MotionComponent& playerMotion = registry.motions.get(playerEntity);
            CollisionComponent& playerCollider = registry.colliders.get(playerEntity);
            HealthBar& playerHealth = registry.healthBar.get(playerEntity);
            GoldBar& playerGold = registry.goldBar.get(playerEntity);

            char textBuffer[128];

            int displayHealth = (playerHealth.health > 0.f && playerHealth.health < 1.f) ? 1 : (int) playerHealth.health;
            sprintf(textBuffer, "HP: %d/%d", displayHealth, (int) playerHealth.maxHealth);
            vtxt_move_cursor(270, 70);
            vtxt_append_line(textBuffer, &font_c64, 24);

            sprintf(textBuffer, "GOLD: %d", (int)playerGold.coins);
            vtxt_move_cursor(40, 130);
            vtxt_append_line(textBuffer, &font_c64, 32);

            sprintf(textBuffer, "Lvl %d", (int) 1);
            if(registry.players.size() > 0)
            {      
                Player playerComponent = registry.players.components[0];
                sprintf(textBuffer, "Lvl %d", playerComponent.level);
            }
            vtxt_move_cursor(900, UI_LAYER_RESOLUTION_HEIGHT - 8);
            vtxt_append_line(textBuffer, &font_c64, 24);

            if(world->gamePaused)
            {
                vtxt_move_cursor(860, 556);
                vtxt_append_line("PAUSED", &font_c64, 32);
            }

            if(registry.players.size() > 0)
            {      
                Player playerComponent = registry.players.components[0];
                if(playerComponent.bDead)
                {
                    vtxt_move_cursor(700, 580);
                    vtxt_append_line("GAME OVER", &font_c64, 64);
                }
            }

        }break;
    }

    vtxt_vertex_buffer vb = vtxt_grab_buffer();
    renderer->textLayer1FontAtlas = texture_c64;
    renderer->textLayer1Colour = vec4(1.f,1.f,1.f,1.0f);
    RebindMeshBufferObjects(renderer->textLayer1VAO, vb.vertex_buffer, vb.index_buffer, vb.vertices_array_count, vb.indices_array_count);

    LOCAL_PERSIST float showTutorialTimer = 0.f;

    // chapter change text
    LOCAL_PERSIST bool showChapterText = false;
    LOCAL_PERSIST float chapterTextAlpha = 0.f;
    if(world->GetCurrentStage() != cachedGameStage)
    {
        cachedGameStage = world->GetCurrentStage();
        showChapterText = true;
        chapterTextAlpha = 3.0f;
    }
    if(showChapterText)
    {
        if(!world->gamePaused)
        {
            chapterTextAlpha -= 0.5f * dt;
        }

        // 2022-03-29 (Kevin): Commented out showing the tutorial after chapter text goes away
        // if(chapterTextAlpha < 0.f)
        // {
        //     showChapterText = false;

        //     if(cachedGameStage == CHAPTER_ONE_STAGE_ONE)
        //     {
        //         *GlobalPauseForSeconds = 9999.f;
        //         showTutorialTimer = 9999.f;
        //         world->darkenGameFrame = true;
        //     }
        // }

        vtxt_clear_buffer();

        if(!world->gamePaused)
        {
            switch(cachedGameStage)
            {
                case CHAPTER_TUTORIAL:
                {
                    vtxt_move_cursor(100,800);
                    vtxt_append_line("Prologue", &font_medusa_gothic, 110);
                    vtxt_move_cursor(100,930);
                    vtxt_append_line("Village at the Base of the Mountain", &font_medusa_gothic, 80);
                }break;
                case CHAPTER_ONE_STAGE_ONE:
                {
                    vtxt_move_cursor(100,800);
                    vtxt_append_line("Chapter One", &font_medusa_gothic, 110);
                    vtxt_move_cursor(100,930);
                    vtxt_append_line("Ancestral Caves", &font_medusa_gothic, 80);
                }break;
                case CHAPTER_TWO_STAGE_ONE:
                {
                    vtxt_move_cursor(100,800);
                    vtxt_append_line("Chapter Two", &font_medusa_gothic, 110);
                    vtxt_move_cursor(100,930);
                    vtxt_append_line("Eternal Forest", &font_medusa_gothic, 80);
                }break;
                case CHAPTER_THREE_STAGE_ONE:
                {
                    vtxt_move_cursor(100,800);
                    vtxt_append_line("Chapter Three", &font_medusa_gothic, 110);
                    vtxt_move_cursor(100,930);
                    vtxt_append_line("Mountaintop of Warriors", &font_medusa_gothic, 80);
                }break;
                case CHAPTER_BOSS:
                {
                    vtxt_move_cursor(100,800);
                    vtxt_append_line("Evil Sorcerer Izual", &font_medusa_gothic, 110);
                    vtxt_move_cursor(100,930);
                    vtxt_append_line("Final Fight", &font_medusa_gothic, 80);
                }break;
            }
        }

        vb = vtxt_grab_buffer();
        renderer->textLayer2FontAtlas = texture_medusa_gothic;
        renderer->textLayer2Colour = vec4(1.f,1.f,1.f,chapterTextAlpha);
        RebindMeshBufferObjects(renderer->textLayer2VAO, vb.vertex_buffer, vb.index_buffer, vb.vertices_array_count, vb.indices_array_count);
    }

    if(showTutorialTimer > 0.f && !world->gamePaused)
    {
        vtxt_clear_buffer();

        showTutorialTimer -= dt;

        if(Input::HasKeyBeenPressed(SDL_SCANCODE_J) || Input::GetGamepad(0).HasBeenPressed(GAMEPAD_A))
        {
            showTutorialTimer = -1.f;
            *GlobalPauseForSeconds = -1.f;
            world->darkenGameFrame = false;
        }

        if(Input::GetGamepad(0).isConnected)
        {
            vtxt_move_cursor(260, 190);
            vtxt_append_line("D-Pad or Left Thumbstick to move.", &font_c64, 40);
            vtxt_move_cursor(260, 260);
            vtxt_append_line("A to jump", &font_c64, 40);
            vtxt_move_cursor(260, 330);
            vtxt_append_line("X to attack", &font_c64, 40);
            vtxt_move_cursor(260, 400);
            vtxt_append_line("B to pick up item", &font_c64, 40);
            vtxt_move_cursor(260, 450);
            vtxt_append_line("B while holding item to throw item", &font_c64, 40);
            vtxt_move_cursor(260, 500);
            vtxt_append_line("B + down to drop item", &font_c64, 40);
            vtxt_move_cursor(320, 800);
            vtxt_append_line("Press A to continue...", &font_c64, 40);
        }
        else
        {
            vtxt_move_cursor(260, 190);
            vtxt_append_line("WASD to move.", &font_c64, 40);
            vtxt_move_cursor(260, 260);
            vtxt_append_line("J to jump", &font_c64, 40);
            vtxt_move_cursor(260, 330);
            vtxt_append_line("K to attack", &font_c64, 40);
            vtxt_move_cursor(260, 400);
            vtxt_append_line("L to pick up item", &font_c64, 40);
            vtxt_move_cursor(260, 450);
            vtxt_append_line("L while holding item to throw item", &font_c64, 40);
            vtxt_move_cursor(260, 500);
            vtxt_append_line("L + down to drop item", &font_c64, 40);
            vtxt_move_cursor(320, 800);
            vtxt_append_line("Press J to continue...", &font_c64, 40);
        }

        vtxt_vertex_buffer vb = vtxt_grab_buffer();
        renderer->textLayer1FontAtlas = texture_c64;
        renderer->textLayer1Colour = vec4(1.f,1.f,1.f,1.0f);
        RebindMeshBufferObjects(renderer->textLayer1VAO, vb.vertex_buffer, vb.index_buffer, vb.vertices_array_count, vb.indices_array_count);
    }
}

void UISystem::UpdateLevelUpUI(float dt)
{
    vtxt_clear_buffer();

    renderer->showMutationSelect = false;

    switch(world->GetCurrentMode())
    {
        case MODE_INGAME:
        {
            Entity playerEntity = registry.players.entities[0];

            LOCAL_PERSIST float levelUpTextTimer = 0.f;
            LOCAL_PERSIST bool pickThreeMutations = false;
            LOCAL_PERSIST Mutation mutationOptions[3];
            if(playerSystem->bLeveledUpLastFrame)
            {
                levelUpTextTimer = 100000.0f;
                *GlobalPauseForSeconds = 100000.0f;
                pickThreeMutations = true;
            }

            if(levelUpTextTimer > 0.f)
            {
                if(!world->gamePaused)
                {
                    levelUpTextTimer -= dt;
                }

                if(levelUpTextTimer > 99999.f)
                {
                    vtxt_move_cursor(670, 580);
                    vtxt_append_line("Level Up!", &font_c64, 80);
                }
                else
                {
                    if(pickThreeMutations)
                    {
                        std::vector<Mutation> mutations = world->allPossibleMutations;
                        // TODO(Kevin): remove mutations that player already has
                        int mut1;
                        int mut2;
                        int mut3;
                        PickThreeRandomInts(&mut1, &mut2, &mut3, (int)mutations.size());
                        mutationOptions[0] = mutations[mut1];
                        mutationOptions[1] = mutations[mut2];
                        mutationOptions[2] = mutations[mut3];
                        pickThreeMutations = false;
                    }

                    // mutationOptions
                    vtxt_move_cursor(180, 350);
                    vtxt_append_line(mutationOptions[0].name.c_str(), &font_c64, 28);
                    vtxt_move_cursor(754, 350);
                    vtxt_append_line(mutationOptions[1].name.c_str(), &font_c64, 28);
                    vtxt_move_cursor(1333, 350);
                    vtxt_append_line(mutationOptions[2].name.c_str(), &font_c64, 28);

                    // 19 char wide
                    std::string mut1desc = mutationOptions[0].description;
                    std::string mut2desc = mutationOptions[1].description;
                    std::string mut3desc = mutationOptions[2].description;
                    int descCursorY = 500;
                    while(mut1desc.length() > 0)
                    {
                        std::string toPrint = mut1desc.substr(0, 24);
                        mut1desc = mut1desc.erase(0, 24);
                        vtxt_move_cursor(180, descCursorY);
                        descCursorY += 25;
                        vtxt_append_line(toPrint.c_str(), &font_c64, 20);
                    }

                    descCursorY = 500;
                    while(mut2desc.length() > 0)
                    {
                        std::string toPrint = mut2desc.substr(0, 24);
                        mut2desc = mut2desc.erase(0, 24);
                        vtxt_move_cursor(754, descCursorY);
                        descCursorY += 25;
                        vtxt_append_line(toPrint.c_str(), &font_c64, 20);
                    }

                    descCursorY = 500;
                    while(mut3desc.length() > 0)
                    {
                        std::string toPrint = mut3desc.substr(0, 24);
                        mut3desc = mut3desc.erase(0, 24);
                        vtxt_move_cursor(1333, descCursorY);
                        descCursorY += 25;
                        vtxt_append_line(toPrint.c_str(), &font_c64, 20);
                    }

                    vtxt_move_cursor(574, 900);
                    if(Input::GetGamepad(0).isConnected)
                    {
                        vtxt_append_line("Press A to select mutation...", &font_c64, 32);
                    }
                    else
                    {
                        vtxt_append_line("Press SPACE to select mutation...", &font_c64, 32);
                    }

                    if(Input::GameLeftHasBeenPressed())
                    {
                        --(renderer->mutationSelectionIndex);
                        if(Mix_PlayChannel(-1, world->blip_select_sound, 0) == -1) 
                        {
                            printf("Mix_PlayChannel: %s\n",Mix_GetError());
                        }
                    }
                    if(Input::GameRightHasBeenPressed())
                    {
                        ++(renderer->mutationSelectionIndex);
                        if(Mix_PlayChannel(-1, world->blip_select_sound, 0) == -1) 
                        {
                            printf("Mix_PlayChannel: %s\n",Mix_GetError());
                        }
                    }
                    renderer->mutationSelectionIndex = (renderer->mutationSelectionIndex + 3) % 3;
                    renderer->showMutationSelect = true;

                    if(Input::GameJumpHasBeenPressed())
                    {
                        Mutation mutationToAdd = mutationOptions[renderer->mutationSelectionIndex];
                        ActiveMutationsComponent& playerActiveMutations = registry.mutations.get(playerEntity);
                        playerActiveMutations.mutations.push_back(mutationToAdd);

                        renderer->mutationSelectionIndex = 1;
                        levelUpTextTimer = -1.f;
                        *GlobalPauseForSeconds = 0.0f;
                    }
                }
            }

        }break;
        default:
        {}break;
    }

    vtxt_vertex_buffer vb = vtxt_grab_buffer();
    renderer->textLayer3FontAtlas = texture_c64;
    renderer->textLayer3Colour = vec4(1.f,1.f,1.f,1.0f);
    RebindMeshBufferObjects(renderer->textLayer3VAO, vb.vertex_buffer, vb.index_buffer, vb.vertices_array_count, vb.indices_array_count);  
}

void UISystem::UpdateShopUI(float dt) {

    renderer->showShopSelect = false;
    vtxt_clear_buffer();

    if (world->GetCurrentMode() == MODE_INGAME) {
        Entity playerEntity = registry.players.entities[0];

        if (registry.activeShopItems.size() > 0)
        {
            renderer->showShopSelect = true;
            ActiveShopItem& activeShopItem = registry.activeShopItems.components[0];
            Entity& shopEntity = activeShopItem.linkedEntity[0];
            ShopItem& shopItem = registry.shopItems.get(shopEntity);
            *GlobalPauseForSeconds = 100000.0f;
            std::vector<Mutation> mutations = world->allPossibleMutations;
            Mutation buy = {};

            switch (shopItem.mutationIndex) {
                case 0:
                    buy = mutations[2];
                    break;
                case 1:
                    buy = mutations[0];
                    break;
                case 2:
                    buy = mutations[1];
                    break;
                case 3:
                    buy = mutations[3];
                    break;
                case 4:
                    buy = mutations[6];
                    break;
                default:
                    break;
            }

            vtxt_move_cursor(754, 350);
            vtxt_append_line(buy.name.c_str(), &font_c64, 28);

            int descCursorY = 500;
            std::string mut_desc = buy.description;
            while (mut_desc.length() > 0)
            {
                std::string toPrint = mut_desc.substr(0, 24);
                mut_desc = mut_desc.erase(0, 24);
                vtxt_move_cursor(754, descCursorY);
                descCursorY += 25;
                vtxt_append_line(toPrint.c_str(), &font_c64, 20);
            }

            vtxt_move_cursor(620, 850);
            if (Input::GetGamepad(0).isConnected)
            {
                vtxt_append_line("Press A to buy for 50 gold.", &font_c64, 32);
            }
            else
            {
                vtxt_append_line("Press Z to buy for 50 gold.", &font_c64, 32);
            }

            vtxt_move_cursor(930, 900);
            vtxt_append_line("or", &font_c64, 32);

            vtxt_move_cursor(730, 950);
            if (Input::GetGamepad(0).isConnected)
            {
                vtxt_append_line("Press X to exit...", &font_c64, 32);
            }
            else
            {
                vtxt_append_line("Press X to exit...", &font_c64, 32);
            }

            if (Input::GameAttackHasBeenPressed())
            {
                GoldBar& playerGold = registry.goldBar.get(playerEntity);
                
                if (playerGold.coins >= 50) {
                    
                    playerGold.coins -= 50;
                    
                    ActiveMutationsComponent& playerActiveMutations = registry.mutations.get(playerEntity);
                    playerActiveMutations.mutations.push_back(buy);

                    registry.remove_all_components_of(shopEntity);
                }
                else {
                    // TODO: display UI saying not enough
                    registry.activeShopItems.clear();
                }

                *GlobalPauseForSeconds = 0.0f;
            }

            if (Input::GameCycleItemRightBeenPressed() || Input::GamePickUpHasBeenPressed())
            {
                registry.activeShopItems.clear();
                *GlobalPauseForSeconds = 0.0f;
            }

        }
    }

    vtxt_vertex_buffer vb = vtxt_grab_buffer();
    renderer->textLayer4FontAtlas = texture_c64;
    renderer->textLayer4Colour = vec4(1.f, 1.f, 1.f, 1.0f);
    RebindMeshBufferObjects(renderer->textLayer4VAO, vb.vertex_buffer, vb.index_buffer, vb.vertices_array_count, vb.indices_array_count);
}

#pragma warning(pop)

void UISystem::Step(float deltaTime)
{
    // Check pause
    if(world->GetCurrentMode() == MODE_INGAME && Input::GamePauseHasBeenPressed())
    {
        if(world->gamePaused)
        {
            // unpause
            world->gamePaused = false;
            world->darkenGameFrame = false;
        }
        else
        {
            // pause
            world->gamePaused = true;
            world->darkenGameFrame = true;
        }
    }

    UpdateHealthBarUI(deltaTime);
    UpdateExpUI(deltaTime);
    UpdateTextUI(deltaTime);
    UpdateLevelUpUI(deltaTime);
    UpdateShopUI(deltaTime);
}
