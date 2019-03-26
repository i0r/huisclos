#pragma once

#include <vector>

struct mesh_t;

enum areaNodeFlag_t
{
	NODE_FLAG_EMPTY_NODE					= 1 << 0,

	NODE_FLAG_CONTENT_SPHERE_LIGHT			= 1 << 1,
	NODE_FLAG_CONTENT_DISK_LIGHT			= 1 << 2,
	NODE_FLAG_CONTENT_RECTANGLE_LIGHT		= 1 << 3,
	NODE_FLAG_CONTENT_TUBE_LIGHT			= 1 << 4,
	NODE_FLAG_CONTENT_SUN_LIGHT				= 1 << 5, // only useful for ed (toggle helper icons)

	NODE_FLAG_CONTENT_MESH					= 1 << 6,
	NODE_FLAG_CONTENT_ACTOR					= 1 << 7,
};

using nodeHash = uint64_t;

// a node is a piece of a world area
struct areaNode_t
{
	areaNode_t()
		: content( nullptr )
        , hash( 0 )
		, flags( NODE_FLAG_EMPTY_NODE )
		, parent( nullptr )
        , name( "???" )
	{

	}

	void*						content;
    nodeHash                    hash;       // random hash assigned during world insert; equals 0 if unset and/or bad node
	uint64_t					flags;		// 0-7 : content bits
	areaNode_t*					parent;		// null if none
    char						name[128];
	std::vector<areaNode_t*>	children;	// null if none
};

// a area is a piece of the world
struct worldArea_t
{
	worldArea_t()
	{
		nodes = new areaNode_t(); // populate the world with at least one node
	}

	areaNode_t*		nodes;
	unsigned char	xIndice;
	unsigned char	yIndice;
};

// aka scenemanager, worldmanager or any fancy name you could think of
class World
{
public:
	const worldArea_t*	GetActiveArea() const { return currentArea; }

public:
					World();
					World( World& )	= default;
					~World()		= default;

	void			CreateEmptyArea();

	void			LoadWorldFromFile( const char* fileName ) {}
	void			LoadAreaFromFile( const char* fileName );
    
    areaNode_t*     InsertNode( void* content, const uint64_t flags, const areaNode_t* parent = nullptr );
    void            RemoveNode( nodeHash hash );

private:
	worldArea_t*	currentArea;

	worldArea_t**	areas; // id0 => X index (h axis); id1 => Z index (v axis)
};
