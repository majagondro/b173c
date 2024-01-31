#include "block.h"
#include "common.h"
#include "world.h"

enum { TRANSPARENT = 0, OPAQUE = 1 };

#define TEXTURE_ALL(idx) {idx,idx,idx,idx,idx,idx}
#define TEXTURE_TOP_BOT_SIDE(top, bot, side) {bot, top, side, side, side, side}
#define TEXTURE_TOPBOT_SIDE(topbot, side) {topbot, topbot, side, side, side, side}

static block_properties blocks[256] = {
	[BLOCK_AIR] = {"air", TEXTURE_ALL(0), TRANSPARENT},
	[BLOCK_STONE] = {"stone", TEXTURE_ALL(1), OPAQUE},
	[BLOCK_GRASS] = {"grass", TEXTURE_TOP_BOT_SIDE(0, 2, 3), OPAQUE},
	[BLOCK_DIRT] = {"dirt", TEXTURE_ALL(2), OPAQUE},
	[BLOCK_COBBLESTONE] = {"cobblestone", TEXTURE_ALL(16), OPAQUE},
	[BLOCK_WOOD_PLANKS] = {"planks", TEXTURE_ALL(4), OPAQUE},
	[BLOCK_SAPLING] = {"sapling", TEXTURE_ALL(15), TRANSPARENT},
	[BLOCK_BEDROCK] = {"bedrock", TEXTURE_ALL(17), OPAQUE},
	[BLOCK_WATER_MOVING] = {"water_flowing", TEXTURE_ALL(255-32), TRANSPARENT},
	[BLOCK_WATER_STILL] = {"water_still", TEXTURE_ALL(255-32), TRANSPARENT},
	[BLOCK_LAVA_MOVING] = {"lava_flowing", TEXTURE_ALL(255), TRANSPARENT},
	[BLOCK_LAVA_STILL] = {"lava_still", TEXTURE_ALL(255), TRANSPARENT},
	[BLOCK_SAND] = {"sand", TEXTURE_ALL(18), OPAQUE},
	[BLOCK_GRAVEL] = {"gravel", TEXTURE_ALL(19), OPAQUE},
	[BLOCK_ORE_GOLD] = {"gold_ore", TEXTURE_ALL(32), OPAQUE},
	[BLOCK_ORE_IRON] = {"iron_ore", TEXTURE_ALL(33), OPAQUE},
	[BLOCK_ORE_COAL] = {"coal_ore", TEXTURE_ALL(34), OPAQUE},
	[BLOCK_WOOD_LOG] = {"log", TEXTURE_TOPBOT_SIDE(21, 20), OPAQUE},
	[BLOCK_LEAVES] = {"leaves", TEXTURE_ALL(52), TRANSPARENT},
	[BLOCK_SPONGE] = {"sponge", TEXTURE_ALL(48), OPAQUE},
	[BLOCK_GLASS] = {"glass", TEXTURE_ALL(49), TRANSPARENT},
	[BLOCK_ORE_LAPIS] = {"lapis_ore", TEXTURE_ALL(160), OPAQUE},
	[BLOCK_BLOCK_LAPIS] = {"lapis_block", TEXTURE_ALL(144), OPAQUE},
	[BLOCK_DISPENSER] = {"dispenser", TEXTURE_TOPBOT_SIDE(45+17, 45), OPAQUE},
	[BLOCK_SANDSTONE] = {"sandstone", TEXTURE_TOP_BOT_SIDE(192-16, 192+16, 192), OPAQUE},
	[BLOCK_NOTEBLOCK] = {"noteblock", TEXTURE_ALL(74), OPAQUE},
	[BLOCK_BED] = {"bed", TEXTURE_ALL(134), TRANSPARENT},
	[BLOCK_RAIL_POWERED] = {"rail_powered", TEXTURE_ALL(179), TRANSPARENT},
	[BLOCK_RAIL_DETECTOR] = {"rail_detector", TEXTURE_ALL(195), TRANSPARENT},
	[BLOCK_PISTON_BASE_STICKY] = {"piston_sticky", TEXTURE_TOP_BOT_SIDE(106, 106+3, 106+2), TRANSPARENT},
	[BLOCK_COBWEB] = {"cobweb", TEXTURE_ALL(11), TRANSPARENT},
	[BLOCK_TALLGRASS] = {"tallgrass", TEXTURE_ALL(39), TRANSPARENT},
	[BLOCK_DEADBUSH] = {"deadbush", TEXTURE_ALL(55), TRANSPARENT},
	[BLOCK_PISTON_BASE] = {"piston", TEXTURE_TOP_BOT_SIDE(107, 107+2, 107+1), TRANSPARENT},
	[BLOCK_PISTON_EXTENSION] = {"piston_head", TEXTURE_ALL(250), TRANSPARENT},
	[BLOCK_CLOTH] = {"wool", TEXTURE_ALL(64), OPAQUE},
	[BLOCK_PISTON_MOVING] = {"piston_moving (idk)", TEXTURE_ALL(240), TRANSPARENT},
	[BLOCK_FLOWER_DANDELION] = {"flower_yellow", TEXTURE_ALL(13), TRANSPARENT},
	[BLOCK_FLOWER_ROSE] = {"flower_red", TEXTURE_ALL(12), TRANSPARENT},
	[BLOCK_MUSHROOM_BROWN] = {"mushroom_brown", TEXTURE_ALL(29), TRANSPARENT},
	[BLOCK_MUSHROOM_RED] = {"mushroom_red", TEXTURE_ALL(28), TRANSPARENT},
	[BLOCK_BLOCK_GOLD] = {"gold_block", TEXTURE_ALL(23), OPAQUE},
	[BLOCK_BLOCK_IRON] = {"iron_block", TEXTURE_ALL(22), OPAQUE},
	[BLOCK_SLAB_DOUBLE] = {"slab_double", TEXTURE_ALL(6), OPAQUE},
	[BLOCK_SLAB_SINGLE] = {"slab_single", TEXTURE_ALL(6), TRANSPARENT},
	[BLOCK_BRICK] = {"bricks", TEXTURE_ALL(7), OPAQUE},
	[BLOCK_TNT] = {"tnt", TEXTURE_TOP_BOT_SIDE(8+1, 8+2, 8), OPAQUE},
	[BLOCK_BOOKSHELF] = {"bookshelf", TEXTURE_TOPBOT_SIDE(4, 35), OPAQUE},
	[BLOCK_COBBLESTONE_MOSSY] = {"mossy_cobblestone", TEXTURE_ALL(36), OPAQUE},
	[BLOCK_OBSIDIAN] = {"obsidian", TEXTURE_ALL(37), OPAQUE},
	[BLOCK_TORCH] = {"torch", TEXTURE_ALL(80), TRANSPARENT},
	[BLOCK_FIRE] = {"fire", TEXTURE_ALL(31), TRANSPARENT},
	[BLOCK_MOB_SPAWNER] = {"spawner", TEXTURE_ALL(65), TRANSPARENT},
	[BLOCK_STAIRS_WOOD] = {"stair_wood", TEXTURE_ALL(4), TRANSPARENT},
	[BLOCK_CHEST] = {"chest", TEXTURE_TOPBOT_SIDE(26-1, 26), OPAQUE},
	[BLOCK_REDSTONE_DUST] = {"redstone_wire", TEXTURE_ALL(164), TRANSPARENT},
	[BLOCK_ORE_DIAMOND] = {"diamond_ore", TEXTURE_ALL(50), OPAQUE},
	[BLOCK_BLOCK_DIAMOND] = {"diamond_block", TEXTURE_ALL(24), OPAQUE},
	[BLOCK_WORKBENCH] = {"workbench", TEXTURE_TOP_BOT_SIDE(59-16, 4, 59), OPAQUE},
	[BLOCK_CROP_WHEAT] = {"crops", TEXTURE_ALL(88), TRANSPARENT},
	[BLOCK_FARMLAND] = {"farmland", TEXTURE_TOP_BOT_SIDE(87, 2, 2), TRANSPARENT},
	[BLOCK_FURNACE_IDLE] = {"furnace", TEXTURE_TOPBOT_SIDE(45+17, 45-1), OPAQUE},
	[BLOCK_FURNACE_ACTIVE] = {"furnace_lit", TEXTURE_TOPBOT_SIDE(45+17, 45+16), OPAQUE},
	[BLOCK_SIGN_POST] = {"sign_post", TEXTURE_ALL(4), TRANSPARENT},
	[BLOCK_DOOR_WOOD] = {"door_wood", TEXTURE_ALL(97), TRANSPARENT},
	[BLOCK_LADDER] = {"ladder", TEXTURE_ALL(83), TRANSPARENT},
	[BLOCK_RAIL] = {"rail", TEXTURE_ALL(128), TRANSPARENT},
	[BLOCK_STAIRS_STONE] = {"stair_stone", TEXTURE_ALL(16), TRANSPARENT},
	[BLOCK_SIGN_WALL] = {"sign_wall", TEXTURE_ALL(4), TRANSPARENT},
	[BLOCK_LEVER] = {"lever", TEXTURE_ALL(96), TRANSPARENT},
	[BLOCK_PRESSURE_PLATE_STONE] = {"pressureplate_stone", TEXTURE_ALL(1), TRANSPARENT},
	[BLOCK_DOOR_IRON] = {"door_iron", TEXTURE_ALL(98), TRANSPARENT},
	[BLOCK_PRESSURE_PLATE_WOOD] = {"pressureplate_wood", TEXTURE_ALL(4), TRANSPARENT},
	[BLOCK_ORE_REDSTONE] = {"redstone_ore", TEXTURE_ALL(51), OPAQUE},
	[BLOCK_ORE_REDSTONE_GLOWING] = {"redstone_ore_lit", TEXTURE_ALL(51), OPAQUE},
	[BLOCK_TORCH_REDSTONE_DISABLED] = {"redstone_torch", TEXTURE_ALL(115), TRANSPARENT},
	[BLOCK_TORCH_REDSTONE_ENABLED] = {"redstone_torch_lit", TEXTURE_ALL(99), TRANSPARENT},
	[BLOCK_BUTTON] = {"button", TEXTURE_ALL(1), TRANSPARENT},
	[BLOCK_SNOW_LAYER] = {"snow_layer", TEXTURE_ALL(66), TRANSPARENT},
	[BLOCK_ICE] = {"ice", TEXTURE_ALL(67), TRANSPARENT},
	[BLOCK_SNOW] = {"snow", TEXTURE_ALL(66), OPAQUE},
	[BLOCK_CACTUS] = {"cactus", TEXTURE_TOP_BOT_SIDE(70-1, 70+1, 70), TRANSPARENT},
	[BLOCK_CLAY] = {"clay", TEXTURE_ALL(72), OPAQUE},
	[BLOCK_SUGAR_CANE] = {"reeds", TEXTURE_ALL(73), TRANSPARENT},
	[BLOCK_JUKEBOX] = {"jukebox", TEXTURE_TOP_BOT_SIDE(75, 74, 74), OPAQUE},
	[BLOCK_FENCE] = {"fence", TEXTURE_ALL(4), TRANSPARENT},
	[BLOCK_PUMPKIN] = {"pumpkin", TEXTURE_TOPBOT_SIDE(102, 102+16), OPAQUE},
	[BLOCK_NETTHERRACK] = {"netherrack", TEXTURE_ALL(103), OPAQUE},
	[BLOCK_SOUL_SAND] = {"soulsand", TEXTURE_ALL(104), TRANSPARENT},
	[BLOCK_GLOWSTONE] = {"glowstone", TEXTURE_ALL(105), OPAQUE},
	[BLOCK_PORTAL] = {"portal", TEXTURE_ALL(240), TRANSPARENT}, // todo
	[BLOCK_PUMPKIN_LANTERN] = {"pumpkin_lit", TEXTURE_TOPBOT_SIDE(102, 102+17), OPAQUE},
	[BLOCK_CAKE] = {"cake", TEXTURE_TOP_BOT_SIDE(121, 121+3, 121+1), TRANSPARENT},
	[BLOCK_REDSTONE_REPEATER_DISABLED] = {"redstone_repeater", TEXTURE_ALL(131), TRANSPARENT},
	[BLOCK_REDSTONE_REPEATER_ENABLED] = {"redstone_repeater_lit", TEXTURE_ALL(147), TRANSPARENT},
	[BLOCK_TRAPDOOR] = {"trapdoor", TEXTURE_ALL(84), TRANSPARENT},
};

block_properties block_get_properties(block_id id)
{
	return blocks[id & 255];
}

bool block_should_face_be_rendered(int x, int y, int z, block_data self, block_face face)
{
	vec3 face_offsets[6] = {
		[BLOCK_FACE_Y_NEG] = { 0, -1,  0},
		[BLOCK_FACE_Y_POS] = { 0,  1,  0},
		[BLOCK_FACE_Z_NEG] = { 0,  0, -1},
		[BLOCK_FACE_Z_POS] = { 0,  0,  1},
		[BLOCK_FACE_X_NEG] = {-1,  0,  0},
		[BLOCK_FACE_X_POS] = { 1,  0,  0}
	};

	block_data other;
	vec3 pos2 = {x, y, z};
	vec3_add(pos2, pos2, face_offsets[face]);

	other = world_get_block(pos2[0], pos2[1], pos2[2]);

	if(block_is_semi_transparent(self) && block_is_semi_transparent(other)) {
		return false;
	}

	return block_is_transparent(other);
}
