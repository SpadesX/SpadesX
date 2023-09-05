-- level.lua: A simple lua script showcasing SapdesX's scripting capabilities.

spadesx = require('spadesx')

-- Function useful to check tables pushed from the C API.
function printTable(table, indent)
    indent = indent or 0
    
    for key, value in pairs(table) do
        if type(value) == "table" then
            print(string.rep("  ", indent) .. key .. ":")
            printTable(value, indent + 1)
        else
            print(string.rep("  ", indent) .. key .. ": " .. tostring(value))
        end
    end
end

-- Babel: Prevent players from destroying top platform.
-- Also prevent people from same team to destroy their own tower.
-- return 1: allow deletion.
-- return 0: abort deletion.
function spadesx.checks.block_destruction(player, x, y, z)
    if (((((x >= 206 and x <= 306) and (y >= 240 and y <= 272) and (z == 2 or z == 0)) or
          ((x >= 205 and x <= 307) and (y >= 239 and y <= 273) and (z == 1)))))
    then
        player:send_notice("You should try to destroy the ennemy's tower... Not the platform!")
        return 0
    end
    -- Prevent players from destroying their own tower.
    if (player.team == 1 and x > 512-220) or ( player.team == 0 and x < 220)  then
        player:send_notice("You should try to destroy the ennemy's tower... It is not on this side of the map!")
        return 0
    end
    return 1
end

-- Does nothing, allow block creation. Only here for showcase purpose.
-- Could be deleted as it will fallback on a C function doing the same.
function spadesx.checks.block_placement(player, x, y, z)
    -- Force players to build int their own color.
    player:set_color(player.color)
    -- Auto restock players.
    if player.blocks < 10 then
        player:restock()
    end
    return 1
end

-- Init the level.
-- Receive a specific init_api with functions to manipulate the level BEFORE players are joining.
-- This api won't trigger any event nor send players any packet.
function spadesx.init(init_api)
    -- Let's add some snow to the map:
    for x = 0,450, 1
    do 
        for y = 0,450, 1
        do
            init_api.add_colored_block(x,y,init_api.find_top_block(x,y), 0xffffff)
        end
    end
    -- Babel: create platform:
    for x = 206, 306 do
        for y = 240, 272 do
            init_api.add_colored_block(x, y, 1, 0xFF00FFFF)
        end
    end
    -- Babel: set intels on top of the platform:
    init_api.set_intel_position(0, 255, 255, init_api.find_top_block(255,255))
    init_api.set_intel_position(1, 255, 255, init_api.find_top_block(255,255))
end

printTable(spadesx)
return spadesx