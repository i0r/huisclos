#pragma once

class UIManager;
class InputManager;
class MaterialManager;

struct renderContext_t;
struct window_t;

#include <Engine/Graphics/Camera.h>

class WorldEditor
{
public:
	inline const Camera*	GetActiveCamera() const	{ return &freeCam; }
	inline Camera*			GetActiveCameraObj()	{ return &freeCam; }
	inline void				ToggleFocusOn()			{ isFocusing = !isFocusing; }
	inline void				CopyNode()				{ copyNode = selectedNode; }

public:
				            WorldEditor();
                            WorldEditor( WorldEditor& ) = delete;
                            ~WorldEditor()				= default;

	const int	            Initialize( UIManager* edUI, const renderContext_t* context, const window_t* win, InputManager* inputManager, MaterialManager* materialManager );
	void		            SetActiveWorld( World* world );
	void		            Frame( const float dt );
	void		            SelectNodeByMouse( const Camera* cam );
	void		            FocusOn();
    void                    RemoveSelectedNode();
    void                    MeshInsertCallback( char* absolutePath );
	void					PasteNode();

private:
	UIManager*		        uiMan;
    MaterialManager*        matMan;
    World*			        activeWorld;
    const renderContext_t*	renderContext;
    const window_t*			window;
	InputManager*	        inputMan;

    areaNode_t*		        selectedNode;
	areaNode_t*		        copyNode;

    bool			        isFocusing;
	FreeCamera		        freeCam;
};
