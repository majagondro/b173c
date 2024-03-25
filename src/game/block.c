#include "block.h"
#include "common.h"
#include "world.h"
#include "client/console.h"
#include "client/client.h"
#include "client/cvar.h"

enum { // this is an enum because of syntax highlighting
    TRANSPARENT = 0, OPAQUE = 1
};

#define TEXTURE_ALL(idx) {idx,idx,idx,idx,idx,idx}
#define TEXTURE_TOP_BOT_SIDE(top, bot, side) {bot, top, side, side, side, side}
#define TEXTURE_TOPBOT_SIDE(topbot, side) {topbot, topbot, side, side, side, side}

static block_properties blocks[256] = {
    [BLOCK_AIR]                        = {"tile.air", 0.0f, TEXTURE_ALL(0), TRANSPARENT, RENDER_NONE},
    [BLOCK_STONE]                      = {"tile.stone", 1.5f, TEXTURE_ALL(1), OPAQUE, RENDER_CUBE},
    [BLOCK_GRASS]                      = {"tile.grass", 0.6f, TEXTURE_TOP_BOT_SIDE(0, 2, 3), OPAQUE, RENDER_CUBE},
    [BLOCK_DIRT]                       = {"tile.dirt", 0.5f, TEXTURE_ALL(2), OPAQUE, RENDER_CUBE},
    [BLOCK_COBBLESTONE]                = {"tile.stonebrick", 2.0f, TEXTURE_ALL(16), OPAQUE, RENDER_CUBE},
    [BLOCK_WOOD_PLANKS]                = {"tile.wood", 2.0f, TEXTURE_ALL(4), OPAQUE, RENDER_CUBE},
    [BLOCK_SAPLING]                    = {"tile.sapling", 0.0f, TEXTURE_ALL(15), TRANSPARENT, RENDER_CROSS},
    [BLOCK_BEDROCK]                    = {"tile.bedrock", -1.0f, TEXTURE_ALL(17), OPAQUE, RENDER_CUBE},
    [BLOCK_WATER_MOVING]               = {"tile.water", 100.0f, TEXTURE_ALL(255 - 32), TRANSPARENT, RENDER_FLUID},
    [BLOCK_WATER_STILL]                = {"tile.water", 100.0f, TEXTURE_ALL(255 - 32), TRANSPARENT, RENDER_FLUID},
    [BLOCK_LAVA_MOVING]                = {"tile.lava", 0.0f, TEXTURE_ALL(255), TRANSPARENT, RENDER_FLUID},
    [BLOCK_LAVA_STILL]                 = {"tile.lava", 100.0f, TEXTURE_ALL(255), TRANSPARENT, RENDER_FLUID},
    [BLOCK_SAND]                       = {"tile.sand", 0.5f, TEXTURE_ALL(18), OPAQUE, RENDER_CUBE},
    [BLOCK_GRAVEL]                     = {"tile.gravel", 0.6f, TEXTURE_ALL(19), OPAQUE, RENDER_CUBE},
    [BLOCK_ORE_GOLD]                   = {"tile.oreGold", 3.0f, TEXTURE_ALL(32), OPAQUE, RENDER_CUBE},
    [BLOCK_ORE_IRON]                   = {"tile.oreIron", 3.0f, TEXTURE_ALL(33), OPAQUE, RENDER_CUBE},
    [BLOCK_ORE_COAL]                   = {"tile.oreCoal", 3.0f, TEXTURE_ALL(34), OPAQUE, RENDER_CUBE},
    [BLOCK_WOOD_LOG]                   = {"tile.log", 2.0f, TEXTURE_TOPBOT_SIDE(21, 20), OPAQUE, RENDER_CUBE},
    [BLOCK_LEAVES]                     = {"tile.leaves", 0.2f, TEXTURE_ALL(52), TRANSPARENT, RENDER_CUBE},
    [BLOCK_SPONGE]                     = {"tile.sponge", 0.6f, TEXTURE_ALL(48), OPAQUE, RENDER_CUBE},
    [BLOCK_GLASS]                      = {"tile.glass", 0.3f, TEXTURE_ALL(49), TRANSPARENT, RENDER_CUBE},
    [BLOCK_ORE_LAPIS]                  = {"tile.oreLapis", 3.0f, TEXTURE_ALL(160), OPAQUE, RENDER_CUBE},
    [BLOCK_BLOCK_LAPIS]                = {"tile.blockLapis", 3.0f, TEXTURE_ALL(144), OPAQUE, RENDER_CUBE},
    [BLOCK_DISPENSER]                  = {"tile.dispenser", 3.5f, TEXTURE_TOPBOT_SIDE(45 + 17, 45), OPAQUE, RENDER_CUBE},
    [BLOCK_SANDSTONE]                  = {"tile.sandStone", 0.8f, TEXTURE_TOP_BOT_SIDE(192 - 16, 192 + 16, 192), OPAQUE, RENDER_CUBE},
    [BLOCK_NOTEBLOCK]                  = {"tile.musicBlock", 0.8f, TEXTURE_ALL(74), OPAQUE, RENDER_CUBE},
    [BLOCK_BED]                        = {"tile.bed", 0.2f, TEXTURE_ALL(134), TRANSPARENT, RENDER_BED},
    [BLOCK_RAIL_POWERED]               = {"tile.goldenRail", 0.7f, TEXTURE_ALL(179), TRANSPARENT, RENDER_RAIL},
    [BLOCK_RAIL_DETECTOR]              = {"tile.detectorRail", 0.7f, TEXTURE_ALL(195), TRANSPARENT, RENDER_RAIL},
    [BLOCK_PISTON_BASE_STICKY]         = {"tile.pistonStickyBase", 0.5f, TEXTURE_TOP_BOT_SIDE(106, 106 + 3, 106 + 2), TRANSPARENT, RENDER_PISTON_BASE},
    [BLOCK_COBWEB]                     = {"tile.web", 4.0f, TEXTURE_ALL(11), TRANSPARENT, RENDER_CROSS},
    [BLOCK_TALLGRASS]                  = {"tile.tallgrass", 0.0f, TEXTURE_ALL(39), TRANSPARENT, RENDER_CROSS},
    [BLOCK_DEADBUSH]                   = {"tile.deadbush", 0.0f, TEXTURE_ALL(55), TRANSPARENT, RENDER_CROSS},
    [BLOCK_PISTON_BASE]                = {"tile.pistonBase", 0.5f, TEXTURE_TOP_BOT_SIDE(107, 107 + 2, 107 + 1), TRANSPARENT, RENDER_PISTON_BASE},
    [BLOCK_PISTON_EXTENSION]           = {"null", 0.5f, TEXTURE_ALL(250), TRANSPARENT, RENDER_PISTON_EXTENSION},
    [BLOCK_CLOTH]                      = {"tile.cloth", 0.8f, TEXTURE_ALL(64), OPAQUE, RENDER_CUBE},
    [BLOCK_PISTON_MOVING]              = {"null", -1.0f, TEXTURE_ALL(240), TRANSPARENT, RENDER_NONE},
    [BLOCK_FLOWER_DANDELION]           = {"tile.flower", 0.0f, TEXTURE_ALL(13), TRANSPARENT, RENDER_CROSS},
    [BLOCK_FLOWER_ROSE]                = {"tile.rose", 0.0f, TEXTURE_ALL(12), TRANSPARENT, RENDER_CROSS},
    [BLOCK_MUSHROOM_BROWN]             = {"tile.mushroom", 0.0f, TEXTURE_ALL(29), TRANSPARENT, RENDER_CROSS},
    [BLOCK_MUSHROOM_RED]               = {"tile.mushroom", 0.0f, TEXTURE_ALL(28), TRANSPARENT, RENDER_CROSS},
    [BLOCK_BLOCK_GOLD]                 = {"tile.blockGold", 3.0f, TEXTURE_ALL(23), OPAQUE, RENDER_CUBE},
    [BLOCK_BLOCK_IRON]                 = {"tile.blockIron", 5.0f, TEXTURE_ALL(22), OPAQUE, RENDER_CUBE},
    [BLOCK_SLAB_DOUBLE]                = {"tile.stoneSlab", 2.0f, TEXTURE_ALL(6), OPAQUE, RENDER_CUBE},
    [BLOCK_SLAB_SINGLE]                = {"tile.stoneSlab", 2.0f, TEXTURE_ALL(6), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_BRICK]                      = {"tile.brick", 2.0f, TEXTURE_ALL(7), OPAQUE, RENDER_CUBE},
    [BLOCK_TNT]                        = {"tile.tnt", 0.0f, TEXTURE_TOP_BOT_SIDE(8 + 1, 8 + 2, 8), OPAQUE, RENDER_CUBE},
    [BLOCK_BOOKSHELF]                  = {"tile.bookshelf", 1.5f, TEXTURE_TOPBOT_SIDE(4, 35), OPAQUE, RENDER_CUBE},
    [BLOCK_COBBLESTONE_MOSSY]          = {"tile.stoneMoss", 2.0f, TEXTURE_ALL(36), OPAQUE, RENDER_CUBE},
    [BLOCK_OBSIDIAN]                   = {"tile.obsidian", 10.0f, TEXTURE_ALL(37), OPAQUE, RENDER_CUBE},
    [BLOCK_TORCH]                      = {"tile.torch", 0.0f, TEXTURE_ALL(80), TRANSPARENT, RENDER_TORCH},
    [BLOCK_FIRE]                       = {"tile.fire", 0.0f, TEXTURE_ALL(31), TRANSPARENT, RENDER_FIRE},
    [BLOCK_MOB_SPAWNER]                = {"tile.mobSpawner", 5.0f, TEXTURE_ALL(65), TRANSPARENT, RENDER_CUBE},
    [BLOCK_STAIRS_WOOD]                = {"tile.stairsWood", 2.0f, TEXTURE_ALL(4), TRANSPARENT, RENDER_STAIRS},
    [BLOCK_CHEST]                      = {"tile.chest", 2.5f, TEXTURE_TOPBOT_SIDE(26 - 1, 26), OPAQUE, RENDER_CUBE},
    [BLOCK_REDSTONE_DUST]              = {"tile.redstoneDust", 0.0f, TEXTURE_ALL(164), TRANSPARENT, RENDER_WIRE},
    [BLOCK_ORE_DIAMOND]                = {"tile.oreDiamond", 3.0f, TEXTURE_ALL(50), OPAQUE, RENDER_CUBE},
    [BLOCK_BLOCK_DIAMOND]              = {"tile.blockDiamond", 5.0f, TEXTURE_ALL(24), OPAQUE, RENDER_CUBE},
    [BLOCK_WORKBENCH]                  = {"tile.workbench", 2.5f, TEXTURE_TOP_BOT_SIDE(59 - 16, 4, 59), OPAQUE, RENDER_CUBE},
    [BLOCK_CROP_WHEAT]                 = {"tile.crops", 0.0f, TEXTURE_ALL(88), TRANSPARENT, RENDER_CROPS},
    [BLOCK_FARMLAND]                   = {"tile.farmland", 0.6f, TEXTURE_TOP_BOT_SIDE(87, 2, 2), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_FURNACE_IDLE]               = {"tile.furnace", 3.5f, TEXTURE_TOPBOT_SIDE(45 + 17, 45), OPAQUE, RENDER_CUBE},
    [BLOCK_FURNACE_ACTIVE]             = {"tile.furnace", 3.5f, TEXTURE_TOPBOT_SIDE(45 + 17, 45), OPAQUE, RENDER_CUBE},
    [BLOCK_SIGN_POST]                  = {"tile.sign", 1.0f, TEXTURE_ALL(4), TRANSPARENT, RENDER_NONE},
    [BLOCK_DOOR_WOOD]                  = {"tile.doorWood", 3.0f, TEXTURE_ALL(97), TRANSPARENT, RENDER_DOOR},
    [BLOCK_LADDER]                     = {"tile.ladder", 0.4f, TEXTURE_ALL(83), TRANSPARENT, RENDER_LADDER},
    [BLOCK_RAIL]                       = {"tile.rail", 0.7f, TEXTURE_ALL(128), TRANSPARENT, RENDER_RAIL},
    [BLOCK_STAIRS_STONE]               = {"tile.stairsStone", 2.0f, TEXTURE_ALL(16), TRANSPARENT, RENDER_STAIRS},
    [BLOCK_SIGN_WALL]                  = {"tile.sign", 1.0f, TEXTURE_ALL(4), TRANSPARENT, RENDER_NONE},
    [BLOCK_LEVER]                      = {"tile.lever", 0.5f, TEXTURE_ALL(96), TRANSPARENT, RENDER_LEVER},
    [BLOCK_PRESSURE_PLATE_STONE]       = {"tile.pressurePlate", 0.5f, TEXTURE_ALL(1), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_DOOR_IRON]                  = {"tile.doorIron", 5.0f, TEXTURE_ALL(98), TRANSPARENT, RENDER_DOOR},
    [BLOCK_PRESSURE_PLATE_WOOD]        = {"tile.pressurePlate", 0.5f, TEXTURE_ALL(4), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_ORE_REDSTONE]               = {"tile.oreRedstone", 3.0f, TEXTURE_ALL(51), OPAQUE, RENDER_CUBE},
    [BLOCK_ORE_REDSTONE_GLOWING]       = {"tile.oreRedstone", 3.0f, TEXTURE_ALL(51), OPAQUE, RENDER_CUBE},
    [BLOCK_TORCH_REDSTONE_DISABLED]    = {"tile.notGate", 0.0f, TEXTURE_ALL(115), TRANSPARENT, RENDER_TORCH},
    [BLOCK_TORCH_REDSTONE_ENABLED]     = {"tile.notGate", 0.0f, TEXTURE_ALL(99), TRANSPARENT, RENDER_TORCH},
    [BLOCK_BUTTON]                     = {"tile.button", 0.5f, TEXTURE_ALL(1), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_SNOW_LAYER]                 = {"tile.snow", 0.1f, TEXTURE_ALL(66), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_ICE]                        = {"tile.ice", 0.5f, TEXTURE_ALL(67), TRANSPARENT, RENDER_CUBE},
    [BLOCK_SNOW]                       = {"tile.snow", 0.2f, TEXTURE_ALL(66), OPAQUE, RENDER_CUBE},
    [BLOCK_CACTUS]                     = {"tile.cactus", 0.4f, TEXTURE_TOP_BOT_SIDE(70 - 1, 70 + 1, 70), TRANSPARENT, RENDER_CACTUS},
    [BLOCK_CLAY]                       = {"tile.clay", 0.6f, TEXTURE_ALL(72), OPAQUE, RENDER_CUBE},
    [BLOCK_SUGAR_CANE]                 = {"tile.reeds", 0.0f, TEXTURE_ALL(73), TRANSPARENT, RENDER_CROSS},
    [BLOCK_JUKEBOX]                    = {"tile.jukebox", 2.0f, TEXTURE_TOP_BOT_SIDE(75, 74, 74), OPAQUE, RENDER_CUBE},
    [BLOCK_FENCE]                      = {"tile.fence", 2.0f, TEXTURE_ALL(4), TRANSPARENT, RENDER_FENCE},
    [BLOCK_PUMPKIN]                    = {"tile.pumpkin", 1.0f, TEXTURE_TOPBOT_SIDE(102, 102 + 16), OPAQUE, RENDER_CUBE},
    [BLOCK_NETTHERRACK]                = {"tile.hellrock", 0.4f, TEXTURE_ALL(103), OPAQUE, RENDER_CUBE},
    [BLOCK_SOUL_SAND]                  = {"tile.hellsand", 0.5f, TEXTURE_ALL(104), TRANSPARENT, RENDER_CUBE},
    [BLOCK_GLOWSTONE]                  = {"tile.lightgem", 0.3f, TEXTURE_ALL(105), OPAQUE, RENDER_CUBE},
    [BLOCK_PORTAL]                     = {"tile.portal", -1.0f, TEXTURE_ALL(224), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_PUMPKIN_LANTERN]            = {"tile.litpumpkin", 1.0f, TEXTURE_TOPBOT_SIDE(102, 102 + 16), OPAQUE, RENDER_CUBE},
    [BLOCK_CAKE]                       = {"tile.cake", 0.5f, TEXTURE_TOP_BOT_SIDE(121, 121 + 3, 121 + 1), TRANSPARENT, RENDER_CUBE_SPECIAL},
    [BLOCK_REDSTONE_REPEATER_DISABLED] = {"tile.diode", 0.0f, TEXTURE_ALL(131), TRANSPARENT, RENDER_REPEATER},
    [BLOCK_REDSTONE_REPEATER_ENABLED]  = {"tile.diode", 0.0f, TEXTURE_ALL(147), TRANSPARENT, RENDER_REPEATER},
    [BLOCK_TRAPDOOR]                   = {"tile.lockedchest", 0.0f, TEXTURE_ALL(84), TRANSPARENT, RENDER_CUBE_SPECIAL},
};

