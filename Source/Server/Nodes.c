#include <Server/Structs/ServerStruct.h>
#include <Util/Checks/PositionChecks.h>
#include <Util/Log.h>

vector3i_t* get_neighbours(vector3i_t pos)
{
    static vector3i_t neighArray[6];

    static const vector3i_t offsets[6] = {{0, 0, -1}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, 1}};

    for (int i = 0; i < 6; i++) {
        neighArray[i].x = pos.x + offsets[i].x;
        neighArray[i].y = pos.y + offsets[i].y;
        neighArray[i].z = pos.z + offsets[i].z;
    }

    return neighArray;
}

#define NODE_RESERVE_SIZE 250000
vector3i_t* nodes = NULL;
int         nodePos;
int         nodesSize;
map_node_t* visitedMap;

static inline void saveNode(int x, int y, int z)
{
    nodes[nodePos].x = x;
    nodes[nodePos].y = y;
    nodes[nodePos].z = z;
    nodePos++;
}

static inline const vector3i_t* returnCurrentNode(void)
{
    return &nodes[--nodePos];
}

static inline void addNode(server_t* server, int x, int y, int z)
{
    if ((x >= 0 && x < server->s_map.map.size_x) && (y >= 0 && y < server->s_map.map.size_y) &&
        (z >= 0 && z < server->s_map.map.size_z))
    {
        if (mapvxl_is_solid(&server->s_map.map, x, y, z)) {
            saveNode(x, y, z);
        }
    }
}

uint8_t check_node(server_t* server, vector3i_t position)
{
    if (valid_pos_v3i(server, position) && mapvxl_is_solid(&server->s_map.map, position.x, position.y, position.z) == 0)
    {
        return 1;
    }
    if (nodes == NULL) {
        nodes = (vector3i_t*) malloc(sizeof(vector3i_t) * NODE_RESERVE_SIZE);
        if (nodes == NULL) {
            LOG_ERROR("Allocation of memory failed. EXITING");
            server->running = 0;
            return 0;
        }
        nodesSize = NODE_RESERVE_SIZE;
    }
    nodePos    = 0;
    visitedMap = NULL;

    saveNode(position.x, position.y, position.z);

    while (nodePos > 0) {
        vector3i_t* old_nodes;
        if (nodePos >= nodesSize - 6) {
            nodesSize += NODE_RESERVE_SIZE;
            old_nodes = (vector3i_t*) realloc((void*) nodes, sizeof(vector3i_t) * nodesSize);
            if (!old_nodes) {
                free(nodes);
                return 0;
            }
            nodes = old_nodes;
        }
        const vector3i_t* currentNode = returnCurrentNode();
        position.z                    = currentNode->z;
        if (position.z >= server->s_map.map.size_z - 2) {
            if (visitedMap != NULL) {
                map_node_t* delNode;
                map_node_t* tmpNode;
                HASH_ITER(hh, visitedMap, delNode, tmpNode)
                {
                    HASH_DEL(visitedMap, delNode);
                    free(delNode);
                }
            }
            free(nodes);
            nodes = NULL;
            return 1;
        }
        position.x = currentNode->x;
        position.y = currentNode->y;

        // already visited?
        map_node_t* node;
        int         id = position.x * server->s_map.map.size_y * server->s_map.map.size_z +
                 position.y * server->s_map.map.size_z + position.z;
        HASH_FIND_INT(visitedMap, &id, node);
        if (node == NULL) {
            node          = (map_node_t*) malloc(sizeof *node);
            if (node == NULL) {
            LOG_ERROR("Allocation of memory failed. EXITING");
            server->running = 0;
            return 0;
        }
            node->pos     = position;
            node->visited = 1;
            node->id      = id;
            HASH_ADD_INT(visitedMap, id, node);
            addNode(server, position.x, position.y, position.z - 1);
            addNode(server, position.x, position.y - 1, position.z);
            addNode(server, position.x, position.y + 1, position.z);
            addNode(server, position.x - 1, position.y, position.z);
            addNode(server, position.x + 1, position.y, position.z);
            addNode(server, position.x, position.y, position.z + 1);
        }
    }

    map_node_t* Node;
    map_node_t* tmpNode;
    HASH_ITER(hh, visitedMap, Node, tmpNode)
    {
        vector3i_t block = {Node->pos.x, Node->pos.y, Node->pos.z};
        if (valid_pos_v3i(server, block)) {
            mapvxl_set_air(&server->s_map.map, Node->pos.x, Node->pos.y, Node->pos.z);
        }
        HASH_DEL(visitedMap, Node);
        free(Node);
    }
    free(nodes);
    nodes = NULL;
    return 0;
}
