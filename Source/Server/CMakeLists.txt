#
# Add sources
#

add_library(Server STATIC "")

set(PACKET_HEADERS
    Packets/Packets.h
)

set(PACKET_SOURCES
    Packets/BlockAction.c
    Packets/BlockLine.c
    Packets/ChangeTeam.c
    Packets/ChangeWeapon.c
    Packets/CreatePlayer.c
    Packets/ExistingPlayer.c
    Packets/Grenade.c
    Packets/Hit.c
    Packets/InputData.c
    Packets/IntelCapture.c
    Packets/IntelDrop.c
    Packets/IntelPickup.c
    Packets/KillAction.c
    Packets/MapChunk.c
    Packets/MapStart.c
    Packets/Message.c
    Packets/MoveObject.c
    Packets/OrientationData.c
    Packets/PacketManager.c
    Packets/PlayerLeft.c
    Packets/PositionData.c
    Packets/Restock.c
    Packets/SetColor.c
    Packets/SetHP.c
    Packets/SetTool.c
    Packets/ShortPlayerData.c
    Packets/StateData.c
    Packets/VersionRequestResponse.c
    Packets/WeaponInput.c
    Packets/WeaponReload.c
    Packets/WorldUpdate.c
)

set(COMMANDS_HEADERS
    Commands/CommandManager.h
    Commands/Commands.h
)

set(COMMANDS_SOURCES
    Commands/Admin.c
    Commands/Ban.c
    Commands/Clin.c
    Commands/CommandManager.c
    Commands/Help.c
    Commands/Intel.c
    Commands/Invisible.c
    Commands/Kick.c
    Commands/Kill.c
    Commands/Login.c
    Commands/Master.c
    Commands/Mute.c
    Commands/PrivateMessage.c
    Commands/Ratio.c
    Commands/Reset.c
    Commands/Say.c
    Commands/Server.c
    Commands/Teleport.c
    Commands/Toggles.c
    Commands/Unban.c
    Commands/Ups.c
    Commands/Shutdown.c
)

set(STRUCTS_HEADERS
    Structs/BlockStruct.h
    Structs/CommandStruct.h
    Structs/EventStruct.h
    Structs/GamemodeStruct.h
    Structs/GrenadeStruct.h
    Structs/IPStruct.h
    Structs/MapStruct.h
    Structs/MasterStruct.h
    Structs/MovementStruct.h
    Structs/PacketStruct.h
    Structs/PlayerStruct.h
    Structs/ProtocolStruct.h
    Structs/ServerStruct.h
    Structs/TimerStruct.h
    Structs/StartStruct.h)

set(SERVER_HEADERS
    ${COMMANDS_HEADERS}
    ${PACKET_HEADERS}
    ${STRUCTS_HEADERS}
    Block.h
    Grenade.h
    IntelTent.h
    Nodes.h
    Staff.h
    Console.h
    Events.h
    Master.h
    Server.h
    Map.h
    Gamemodes/Gamemodes.h
    Ping.h
    ParseConvert.h
    Player.h
)

set(SERVER_SOURCES
    ${COMMANDS_SOURCES}
    ${PACKET_SOURCES}
    Block.c
    Grenade.c
    IntelTent.c
    Nodes.c
    Staff.c
    Console.c
    Events.c
    Server.c
    Master.c
    Map.c
    Gamemodes/Gamemodes.c
    Ping.c
    ParseConvert.c
    Player.c
)

target_sources(Server
    PRIVATE
        ${SERVER_SOURCES}
    PUBLIC
        ${SERVER_HEADERS}
)

target_link_libraries(Server PRIVATE SpadesXCommon Util)