block_properties block_get_properties(block_id id)
{
    return blocks[id & 255];
}

bool block_should_render_face(int x, int y, int z, block_data self, block_face face)
{
    vec3_t face_offsets[6] = {
        [BLOCK_FACE_Y_NEG] = vec3(0, -1, 0),
        [BLOCK_FACE_Y_POS] = vec3(0, 1, 0),
        [BLOCK_FACE_Z_NEG] = vec3(0, 0, -1),
        [BLOCK_FACE_Z_POS] = vec3(0, 0, 1),
        [BLOCK_FACE_X_NEG] = vec3(-1, 0, 0),
        [BLOCK_FACE_X_POS] = vec3(1, 0, 0),
    };

    block_data other;
    vec3_t pos2 = vec3(x, y, z);
    pos2 = vec3_add(pos2, face_offsets[face]);

    other = world_get_block((int) pos2.x, (int) pos2.y, (int) pos2.z);

    if(block_is_semi_transparent(self) && block_is_semi_transparent(other))
        return false;

    if(block_is_transparent(self)) {
        if(self.id == BLOCK_BED) {
            switch(self.metadata & METADATA_BED_ROT) {
            case 0:
                if(self.metadata & METADATA_BED_PILLOW)
                    return face != BLOCK_FACE_Z_NEG;
                else
                    return face != BLOCK_FACE_Z_POS;
            case 1:
                if(self.metadata & METADATA_BED_PILLOW)
                    return face != BLOCK_FACE_X_POS;
                else
                    return face != BLOCK_FACE_X_NEG;
            case 2:
                if(self.metadata & METADATA_BED_PILLOW)
                    return face != BLOCK_FACE_Z_POS;
                else
                    return face != BLOCK_FACE_Z_NEG;
            case 3:
                if(self.metadata & METADATA_BED_PILLOW)
                    return face != BLOCK_FACE_X_NEG;
                else
                    return face != BLOCK_FACE_X_POS;
            default:
                return true;
            }
        }
        if(self.id == BLOCK_CACTUS && other.id == BLOCK_CACTUS && IS_Y_FACE(face)) {
            return false;
        }
        if(self.id == BLOCK_SLAB_SINGLE)
            return true;
        if(self.id != BLOCK_LEAVES || r_smartleaves.integer) {
            if(other.id == self.id) {
                return false;
            }
        }
    }

    if(block_get_properties(self.id).render_type == RENDER_FLUID) {
        if(face == BLOCK_FACE_Y_POS) {
            return true;
        }
    }

    return block_is_transparent(other);
}

