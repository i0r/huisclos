#pragma once

struct renderContext_t;
struct window_t;
struct areaNode_t;
class Camera;
class World;

#include <d3d11.h>
#include <Editor/Graphics/Surfaces/Icon.h>

class UIManager
{
public:
    inline void	        SetTranslationMode()	                { activeManipulationMode = 0; }
    inline void	        SetRotationMode()		                { activeManipulationMode = 1; }
    inline void	        SetScaleMode()                          { activeManipulationMode = 2; }
	inline void			Toggle()                                { isToggled = !isToggled; }
	inline const bool	IsToggled() const		                { return isToggled; }
	inline const bool	IsEditing() const						{ return activeNode != nullptr; }
	inline const bool	IsInputingText() const					{ return isInputingText; }
    inline void         SetNodeEdit( areaNode_t* nodeToEdit )   { activeNode = nodeToEdit; }
    inline void		    SetActiveWorld( World* world )          { activeWorld = world; }
    inline void         AddIconToRenderList( const edEntityIcon_t& iconPos, const edIcons_t iconId ) { iconsToRender.push_back( std::make_pair( iconId, iconPos ) ); }

public:
				        UIManager();
                        UIManager( UIManager& ) = delete;
                        ~UIManager()			= default;

	void		        Initialize( const renderContext_t* context, TextureManager* texMan, const window_t* win );


	void		        Draw( const float dt, const Camera* cam );
	void		        Resize();
	const bool	        IsManipulating() const;

private:
    World*	                activeWorld;
    areaNode_t*	            activeNode;
    const renderContext_t*	renderContext;

	int			        activeManipulationMode;
	bool		        isInputingText;
	bool		        isToggled;
	bool		        sceneHiearchyWinToggled;
    bool                debugOverlayToggled;
    bool                todClockToggled;
	int			        activeColorMode;

    std::vector<std::pair<edIcons_t, edEntityIcon_t>> iconsToRender;

    SurfaceIcon         iconSurf;

private:
    void                DrawEdPanel( const float dt, const Camera* cam );
    void                DrawDebugOverlay( const float dt, const Camera* cam );
    void                DrawSceneHiearchy();
    void                DrawMenuBar( const Camera* cam );

	void				PrintNode( areaNode_t* parentNode, areaNode_t* previousNode, areaNode_t* toInsertNext );
};

extern LRESULT ImGui_ImplDX11_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
