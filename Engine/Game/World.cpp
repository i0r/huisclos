#include "Shared.h"
#include "World.h"

#include <Engine/System/MurmurHash2_64.h>

World::World()
	: currentArea( nullptr )
{

}

void World::CreateEmptyArea()
{
	currentArea = new worldArea_t();
}

void World::LoadAreaFromFile( const char* fileName )
{

}

areaNode_t* World::InsertNode( void* content, const uint64_t flags, const areaNode_t* parent )
{
	if ( currentArea == nullptr || currentArea->nodes == nullptr || content == nullptr ) {
		return nullptr;
	}

    const nodeHash hashKey = *static_cast<uint64_t*>( content ) ^ flags;

	if ( parent == nullptr ) {
		areaNode_t* newNode = new areaNode_t();
		newNode->content	= content;
        newNode->hash    = MurmurHash64A( &hashKey, sizeof( nodeHash ), 0xB );
		newNode->flags	= flags;
		newNode->parent	= currentArea->nodes;

		currentArea->nodes->children.push_back( newNode );

		return newNode;
	}


	return nullptr;
}

void World::RemoveNode( nodeHash hash ) 
{
    for ( auto it = currentArea->nodes->children.begin(); it != currentArea->nodes->children.end(); it++ ) {
        if ( ( *it )->hash == hash ) {
           currentArea->nodes->children.erase( it );
            return;
        }
    }
}