float block_fluid_get_percent_air(ubyte metadata)
{
    if(metadata >= 8)
        metadata = 0;
    return (float) (metadata + 1) / 9.0f;
}

static bool is_same_fluid_material(block_id self, block_id other)
{
    if(other == self)
        return true;
    if(self == BLOCK_WATER_MOVING)
        return other == BLOCK_WATER_STILL;
    if(self == BLOCK_WATER_STILL)
        return other == BLOCK_WATER_MOVING;
    if(self == BLOCK_LAVA_MOVING)
        return other == BLOCK_LAVA_STILL;
    if(self == BLOCK_LAVA_STILL)
        return other == BLOCK_LAVA_MOVING;
    return false;
}

float block_fluid_get_height(int x, int y, int z, block_id self_id)
{
    int total = 0;
    float total_air = 0.0f;

    for(int side = 0; side < 4; side++) {
        int xoff = x - (side & 1);
        int zoff = z - (side >> 1 & 1);
        block_data other;

        if(world_get_block(xoff, y + 1, zoff).id == self_id) {
            return 1.0f;
        }

        other = world_get_block(xoff, y, zoff);
        if(other.id != self_id && !is_same_fluid_material(self_id, other.id)) {
            if(!block_is_solid(other)) {
                total_air++;
                total++;
            }
        } else {
            if(other.metadata >= 8 || other.metadata == 0) {
                total_air += block_fluid_get_percent_air(other.metadata) * 10.0f;
                total += 10;
            }
            total_air += block_fluid_get_percent_air(other.metadata);
            total++;
        }
    }
    return 1.0f - total_air / (float) total;
}

static int fluid_get_decay(bool lava, int x, int y, int z)
{
    block_data block = world_get_block(x, y, z);
    int metadata = block.metadata;

    if(lava && block.id != BLOCK_LAVA_STILL && block.id != BLOCK_LAVA_MOVING)
        return -1;
    if(!lava && block.id != BLOCK_WATER_STILL && block.id != BLOCK_WATER_MOVING)
        return -1;

    if(metadata >= 8)
        metadata = 0;
    return metadata;
}

vec3_t block_fluid_get_flow_direction(int x, int y, int z)
{
    vec3_t flow_dir = {0};
    block_data orig = world_get_block(x, y, z);
    bool lava = orig.id == BLOCK_LAVA_MOVING || orig.id == BLOCK_LAVA_STILL;
    int decay = fluid_get_decay(lava, x, y, z);

    for(int side = 0; side < 4; side++) {
        int xoff = x;
        int zoff = z;
        if(side == 0)
            xoff--;
        if(side == 1)
            zoff--;
        if(side == 2)
            xoff++;
        if(side == 3)
            zoff++;
        {
            int decay2 = fluid_get_decay(lava, xoff, y, zoff);
            int diff;
            if(decay2 >= 0) {
                diff = decay2 - decay;
                flow_dir = vec3_add(flow_dir, vec3((xoff - x) * diff, 0, (zoff - z) * diff));
            } else {
                if(!block_is_solid(world_get_block(xoff, y, zoff))) {
                    decay2 = fluid_get_decay(lava, xoff, y - 1, zoff);
                    if(decay2 >= 0) {
                        diff = decay2 - (decay - 8);
                        flow_dir = vec3_add(flow_dir, vec3((xoff - x) * diff, 0, (zoff - z) * diff));
                    }
                }
            }
        }
    }

    flow_dir = vec3_normalize(flow_dir);
    return flow_dir;
}

void onchange_block_render_modes(void)
{
    blocks[BLOCK_LEAVES].opaque = !r_fancyleaves.integer;
    if(world_is_init()) {
        world_mark_all_for_remesh();
    }
}

bool block_is_collidable(block_data block)
{
    return !bbox_null(block_get_bbox(block, 0, 0, 0, false));
}

bool block_is_selectable(block_data block)
{
    return !bbox_null(block_get_bbox(block, 0, 0, 0, true));
}

float block_get_break_progress_per_tick(entity *miner)
{

}

const char *face_to_string[] = {
    [BLOCK_FACE_Y_NEG] = "-Y",
    [BLOCK_FACE_Y_POS] = "+Y",
    [BLOCK_FACE_Z_NEG] = "-Z",
    [BLOCK_FACE_Z_POS] = "+Z",
    [BLOCK_FACE_X_NEG] = "-X",
    [BLOCK_FACE_X_POS] = "+X"
};

const char *block_face_to_str(block_face f)
{
    if(f < BLOCK_FACE_Y_NEG || f > BLOCK_FACE_X_POS)
        return "???";
    return face_to_string[f % 7];
}
